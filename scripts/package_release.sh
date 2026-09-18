#!/usr/bin/env bash
# Package release archives for Ten Flying Fingers (TFF)
# Generates portable Linux binaries with static C++ runtime
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

TAG="${1:-"dev"}"
BUILD_DIR="${2:-"${ROOT_DIR}/build-static"}"
DIST_DIR="${3:-"${ROOT_DIR}/dist"}"

echo "=================================================="
echo "Packaging TFF Release: ${TAG}"
echo "Build Dir: ${BUILD_DIR}"
echo "Dist Dir:  ${DIST_DIR}"
echo "=================================================="

# Ensure build exists and is compiled with static libstdc++
if [ ! -f "${BUILD_DIR}/tff_linux" ]; then
    echo "==> Building tff_linux with static C++ runtime..."
    cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DTFF_STATIC_LIBSTDCXX=ON \
        -DTFF_WARNINGS_AS_ERRORS=ON
    cmake --build "${BUILD_DIR}" --target tff_linux -j"$(nproc)"
fi

# Verify binary portability with ldd
echo "==> Checking binary shared library dependencies..."
if ldd "${BUILD_DIR}/tff_linux" | grep -q "libstdc++"; then
    echo "ERROR: tff_linux dynamically links libstdc++. Static linking failed." >&2
    exit 1
fi
if ldd "${BUILD_DIR}/tff_linux" | grep -q "libgcc_s"; then
    echo "ERROR: tff_linux dynamically links libgcc_s. Static linking failed." >&2
    exit 1
fi
echo "==> Portability check passed (no dynamic libstdc++ / libgcc_s dependencies)."

# Prepare distribution directories
STAGE_DIR="${DIST_DIR}/stage_linux_x86_64"
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}" "${DIST_DIR}"

# Copy binary and create aliases
echo "==> Staging files..."
cp "${BUILD_DIR}/tff_linux" "${STAGE_DIR}/tff_linux"
strip "${STAGE_DIR}/tff_linux"
chmod 0755 "${STAGE_DIR}/tff_linux"

# Provide tff and tff2 copies/symlinks for ubi and mise compatibility
cp -P "${STAGE_DIR}/tff_linux" "${STAGE_DIR}/tff"
cp -P "${STAGE_DIR}/tff_linux" "${STAGE_DIR}/tff2"

# Copy configurations and metadata
cp "${ROOT_DIR}/config/tff-combos.yaml" "${STAGE_DIR}/tff-combos.yaml"
cp "${ROOT_DIR}/ten-flying-fingers.service.example" "${STAGE_DIR}/ten-flying-fingers.service.example"
cp "${ROOT_DIR}/README.md" "${STAGE_DIR}/README.md"
if [ -f "${ROOT_DIR}/LICENSE" ]; then
    cp "${ROOT_DIR}/LICENSE" "${STAGE_DIR}/LICENSE"
fi
if [ -f "${ROOT_DIR}/docs/man/tff.1" ]; then
    mkdir -p "${STAGE_DIR}/man/man1"
    cp "${ROOT_DIR}/docs/man/tff.1" "${STAGE_DIR}/man/man1/tff.1"
    ln -sf "tff.1" "${STAGE_DIR}/man/man1/tff2.1"
    ln -sf "tff.1" "${STAGE_DIR}/man/man1/tff_linux.1"
fi
if [ -d "${ROOT_DIR}/completions" ]; then
    mkdir -p "${STAGE_DIR}/completions"
    cp -r "${ROOT_DIR}/completions/"* "${STAGE_DIR}/completions/"
fi

# Create tar archives
echo "==> Creating release archives..."
tar -czf "${DIST_DIR}/tff2_Linux_x86_64.tar.gz" -C "${STAGE_DIR}" .
cp "${DIST_DIR}/tff2_Linux_x86_64.tar.gz" "${DIST_DIR}/tff_linux_Linux_x86_64.tar.gz"

if [ "${TAG}" != "dev" ]; then
    cp "${DIST_DIR}/tff2_Linux_x86_64.tar.gz" "${DIST_DIR}/tff2_${TAG}_Linux_x86_64.tar.gz"
fi

# If RP2040 uf2 exists, copy it to dist
if [ -f "${ROOT_DIR}/build-rp2040/tff_rp2040.uf2" ]; then
    echo "==> Copying RP2040 firmware (.uf2)..."
    cp "${ROOT_DIR}/build-rp2040/tff_rp2040.uf2" "${DIST_DIR}/tff_rp2040.uf2"
fi

# Generate SHA256 checksums
echo "==> Generating SHA256 checksums..."
(
    cd "${DIST_DIR}"
    rm -f SHA256SUMS.txt
    sha256sum *.tar.gz > SHA256SUMS.txt
    if [ -f "tff_rp2040.uf2" ]; then
        sha256sum tff_rp2040.uf2 >> SHA256SUMS.txt
    fi
)

echo "=================================================="
echo "Packaging complete! Generated release assets:"
ls -lh "${DIST_DIR}"
echo "=================================================="
