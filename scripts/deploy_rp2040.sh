#!/usr/bin/env bash
# ==============================================================================
# scripts/deploy_rp2040.sh - Deploy TFF Firmware or code.py to RP2040
# Supports automated live updates (CIRCUITPY) and BOOTSEL flashing (RPI-RP2).
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

CODE_PY="${ROOT_DIR}/src/platform/rp2040/code.py"
CIRCUITPY_LABEL="/dev/disk/by-label/CIRCUITPY"
BOOTSEL_LABEL="/dev/disk/by-label/RPI-RP2"
MOUNT_POINT_CP="/mnt/circuitpy"
MOUNT_POINT_RP2="/mnt/rpi-rp2"

UF2_PATH="${1:-}"

echo "=============================================="
echo "   TFF RP2040 Deployment / Sync Tool         "
echo "=============================================="

# Check if UF2 flashing is requested
if [[ -n "${UF2_PATH}" ]]; then
    if [[ ! -f "${UF2_PATH}" ]]; then
        echo "Error: UF2 file not found: ${UF2_PATH}" >&2
        exit 1
    fi

    echo "UF2 flashing requested: ${UF2_PATH}"
    
    # Check if currently running CircuitPython and needs automated reboot
    if [[ ! -e "${BOOTSEL_LABEL}" ]] && compgen -G "/dev/ttyACM*" >/dev/null; then
        echo "RP2040 is running CircuitPython. Triggering automated reboot into bootloader..."
        python3 "${SCRIPT_DIR}/reboot_rp2040_bootloader.py" || true
        sleep 2
    fi

    if [[ ! -e "${BOOTSEL_LABEL}" ]]; then
        echo "Error: RPI-RP2 bootloader drive not detected." >&2
        echo "Please enter bootloader mode on the RP2040:" >&2
        echo "  1. Hold the BOOT button." >&2
        echo "  2. Click (press and release) the RESET button." >&2
        echo "  3. Release the BOOT button." >&2
        exit 1
    fi

    mkdir -p "${MOUNT_POINT_RP2}"
    mount "${BOOTSEL_LABEL}" "${MOUNT_POINT_RP2}"
    echo "Flashing ${UF2_PATH} to RP2040..."
    cp "${UF2_PATH}" "${MOUNT_POINT_RP2}/"
    sync
    echo "✓ UF2 firmware flashed successfully! Board is rebooting..."
    exit 0
fi

# Standard deployment: deploy code.py to CIRCUITPY drive
echo "Deploying ${CODE_PY}..."

if [[ ! -e "${CIRCUITPY_LABEL}" ]]; then
    # Maybe the board is in RPI-RP2 mode
    if [[ -e "${BOOTSEL_LABEL}" ]]; then
        echo "RP2040 is in BOOTSEL mode (RPI-RP2)."
        echo "To run CircuitPython, please flash the CircuitPython UF2 first:"
        echo "  $0 /path/to/adafruit-circuitpython-*.uf2"
        exit 1
    fi

    echo "Waiting for CIRCUITPY drive (${CIRCUITPY_LABEL})..."
    sleep 1
fi

if [[ ! -e "${CIRCUITPY_LABEL}" ]]; then
    echo "Error: CIRCUITPY drive not found." >&2
    echo "Is the RP2040 connected to the computer via USB-C with CircuitPython installed?" >&2
    exit 1
fi

mkdir -p "${MOUNT_POINT_CP}"
if ! mountpoint -q "${MOUNT_POINT_CP}"; then
    mount "${CIRCUITPY_LABEL}" "${MOUNT_POINT_CP}"
fi

echo "Copying code.py to ${MOUNT_POINT_CP}/code.py..."
cp "${CODE_PY}" "${MOUNT_POINT_CP}/code.py"
sync
echo "✓ code.py deployed successfully! CircuitPython will auto-reload."
