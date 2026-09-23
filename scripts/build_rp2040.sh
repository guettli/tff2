#!/usr/bin/env bash
# ==============================================================================
# scripts/build_rp2040.sh - Automated RP2040 Firmware Cross-Compilation Script
# Compiles Ten Flying Fingers for RP2040 boards and produces tff_rp2040.uf2
# ==============================================================================
set -euo pipefail

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
    DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${1:-"${ROOT_DIR}/build-rp2040"}"
BOARD="${PICO_BOARD:-adafruit_feather_rp2040}"
BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"

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

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    echo "Usage: $0 [BUILD_DIR]"
    echo ""
    echo "Cross-compiles Ten Flying Fingers for the RP2040 microcontroller."
    echo "Produces tff_rp2040.uf2 ready for drag-and-drop BOOTSEL flashing."
    echo ""
    echo "Environment Variables:"
    echo "  PICO_SDK_PATH      Path to Raspberry Pi Pico SDK (auto-cloned if unset)"
    echo "  PICO_BOARD         Target board (default: adafruit_feather_rp2040)"
    echo "  CMAKE_BUILD_TYPE   Build type: Release or Debug (default: Release)"
    echo ""
    echo "Examples:"
    echo "  $0"
    echo "  PICO_BOARD=pico $0 build-pico"
    exit 0
fi

echo -e "${COLOR_BLUE}======================================================${COLOR_RESET}"
echo -e "${COLOR_BLUE}   TFF2 - RP2040 Firmware Cross-Compilation          ${COLOR_RESET}"
echo -e "${COLOR_BLUE}======================================================${COLOR_RESET}"
echo "Board:      ${BOARD}"
echo "Build Dir:  ${BUILD_DIR}"
echo "Build Type: ${BUILD_TYPE}"

# 1. Check for ARM GCC cross-compiler
step "Checking ARM GCC toolchain (arm-none-eabi-gcc)..."
if ! command -v arm-none-eabi-gcc &>/dev/null; then
    echo -e "${COLOR_RED}Error: arm-none-eabi-gcc not found on PATH.${COLOR_RESET}" >&2
    echo "To install the required toolchain on Debian / Ubuntu:" >&2
    echo "  sudo apt-get update && sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib" >&2
    echo "Or on macOS (Homebrew):" >&2
    echo "  brew install --cask gcc-arm-embedded" >&2
    fail "ARM GCC toolchain is missing."
fi
ARM_GCC_VERSION="$(arm-none-eabi-gcc --version | head -n 1)"
success "Found ARM toolchain: ${ARM_GCC_VERSION}"

# 2. Check or set up Raspberry Pi Pico SDK
step "Checking Raspberry Pi Pico SDK..."
PICO_SDK_VERSION="2.1.1"
if [ -z "${PICO_SDK_PATH:-}" ]; then
    if [ -d "${ROOT_DIR}/pico-sdk" ]; then
        PICO_SDK_PATH="${ROOT_DIR}/pico-sdk"
    elif [ -d "${HOME}/pico-sdk" ]; then
        PICO_SDK_PATH="${HOME}/pico-sdk"
    else
        echo "PICO_SDK_PATH is not set. Fetching shallow clone of Pico SDK v${PICO_SDK_VERSION}..."
        git clone --depth 1 --branch "${PICO_SDK_VERSION}" https://github.com/raspberrypi/pico-sdk.git "${ROOT_DIR}/pico-sdk"
        PICO_SDK_PATH="${ROOT_DIR}/pico-sdk"
    fi
fi

if [ ! -d "${PICO_SDK_PATH}" ]; then
    fail "Pico SDK directory does not exist: ${PICO_SDK_PATH}"
fi

# Ensure TinyUSB submodule is initialized in Pico SDK
if [ ! -f "${PICO_SDK_PATH}/lib/tinyusb/src/tusb.h" ]; then
    echo "Initializing TinyUSB submodule in Pico SDK..."
    (
        cd "${PICO_SDK_PATH}"
        git submodule update --init --depth 1 lib/tinyusb
    )
fi
success "Pico SDK ready at: ${PICO_SDK_PATH}"

# 3. Configure CMake for RP2040
step "Configuring CMake for RP2040 (Board: ${BOARD})..."
cmake -B "${BUILD_DIR}" -S "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DPICO_BUILD=ON \
    -DPICO_SDK_PATH="${PICO_SDK_PATH}" \
    -DPICO_BOARD="${BOARD}"
success "CMake configuration successful"

# 4. Build RP2040 Firmware
step "Building RP2040 firmware target (tff_rp2040)..."
cmake --build "${BUILD_DIR}" --target tff_rp2040 --parallel
success "Firmware compiled successfully"

# 5. Verify Generated Artifacts
step "Verifying generated UF2 binary..."
UF2_FILE="${BUILD_DIR}/tff_rp2040.uf2"
ELF_FILE="${BUILD_DIR}/tff_rp2040.elf"

if [ ! -f "${UF2_FILE}" ]; then
    fail "UF2 binary was not generated: ${UF2_FILE}"
fi

UF2_SIZE="$(stat -c %s "${UF2_FILE}" 2>/dev/null || stat -f %z "${UF2_FILE}")"
if command -v sha256sum &>/dev/null; then
    UF2_SHA="$(sha256sum "${UF2_FILE}" | awk '{print $1}')"
elif command -v shasum &>/dev/null; then
    UF2_SHA="$(shasum -a 256 "${UF2_FILE}" | awk '{print $1}')"
else
    UF2_SHA="unavailable"
fi

echo -e "\n${COLOR_GREEN}======================================================${COLOR_RESET}"
echo -e "${COLOR_GREEN}   RP2040 Firmware Built Successfully!               ${COLOR_RESET}"
echo -e "${COLOR_GREEN}======================================================${COLOR_RESET}"
echo "Binary:     ${UF2_FILE}"
echo "Size:       ${UF2_SIZE} bytes"
echo "SHA256:     ${UF2_SHA}"
if [ -f "${ELF_FILE}" ]; then
    echo "ELF debug:  ${ELF_FILE}"
fi
echo ""
echo "Flashing instructions:"
echo "  1. Hold down the BOOTSEL button on your board while plugging in USB."
echo "  2. Drag and drop '${UF2_FILE}' onto the 'RPI-RP2' USB drive."
echo "  3. The board will automatically reboot and start running Ten Flying Fingers."
