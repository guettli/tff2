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

UF2_PATH=""
DEPLOY_PYTHON=false

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    echo "Usage: $0 [FIRMWARE.uf2 | --python]"
    echo ""
    echo "Deploy Ten Flying Fingers firmware or CircuitPython code.py to RP2040."
    echo ""
    echo "Options / Arguments:"
    echo "  [no args]       Default: auto-detect and flash compiled C++ firmware (tff_rp2040.uf2)"
    echo "  FIRMWARE.uf2    Flash a specific .uf2 firmware binary to RP2040"
    echo "  --python        Deploy src/platform/rp2040/code.py to CIRCUITPY drive (legacy)"
    echo ""
    echo "Examples:"
    echo "  $0                                # Flashes build-rp2040/tff_rp2040.uf2"
    echo "  $0 build/tff_rp2040.uf2           # Flashes specific UF2"
    echo "  $0 --python                       # Syncs code.py to CIRCUITPY drive"
    exit 0
fi

if [[ "${1:-}" == "--python" || "${1:-}" == "--circuitpython" || "${1:-}" == "code.py" ]]; then
    DEPLOY_PYTHON=true
elif [[ -n "${1:-}" ]]; then
    UF2_PATH="$1"
else
    # Auto-detect compiled C++ UF2 binary
    for candidate in \
        "${ROOT_DIR}/build-rp2040/tff_rp2040.uf2" \
        "${ROOT_DIR}/build/tff_rp2040.uf2" \
        "${ROOT_DIR}/tff_rp2040.uf2" \
        "tff_rp2040.uf2"; do
        if [[ -f "${candidate}" ]]; then
            UF2_PATH="${candidate}"
            break
        fi
    done
fi

get_mount_point() {
    local label_dev="$1"
    local default_target="$2"

    # 1. Check if already mounted
    local existing
    existing="$(findmnt -n -o TARGET "${label_dev}" 2>/dev/null || true)"
    if [[ -n "${existing}" && -d "${existing}" ]]; then
        echo "${existing}"
        return 0
    fi

    # 2. Check desktop media mounts
    local label_name="${label_dev##*/}"
    for candidate in /media/*/"${label_name}" /run/media/*/"${label_name}"; do
        if [[ -d "${candidate}" ]]; then
            echo "${candidate}"
            return 0
        fi
    done

    # 3. Try udisksctl mount (unprivileged user friendly)
    if command -v udisksctl &>/dev/null; then
        local udisks_out
        udisks_out="$(udisksctl mount -b "${label_dev}" 2>/dev/null || true)"
        existing="$(echo "${udisks_out}" | grep -o '/.*' || true)"
        if [[ -n "${existing}" && -d "${existing}" ]]; then
            echo "${existing}"
            return 0
        fi
    fi

    # 4. Fallback to sudo / root mount
    mkdir -p "${default_target}" 2>/dev/null || sudo mkdir -p "${default_target}"
    if ! mountpoint -q "${default_target}"; then
        mount "${label_dev}" "${default_target}" 2>/dev/null || sudo mount "${label_dev}" "${default_target}"
    fi
    echo "${default_target}"
}

echo "=============================================="
echo "   TFF RP2040 Deployment / Sync Tool         "
echo "=============================================="

# Check if UF2 flashing is requested (or auto-detected)
if [[ "${DEPLOY_PYTHON}" == "false" && -n "${UF2_PATH}" ]]; then
    if [[ ! -f "${UF2_PATH}" ]]; then
        echo "Error: UF2 file not found: ${UF2_PATH}" >&2
        exit 1
    fi

    echo "Target UF2 firmware: ${UF2_PATH}"

    # Check if currently running and needs automated reboot into bootloader
    if [[ ! -e "${BOOTSEL_LABEL}" ]]; then
        if compgen -G "/dev/ttyACM*" >/dev/null || compgen -G "/dev/ttyUSB*" >/dev/null; then
            echo "RP2040 detected on USB serial. Triggering automated reboot into bootloader..."
            python3 "${SCRIPT_DIR}/reboot_rp2040_bootloader.py" || true
            sleep 1
        fi
    fi

    # Wait up to 5 seconds for RPI-RP2 drive to appear
    for ((i = 0; i < 10; i++)); do
        if [[ -e "${BOOTSEL_LABEL}" ]]; then
            break
        fi
        sleep 0.5
    done

    if [[ ! -e "${BOOTSEL_LABEL}" ]]; then
        echo "Error: RPI-RP2 bootloader drive not detected (${BOOTSEL_LABEL})." >&2
        echo "Please enter bootloader mode on the RP2040:" >&2
        echo "  1. Hold the BOOT button." >&2
        echo "  2. Click (press and release) the RESET button." >&2
        echo "  3. Release the BOOT button." >&2
        exit 1
    fi

    TARGET_DIR="$(get_mount_point "${BOOTSEL_LABEL}" "${MOUNT_POINT_RP2}")"
    echo "Flashing ${UF2_PATH} to RP2040 (${TARGET_DIR})..."
    cp "${UF2_PATH}" "${TARGET_DIR}/"
    sync
    echo "✓ UF2 firmware flashed successfully! Board is rebooting..."
    exit 0
fi

if [[ "${DEPLOY_PYTHON}" == "false" && -z "${UF2_PATH}" && ! -e "${CIRCUITPY_LABEL}" ]]; then
    echo "No compiled tff_rp2040.uf2 firmware found."
    echo ""
    echo "To compile the pure C++ firmware:"
    echo "  ./build_rp2040.sh"
    echo ""
    echo "Or supply a pre-built UF2 binary:"
    echo "  $0 path/to/tff_rp2040.uf2"
    echo ""
    echo "Or if you intended to deploy CircuitPython code.py:"
    echo "  $0 --python"
    exit 1
fi

# Fallback or explicit request: deploy code.py to CIRCUITPY drive
echo "Deploying CircuitPython ${CODE_PY}..."

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

TARGET_DIR="$(get_mount_point "${CIRCUITPY_LABEL}" "${MOUNT_POINT_CP}")"
echo "Copying code.py to ${TARGET_DIR}/code.py..."
cp "${CODE_PY}" "${TARGET_DIR}/code.py"
sync
echo "✓ code.py deployed successfully! CircuitPython will auto-reload."
