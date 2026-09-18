#!/usr/bin/env bash
# TFF2 - Code Coverage Collector and Reporter
# Compiles with --coverage, runs tests, and summarizes line coverage
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${ROOT_DIR}/build-coverage"
SUMMARY_ONLY=false
NO_BUILD=false

usage() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -b, --build-dir DIR    Build directory to use (default: build-coverage)"
    echo "  -s, --summary          Output only the final summary table"
    echo "  -n, --no-build         Skip CMake build (assumes targets are already built)"
    echo "  -h, --help             Show this help message"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--build-dir)
            if [[ $# -lt 2 || -z "${2:-}" ]]; then
                echo "Error: --build-dir requires a directory argument" >&2
                usage
            fi
            BUILD_DIR="$2"
            shift 2
            ;;
        -s|--summary)
            SUMMARY_ONLY=true
            shift
            ;;
        -n|--no-build)
            NO_BUILD=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            ;;
    esac
done

COLOR_GREEN="\033[1;32m"
COLOR_BLUE="\033[1;34m"
COLOR_YELLOW="\033[1;33m"
COLOR_RESET="\033[0m"

if [ "${SUMMARY_ONLY}" = false ]; then
    echo -e "${COLOR_BLUE}======================================================${COLOR_RESET}"
    echo -e "${COLOR_BLUE}   TFF2 - Code Coverage Instrumentation & Analysis    ${COLOR_RESET}"
    echo -e "${COLOR_BLUE}======================================================${COLOR_RESET}"
fi

# 1. Configure and build if not skipped
if [ "${NO_BUILD}" = false ]; then
    if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ] || ! grep -q "ENABLE_COVERAGE:BOOL=ON" "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null; then
        if [ "${SUMMARY_ONLY}" = false ]; then
            echo -e "\n==> Configuring CMake with coverage instrumentation..."
        fi
        cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
            -DCMAKE_BUILD_TYPE=Debug \
            -DENABLE_COVERAGE=ON \
            -DTFF_WARNINGS_AS_ERRORS=ON >/dev/null
    fi

    if [ "${SUMMARY_ONLY}" = false ]; then
        echo -e "\n==> Building all targets with coverage flags..."
    fi
    if [ "${SUMMARY_ONLY}" = true ]; then
        cmake --build "${BUILD_DIR}" --parallel >/dev/null
    else
        cmake --build "${BUILD_DIR}" --parallel
    fi
fi

# 2. Reset coverage counters before running tests
if [ "${SUMMARY_ONLY}" = false ]; then
    echo -e "\n==> Clearing previous coverage counters..."
fi
find "${BUILD_DIR}" -name "*.gcda" -delete 2>/dev/null || true

# 3. Run CTest test suites
if [ "${SUMMARY_ONLY}" = false ]; then
    echo -e "\n==> Running all 15 unit test suites..."
fi
if [ "${SUMMARY_ONLY}" = true ]; then
    ctest --test-dir "${BUILD_DIR}" --output-on-failure >/dev/null
else
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi

# 4. Collect and summarize gcov statistics
if [ "${SUMMARY_ONLY}" = false ]; then
    echo -e "\n==> Collecting and analyzing code coverage with gcov..."
fi

python3 - <<EOF
import os
import subprocess
import re
import sys

root_dir = "${ROOT_DIR}"
build_dir = "${BUILD_DIR}"
gcov_bin = os.environ.get("GCOV_BIN", "gcov")

# Find all .gcda files in build directory
gcda_files = []
for root, _, files in os.walk(build_dir):
    for f in files:
        if f.endswith('.gcda'):
            gcda_files.append(os.path.join(root, f))

if not gcda_files:
    print("No .gcda coverage data files found. Did tests run?", file=sys.stderr)
    sys.exit(1)

file_stats = {}

for gcda in gcda_files:
    # Use -n (--no-output) so gcov does not write .gcov files to disk
    cmd = [gcov_bin, "-n", "-o", os.path.dirname(gcda), gcda]
    res = subprocess.run(cmd, cwd=build_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0 and res.stderr:
        print(f"Warning: {gcov_bin} failed on {gcda}: {res.stderr.strip()}", file=sys.stderr)
    
    current_file = None
    for line in res.stdout.splitlines():
        m_file = re.match(r"^File '(.+)'", line)
        if m_file:
            current_file = m_file.group(1)
            continue
        # Match percentage with optional decimals and optional leading spaces
        m_lines = re.match(r"^Lines executed:\s*(\d+(?:\.\d+)?)% of (\d+)", line)
        if m_lines and current_file:
            pct = float(m_lines.group(1))
            total = int(m_lines.group(2))
            
            # Filter only source/header files belonging to our repository
            abs_current = os.path.abspath(os.path.join(build_dir, current_file))
            if abs_current.startswith(root_dir) and ('/src/' in abs_current or '/include/' in abs_current):
                rel_path = os.path.relpath(abs_current, root_dir)
                if rel_path not in file_stats or total > file_stats[rel_path]['total']:
                    exec_lines = int(round(pct * total / 100.0))
                    file_stats[rel_path] = {'pct': pct, 'total': total, 'exec': exec_lines}

if not file_stats:
    print("No repository source files found in gcov output.", file=sys.stderr)
    sys.exit(1)

# Format summary table
print("-" * 78)
print(f"{'Source File':<42} {'Executed':>10} {'Total Lines':>12} {'Coverage':>10}")
print("-" * 78)

total_exec = 0
total_lines = 0

for f in sorted(file_stats.keys()):
    stats = file_stats[f]
    total_exec += stats['exec']
    total_lines += stats['total']
    pct_str = f"{stats['pct']:.1f}%"
    print(f"{f:<42} {stats['exec']:>10} {stats['total']:>12} {pct_str:>10}")

print("=" * 78)
overall_pct = (total_exec / total_lines * 100.0) if total_lines > 0 else 0.0
print(f"{'TOTAL CORE & PLATFORM COVERAGE':<42} {total_exec:>10} {total_lines:>12} {overall_pct:>9.1f}%")
print("=" * 78)

if overall_pct >= 80.0:
    print(f"\n\033[1;32m✓ High test coverage achieved: {overall_pct:.1f}% (target >= 80%)\033[0m")
else:
    print(f"\n\033[1;33m! Test coverage: {overall_pct:.1f}%\033[0m")
EOF

if [ "${SUMMARY_ONLY}" = false ]; then
    echo -e "\n${COLOR_GREEN}======================================================${COLOR_RESET}"
    echo -e "${COLOR_GREEN}   Coverage analysis complete!                         ${COLOR_RESET}"
    echo -e "${COLOR_GREEN}======================================================${COLOR_RESET}"
fi
