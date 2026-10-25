#!/usr/bin/env bash
# ==============================================================================
# scripts/deploy_rp2040.sh - Deploy TFF C++ Firmware to RP2040
# Flashes a tff_rp2040.uf2 to an RP2040 in BOOTSEL mode (RPI-RP2).
#
# Default (no args): auto-update. Fetches the latest GitHub release, compares it
# with the version currently running on the connected board, and flashes only if
# they differ. If the board is already on the latest version, it prints that and
# does nothing.
#
# Fails fast on any error or missing dependency.
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BOOTSEL_LABEL="/dev/disk/by-label/RPI-RP2"
MOUNT_POINT_RP2="/mnt/rpi-rp2"
DEFAULT_UF2="${ROOT_DIR}/build-rp2040/tff_rp2040.uf2"

# GitHub project and USB identity (must match src/platform/rp2040/usb_descriptors.c)
REPO="guettli/tff2"
UF2_ASSET="tff_rp2040.uf2"
USB_VID="cafe"
USB_PID="4004"

MODE="auto"       # auto | local | file
FORCE=0
UF2_ARG=""

usage() {
    cat <<EOF
Usage: $0 [OPTIONS] [FIRMWARE.uf2]

Deploy Ten Flying Fingers C++ firmware (UF2) to an RP2040 microcontroller.

Modes:
  (no args)       Auto-update: download the latest GitHub release and flash it,
                  unless the connected board already runs that version.
  --local         Flash the locally built firmware (${DEFAULT_UF2}).
  FIRMWARE.uf2    Flash a specific .uf2 file.

Options:
  --force         In auto mode, flash even if the board is already up to date.
  -h, --help      Show this help and exit.

Examples:
  $0                                # Auto-update to the latest release
  $0 --force                        # Re-flash latest even if already current
  $0 --local                        # Flash build-rp2040/tff_rp2040.uf2
  $0 build-rp2040/tff_rp2040.uf2    # Flash a specific UF2
EOF
}

# --- Argument parsing ---------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h | --help)
            usage
            exit 0
            ;;
        --force)
            FORCE=1
            shift
            ;;
        --local)
            MODE="local"
            shift
            ;;
        --)
            shift
            ;;
        -*)
            echo "Error: unknown option '$1'" >&2
            usage >&2
            exit 2
            ;;
        *)
            MODE="file"
            UF2_ARG="$1"
            shift
            ;;
    esac
done

echo "=============================================="
echo "   TFF RP2040 Firmware Deployment Tool       "
echo "=============================================="

# --- Version helpers ----------------------------------------------------------

# Strip a leading "v" and surrounding whitespace from a version string.
normalize_version() {
    local v="$1"
    v="${v#v}"
    v="${v//[[:space:]]/}"
    printf '%s' "$v"
}

# Latest release version from GitHub (prefers gh, falls back to the HTTP redirect).
get_latest_version() {
    local tag=""
    if command -v gh &>/dev/null; then
        tag="$(gh release view --repo "${REPO}" --json tagName -q .tagName 2>/dev/null || true)"
    fi
    if [[ -z "${tag}" ]] && command -v curl &>/dev/null; then
        # The /releases/latest page redirects to /releases/tag/<tag>.
        local url
        url="$(curl -fsSLI -o /dev/null -w '%{url_effective}' \
            "https://github.com/${REPO}/releases/latest" 2>/dev/null || true)"
        tag="${url##*/tag/}"
        [[ "${tag}" == "${url}" ]] && tag=""  # no redirect captured
    fi
    [[ -n "${tag}" ]] || return 1
    normalize_version "${tag}"
}

# Version currently running on the board, via the USB bcdDevice descriptor
# (readable from sysfs without opening the serial port or grabbing the keyboard).
# bcdDevice layout (must match src/platform/rp2040/usb_descriptors.c):
#   nibble [15:12] major, [11:8] minor, byte [7:0] patch  ->  2.0.3 == 0x2003.
detect_version_bcd() {
    local d vid pid bcd
    for d in /sys/bus/usb/devices/*/; do
        [[ -f "${d}idVendor" ]] || continue
        vid="$(cat "${d}idVendor" 2>/dev/null || true)"
        pid="$(cat "${d}idProduct" 2>/dev/null || true)"
        if [[ "${vid}" == "${USB_VID}" && "${pid}" == "${USB_PID}" ]]; then
            bcd="$(cat "${d}bcdDevice" 2>/dev/null || true)"
            [[ "${bcd}" =~ ^[0-9a-fA-F]{4}$ ]] || return 1
            printf '%d.%d.%d' "$((16#${bcd:0:1}))" "$((16#${bcd:1:1}))" "$((16#${bcd:2:2}))"
            return 0
        fi
    done
    return 1
}

# Persistent by-id path of the TFF board's CDC serial console, if present. We
# only ever touch the board's own port, never unrelated /dev/ttyACM* devices.
cdc_port() {
    local p
    for p in /dev/serial/by-id/*TFF*RP2040* /dev/serial/by-id/*Flying_Fingers*; do
        [[ -e "${p}" ]] && { printf '%s' "${p}"; return 0; }
    done
    return 1
}

# Query the running firmware over the CDC serial console ("version" command).
# Returns the reported version, or fails if the port is absent / not accessible
# (e.g. the user is not in the 'dialout' group) or the firmware did not answer.
detect_version_cdc() {
    local port resp ver line deadline
    port="$(cdc_port)" || return 1
    [[ -r "${port}" && -w "${port}" ]] || return 1
    # One read/write fd; -hupcl keeps DTR asserted across the exchange so the
    # firmware's reply is not cut off by a hangup when the port is opened.
    exec 3<>"${port}" 2>/dev/null || return 1
    command -v stty &>/dev/null && stty -F "${port}" 115200 raw -echo -hupcl &>/dev/null || true
    printf 'version\r\n' >&3 2>/dev/null || true
    resp=""
    deadline=$((SECONDS + 3))
    while ((SECONDS < deadline)); do
        if IFS= read -r -t 1 -u 3 line; then
            resp+="${line} "
            [[ "${resp}" =~ [0-9]+\.[0-9]+\.[0-9]+ ]] && break
        fi
    done
    exec 3>&- 2>/dev/null || true
    ver="$(printf '%s' "${resp}" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -n1 || true)"
    [[ -n "${ver}" ]] && { printf '%s' "${ver}"; return 0; }
    return 1
}

detect_board_version() {
    detect_version_bcd || detect_version_cdc
}

# After flashing, wait for the board to leave the bootloader and re-enumerate as
# the running keyboard, then confirm the version it now reports via BOTH the USB
# bcdDevice descriptor and the CDC "version" command. "${1}" is the expected
# version (empty when flashing a local/explicit UF2 whose version is unknown).
verify_after_flash() {
    local expected="${1:-}"
    local bcd_ver="" cdc_ver="" appeared=0 i

    echo
    echo "Waiting for the board to reboot and re-enumerate..."

    # 1. Wait (up to ~10s) for the bootloader drive to disappear.
    for ((i = 0; i < 20; i++)); do
        [[ -e "${BOOTSEL_LABEL}" ]] || break
        sleep 0.5
    done

    # 2. Wait (up to ~20s) for the TFF keyboard to re-appear running new firmware.
    for ((i = 0; i < 40; i++)); do
        if bcd_ver="$(detect_version_bcd)"; then
            appeared=1
            break
        fi
        sleep 0.5
    done

    if [[ "${appeared}" -ne 1 ]]; then
        echo "⚠ Board did not re-enumerate within ~20s. It may still be booting."
        echo "  Once its keyboard is active it is safe to unplug; verify with: tff --list"
        return 0
    fi

    # The CDC "version" command is a supplementary check. Distinguish a real
    # mismatch from "could not read it" (port missing, or no serial access).
    local cdc_note="" port
    if ! port="$(cdc_port)"; then
        cdc_note="serial port not found"
    elif [[ ! -r "${port}" || ! -w "${port}" ]]; then
        cdc_note="no access to ${port} — add your user to the 'dialout' group: sudo usermod -aG dialout \"\$USER\" (then re-login)"
    else
        # Give the CDC interface a moment to come up, then query it.
        for ((i = 0; i < 6; i++)); do
            cdc_ver="$(detect_version_cdc || true)"
            [[ -n "${cdc_ver}" ]] && break
            sleep 0.5
        done
        [[ -z "${cdc_ver}" ]] && cdc_note="no response from firmware"
    fi

    echo "✓ Board re-enumerated. Reported firmware version:"
    printf '    USB bcdDevice : %s\n' "${bcd_ver:-<unavailable>}"
    if [[ -n "${cdc_ver}" ]]; then
        printf "    CDC 'version' : %s\n" "${cdc_ver}"
    else
        printf "    CDC 'version' : <%s>\n" "${cdc_note:-no response}"
    fi

    if [[ -n "${expected}" ]]; then
        if [[ "${bcd_ver}" == "${expected}" && "${cdc_ver}" == "${expected}" ]]; then
            echo "✓ Verified ${expected}: confirmed via both the USB descriptor and the CDC serial."
        elif [[ "${bcd_ver}" == "${expected}" && -z "${cdc_ver}" ]]; then
            # bcdDevice is read straight from the running device's USB descriptor,
            # so it alone is sufficient confirmation; CDC is a bonus.
            echo "✓ Verified ${expected} via the USB descriptor. (CDC not checked: ${cdc_note}.)"
        elif [[ "${bcd_ver}" == "${expected}" || "${cdc_ver}" == "${expected}" ]]; then
            echo "⚠ Channels disagree — expected ${expected}, got bcdDevice=${bcd_ver:-?}, CDC=${cdc_ver:-?}."
        else
            echo "⚠ Could not confirm ${expected} (bcdDevice=${bcd_ver:-?}, CDC=${cdc_ver:-?})."
            echo "  The flashed firmware may predate version reporting, or the flash did not take."
        fi
    fi

    echo
    echo "✅ Done — you can now safely unplug the device."
}

# --- Resolve which UF2 to flash ----------------------------------------------
EXPECTED_VERSION=""
TMP_DIR=""
cleanup() { [[ -n "${TMP_DIR}" && -d "${TMP_DIR}" ]] && rm -rf "${TMP_DIR}"; }
trap cleanup EXIT

case "${MODE}" in
    file)
        UF2_PATH="${UF2_ARG}"
        if [[ ! -f "${UF2_PATH}" ]]; then
            echo "Error: Firmware binary not found at: ${UF2_PATH}" >&2
            exit 1
        fi
        ;;

    local)
        UF2_PATH="${DEFAULT_UF2}"
        if [[ ! -f "${UF2_PATH}" ]]; then
            echo "Error: Local firmware not found at: ${UF2_PATH}" >&2
            echo "Build it first with: ./build_rp2040.sh" >&2
            exit 1
        fi
        ;;

    auto)
        echo "Checking latest release on GitHub (${REPO})..."
        if ! LATEST="$(get_latest_version)"; then
            echo "Error: could not determine the latest release version." >&2
            echo "Check your network, or pass a UF2 explicitly: $0 path/to/tff_rp2040.uf2" >&2
            exit 1
        fi
        echo "Latest release:   ${LATEST}"
        EXPECTED_VERSION="${LATEST}"

        if BOARD="$(detect_board_version)"; then
            echo "Board firmware:   ${BOARD}"
            if [[ "${BOARD}" == "${LATEST}" && "${FORCE}" -ne 1 ]]; then
                echo
                echo "✓ Board is already running the latest version (${LATEST}). Nothing to do."
                exit 0
            fi
            if [[ "${BOARD}" == "${LATEST}" ]]; then
                echo "Board is current, but --force was given; re-flashing ${LATEST}."
            else
                echo "Update available: ${BOARD} -> ${LATEST}."
            fi
        else
            echo "Board firmware:   unknown (board not detected, in bootloader, or pre-version firmware)"
            echo "Proceeding to flash the latest release (${LATEST})."
        fi

        echo "Downloading ${UF2_ASSET} (${LATEST})..."
        TMP_DIR="$(mktemp -d)"
        UF2_PATH="${TMP_DIR}/${UF2_ASSET}"
        if ! curl -fL "https://github.com/${REPO}/releases/latest/download/${UF2_ASSET}" \
            -o "${UF2_PATH}"; then
            echo "Error: failed to download ${UF2_ASSET} from the latest release." >&2
            exit 1
        fi
        ;;
esac

echo "Target firmware: ${UF2_PATH}"

# --- Flashing -----------------------------------------------------------------
get_mount_point() {
    local label_dev="$1"
    local default_target="$2"

    # Check if already mounted
    local existing
    existing="$(findmnt -n -o TARGET "${label_dev}" 2>/dev/null || true)"
    if [[ -n "${existing}" && -d "${existing}" ]]; then
        echo "${existing}"
        return 0
    fi

    # Mount via udisksctl (unprivileged user friendly) if available
    if command -v udisksctl &>/dev/null; then
        local udisks_out
        if udisks_out="$(udisksctl mount -b "${label_dev}" 2>/dev/null)"; then
            existing="$(echo "${udisks_out}" | grep -o '/.*' || true)"
            if [[ -n "${existing}" && -d "${existing}" ]]; then
                echo "${existing}"
                return 0
            fi
        fi
    fi

    # Direct mount
    mkdir -p "${default_target}"
    mount "${label_dev}" "${default_target}"
    echo "${default_target}"
}

# Check if currently running and needs automated reboot into bootloader
if [[ ! -e "${BOOTSEL_LABEL}" ]]; then
    if compgen -G "/dev/ttyACM*" >/dev/null || compgen -G "/dev/ttyUSB*" >/dev/null; then
        echo "RP2040 detected on USB serial. Triggering automated reboot into bootloader..."
        python3 "${SCRIPT_DIR}/reboot_rp2040_bootloader.py"
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
echo "✓ UF2 firmware copied. Board is rebooting into the new firmware..."

verify_after_flash "${EXPECTED_VERSION}"
exit 0
