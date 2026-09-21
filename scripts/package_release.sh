#!/usr/bin/env bash
# Package release archives for Ten Flying Fingers (TFF)
# Generates portable Linux binaries with static C++ runtime
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

TAG="${1:-"dev"}"
BUILD_DIR="${2:-"${ROOT_DIR}/build-static"}"
DIST_DIR="${3:-"${ROOT_DIR}/dist"}"

mkdir -p "${BUILD_DIR}" "${DIST_DIR}"
BUILD_DIR="$(cd "${BUILD_DIR}" && pwd)"
DIST_DIR="$(cd "${DIST_DIR}" && pwd)"

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

# ==================================================
# Build Debian (.deb) package
# ==================================================
echo "==> Preparing Debian package staging..."

if command -v dpkg --print-architecture >/dev/null 2>&1; then
    DEB_ARCH="$(dpkg --print-architecture)"
else
    case "$(uname -m)" in
        x86_64) DEB_ARCH="amd64" ;;
        aarch64) DEB_ARCH="arm64" ;;
        armv7l) DEB_ARCH="armhf" ;;
        *) DEB_ARCH="$(uname -m)" ;;
    esac
fi

DEB_VERSION="${TAG#v}"
if [ "${DEB_VERSION}" = "dev" ] || [ -z "${DEB_VERSION}" ]; then
    DEB_VERSION="2.0.0-dev"
elif [[ ! "${DEB_VERSION}" =~ ^[0-9] ]]; then
    DEB_VERSION="2.0.0-${DEB_VERSION}"
fi

STAGE_DEB="${DIST_DIR}/stage_deb_${DEB_ARCH}"
rm -rf "${STAGE_DEB}"
mkdir -p "${STAGE_DEB}/DEBIAN"
mkdir -p "${STAGE_DEB}/usr/bin"
mkdir -p "${STAGE_DEB}/etc/tff"
mkdir -p "${STAGE_DEB}/lib/udev/rules.d"
mkdir -p "${STAGE_DEB}/lib/systemd/system"
mkdir -p "${STAGE_DEB}/usr/lib/systemd/user"
mkdir -p "${STAGE_DEB}/usr/share/bash-completion/completions"
mkdir -p "${STAGE_DEB}/usr/share/zsh/site-functions"
mkdir -p "${STAGE_DEB}/usr/share/fish/vendor_completions.d"
mkdir -p "${STAGE_DEB}/usr/share/man/man1"
mkdir -p "${STAGE_DEB}/usr/share/doc/tff2"

# Copy binary & symlinks
cp "${STAGE_DIR}/tff_linux" "${STAGE_DEB}/usr/bin/tff_linux"
chmod 0755 "${STAGE_DEB}/usr/bin/tff_linux"
ln -sf "tff_linux" "${STAGE_DEB}/usr/bin/tff"
ln -sf "tff_linux" "${STAGE_DEB}/usr/bin/tff2"

# Configuration
cp "${ROOT_DIR}/config/tff-combos.yaml" "${STAGE_DEB}/etc/tff/tff-combos.yaml"
chmod 0644 "${STAGE_DEB}/etc/tff/tff-combos.yaml"

# Udev rules
cp "${ROOT_DIR}/packaging/udev/99-tff.rules" "${STAGE_DEB}/lib/udev/rules.d/99-tff.rules"
chmod 0644 "${STAGE_DEB}/lib/udev/rules.d/99-tff.rules"

# Systemd units
cp "${ROOT_DIR}/packaging/systemd/system/ten-flying-fingers.service" "${STAGE_DEB}/lib/systemd/system/ten-flying-fingers.service"
chmod 0644 "${STAGE_DEB}/lib/systemd/system/ten-flying-fingers.service"
cp "${ROOT_DIR}/packaging/systemd/user/ten-flying-fingers.service" "${STAGE_DEB}/usr/lib/systemd/user/ten-flying-fingers.service"
chmod 0644 "${STAGE_DEB}/usr/lib/systemd/user/ten-flying-fingers.service"

# Shell completions
cp "${ROOT_DIR}/completions/bash/tff" "${STAGE_DEB}/usr/share/bash-completion/completions/tff"
chmod 0644 "${STAGE_DEB}/usr/share/bash-completion/completions/tff"
ln -sf "tff" "${STAGE_DEB}/usr/share/bash-completion/completions/tff2"
ln -sf "tff" "${STAGE_DEB}/usr/share/bash-completion/completions/tff_linux"

cp "${ROOT_DIR}/completions/zsh/_tff" "${STAGE_DEB}/usr/share/zsh/site-functions/_tff"
chmod 0644 "${STAGE_DEB}/usr/share/zsh/site-functions/_tff"
ln -sf "_tff" "${STAGE_DEB}/usr/share/zsh/site-functions/_tff2"
ln -sf "_tff" "${STAGE_DEB}/usr/share/zsh/site-functions/_tff_linux"

cp "${ROOT_DIR}/completions/fish/tff.fish" "${STAGE_DEB}/usr/share/fish/vendor_completions.d/tff.fish"
chmod 0644 "${STAGE_DEB}/usr/share/fish/vendor_completions.d/tff.fish"
ln -sf "tff.fish" "${STAGE_DEB}/usr/share/fish/vendor_completions.d/tff2.fish"
ln -sf "tff.fish" "${STAGE_DEB}/usr/share/fish/vendor_completions.d/tff_linux.fish"

# Man page (compressed)
gzip -9 -c "${ROOT_DIR}/docs/man/tff.1" > "${STAGE_DEB}/usr/share/man/man1/tff.1.gz"
chmod 0644 "${STAGE_DEB}/usr/share/man/man1/tff.1.gz"
ln -sf "tff.1.gz" "${STAGE_DEB}/usr/share/man/man1/tff2.1.gz"
ln -sf "tff.1.gz" "${STAGE_DEB}/usr/share/man/man1/tff_linux.1.gz"

# Docs & license
cp "${ROOT_DIR}/README.md" "${STAGE_DEB}/usr/share/doc/tff2/README.md"
chmod 0644 "${STAGE_DEB}/usr/share/doc/tff2/README.md"
if [ -f "${ROOT_DIR}/LICENSE" ]; then
    cp "${ROOT_DIR}/LICENSE" "${STAGE_DEB}/usr/share/doc/tff2/copyright"
    chmod 0644 "${STAGE_DEB}/usr/share/doc/tff2/copyright"
fi

# Package control files
cp "${ROOT_DIR}/packaging/debian/conffiles" "${STAGE_DEB}/DEBIAN/conffiles"
chmod 0644 "${STAGE_DEB}/DEBIAN/conffiles"
cp "${ROOT_DIR}/packaging/debian/prerm" "${STAGE_DEB}/DEBIAN/prerm"
chmod 0755 "${STAGE_DEB}/DEBIAN/prerm"
cp "${ROOT_DIR}/packaging/debian/postinst" "${STAGE_DEB}/DEBIAN/postinst"
chmod 0755 "${STAGE_DEB}/DEBIAN/postinst"
cp "${ROOT_DIR}/packaging/debian/postrm" "${STAGE_DEB}/DEBIAN/postrm"
chmod 0755 "${STAGE_DEB}/DEBIAN/postrm"

# Calculate installed size (in KB)
INSTALLED_SIZE=$(du -sk "${STAGE_DEB}/usr" "${STAGE_DEB}/etc" "${STAGE_DEB}/lib" 2>/dev/null | awk '{s+=$1} END {print s}')
sed -e "s/@VERSION@/${DEB_VERSION}/g" \
    -e "s/@ARCH@/${DEB_ARCH}/g" \
    -e "s/@INSTALLED_SIZE@/${INSTALLED_SIZE}/g" \
    "${ROOT_DIR}/packaging/debian/control.in" > "${STAGE_DEB}/DEBIAN/control"
chmod 0644 "${STAGE_DEB}/DEBIAN/control"

# Generate md5sums for package contents
(
    cd "${STAGE_DEB}"
    find usr etc lib -type f -print0 | xargs -0 md5sum > "${STAGE_DEB}/DEBIAN/md5sums"
    chmod 0644 "${STAGE_DEB}/DEBIAN/md5sums"
)

DEB_FILENAME="tff2_${DEB_VERSION}_${DEB_ARCH}.deb"
echo "==> Building Debian package with dpkg-deb: ${DEB_FILENAME}..."
dpkg-deb --build --root-owner-group "${STAGE_DEB}" "${DIST_DIR}/${DEB_FILENAME}"

# Create convenient aliases/copies
if [ "${DIST_DIR}/${DEB_FILENAME}" != "${DIST_DIR}/tff2_${DEB_ARCH}.deb" ]; then
    cp -P "${DIST_DIR}/${DEB_FILENAME}" "${DIST_DIR}/tff2_${DEB_ARCH}.deb"
fi
if [ "${TAG}" != "dev" ] && [ "${DIST_DIR}/${DEB_FILENAME}" != "${DIST_DIR}/tff2_${TAG}_${DEB_ARCH}.deb" ]; then
    cp -P "${DIST_DIR}/${DEB_FILENAME}" "${DIST_DIR}/tff2_${TAG}_${DEB_ARCH}.deb"
fi

# Verify package metadata and structure
echo "==> Verifying Debian package metadata..."
dpkg-deb -I "${DIST_DIR}/${DEB_FILENAME}"

echo "==> Verifying Debian package file tree..."
dpkg-deb -c "${DIST_DIR}/${DEB_FILENAME}"

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
    sha256sum *.tar.gz *.deb > SHA256SUMS.txt
    if [ -f "tff_rp2040.uf2" ]; then
        sha256sum tff_rp2040.uf2 >> SHA256SUMS.txt
    fi
)

echo "=================================================="
echo "Packaging complete! Generated release assets:"
ls -lh "${DIST_DIR}"
echo "=================================================="
