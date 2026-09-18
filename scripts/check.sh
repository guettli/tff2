#!/usr/bin/env bash
# ==============================================================================
# scripts/check.sh - Unified Local Verification & Sanity Check Script
# Runs code formatting check, strict compilation (-Werror), test suite (ctest),
# Cppcheck static analysis, and CLI smoke verification.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    echo "Usage: $0 [--sanitizers|--all]"
    echo "Runs unified local verification: clang-format check, -Werror build, ctest,"
    echo "cppcheck static analysis, and CLI smoke tests."
    echo ""
    echo "Options:"
    echo "  --sanitizers, --all   Also run AddressSanitizer and UndefinedBehaviorSanitizer suite"
    echo "  -h, --help            Show this help message"
    exit 0
fi

COLOR_RESET="\033[0m"
COLOR_GREEN="\033[1;32m"
COLOR_YELLOW="\033[1;33m"
COLOR_RED="\033[1;31m"
COLOR_BLUE="\033[1;34m"

step() {
    echo -e "\n${COLOR_BLUE}==>${COLOR_RESET} ${COLOR_YELLOW}$1${COLOR_RESET}"
}

success() {
    echo -e "${COLOR_GREEN}✓ $1${COLOR_RESET}"
}

fail() {
    echo -e "${COLOR_RED}✗ $1${COLOR_RESET}" >&2
    exit 1
}

echo -e "${COLOR_BLUE}======================================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}   TFF2 - Local Developer Verification & Quality Check${COLOR_RESET}"
echo -e "${COLOR_BLUE}======================================================${COLOR_RESET}"

# 1. Formatting Check
step "Checking code formatting with clang-format..."
CLANG_FORMAT_BIN=""
for candidate in clang-format clang-format-18 clang-format-17 clang-format-16 clang-format-15 clang-format-14 "${HOME}/.local/bin/clang-format"; do
    if command -v "$candidate" &>/dev/null; then
        CLANG_FORMAT_BIN="$candidate"
        break
    fi
done

if [[ -z "${CLANG_FORMAT_BIN}" ]]; then
    fail "clang-format not found. Please install clang-format (e.g. via pip install clang-format or apt install clang-format)"
fi

if ! find src include tests -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -print0 | xargs -0 "${CLANG_FORMAT_BIN}" --dry-run --Werror; then
    fail "Code formatting check failed. Run 'cmake --build build --target format' to reformat code automatically."
fi
success "Code formatting check passed"

# 2. CMake Configuration (Release with Warnings as Errors)
step "Configuring CMake (Release, Warnings-as-Errors)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release -DTFF_WARNINGS_AS_ERRORS=ON
success "CMake configuration successful"

# 3. Build All Targets
step "Building all targets..."
cmake --build build --parallel
success "All targets built cleanly with zero warnings"

# 4. Run Unit and Integration Tests via CTest
step "Running all unit and integration test suites..."
ctest --test-dir build --output-on-failure
success "All test suites passed"

# 5. Run Static Analysis (Cppcheck)
step "Running Cppcheck static analysis..."
if command -v cppcheck &>/dev/null || [[ -x "${HOME}/.local/bin/cppcheck" ]]; then
    cmake --build build --target cppcheck
    success "Cppcheck analysis passed with zero errors or warnings"
else
    echo -e "${COLOR_YELLOW}! cppcheck not found, skipping static analysis step${COLOR_RESET}"
fi

# 6. CLI Smoke Tests
step "Running CLI smoke verification..."
./build/tff_linux --help >/dev/null
./build/tff_linux --version >/dev/null
./build/tff_linux -V >/dev/null
./build/tff_linux --list >/dev/null || true
./build/tff_linux validate config/tff-combos.yaml >/dev/null
./build/tff_linux cheatsheet --plain >/dev/null
./build/tff_linux setup-udev --print >/dev/null
success "CLI smoke tests passed"

# 7. Optional Sanitizers check if requested
if [[ "${1:-}" == "--sanitizers" || "${1:-}" == "--all" ]]; then
    step "Running AddressSanitizer & UndefinedBehaviorSanitizer checks..."
    export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:fast_unwind_on_fatal=0"
    export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"
    cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
    cmake --build build-asan --parallel
    ctest --test-dir build-asan --output-on-failure
    success "Sanitizers test suite passed with zero errors"
fi

echo -e "\n${COLOR_GREEN}======================================================${COLOR_RESET}"
echo -e "${COLOR_GREEN}   All checks passed successfully! Ready to push / PR.${COLOR_RESET}"
echo -e "${COLOR_GREEN}======================================================${COLOR_RESET}"
