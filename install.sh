#!/usr/bin/env bash
# Installation script for Ten Flying Fingers (TFF) on Linux
# Supports both system-wide (/usr/local, /etc) and user-level (~/.local, ~/.config) installation.

set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

MODE="system"
if [[ "${1:-}" == "--user" ]]; then
    MODE="user"
elif [[ $EUID -ne 0 ]]; then
    if ! sudo -n true 2>/dev/null; then
        echo "Note: Running as non-root without passwordless sudo."
        echo "Defaulting to user-level installation (--user)."
        echo "To install system-wide, run: sudo ./install.sh"
        echo ""
        MODE="user"
    fi
fi

echo "=================================================="
echo "  Installing Ten Flying Fingers (TFF) on Linux    "
echo "  Mode: ${MODE}                                   "
echo "=================================================="

# 1. Build and verify test suite
echo "[1/6] Building release binary and executing tests..."
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$(nproc)"
./test_linux_platform
cd "${SCRIPT_DIR}"

# 2. Setup install paths
if [[ "${MODE}" == "user" ]]; then
    XDG_DATA_HOME="${XDG_DATA_HOME:-${HOME}/.local/share}"
    XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-${HOME}/.config}"
    BIN_DIR="${HOME}/.local/bin"
    CONFIG_DIR="${XDG_CONFIG_HOME}/tff"
    SYSTEMD_DIR="${XDG_CONFIG_HOME}/systemd/user"
    MAN_DIR="${XDG_DATA_HOME}/man/man1"
    BASH_COMPLETION_DIR="${XDG_DATA_HOME}/bash-completion/completions"
    ZSH_COMPLETION_DIR="${XDG_DATA_HOME}/zsh/site-functions"
    FISH_COMPLETION_DIR="${XDG_CONFIG_HOME}/fish/completions"
    SUDO=""
else
    BIN_DIR="/usr/local/bin"
    CONFIG_DIR="/etc/tff"
    SYSTEMD_DIR="/etc/systemd/system"
    MAN_DIR="/usr/local/share/man/man1"
    BASH_COMPLETION_DIR="/usr/share/bash-completion/completions"
    ZSH_COMPLETION_DIR="/usr/share/zsh/site-functions"
    FISH_COMPLETION_DIR="/usr/share/fish/vendor_completions.d"
    SUDO="sudo"
fi

# 3. Install binary and symlink
echo "[2/6] Installing binary to ${BIN_DIR}/tff..."
${SUDO} install -d "${BIN_DIR}"
${SUDO} install -m 755 build/tff_linux "${BIN_DIR}/tff_linux"
${SUDO} ln -sf "${BIN_DIR}/tff_linux" "${BIN_DIR}/tff"

# 4. Install default configuration
echo "[3/6] Installing configuration to ${CONFIG_DIR}/tff-combos.yaml..."
${SUDO} install -d "${CONFIG_DIR}"
if [[ ! -f "${CONFIG_DIR}/tff-combos.yaml" ]]; then
    ${SUDO} install -m 644 config/tff-combos.yaml "${CONFIG_DIR}/tff-combos.yaml"
    echo "  Created ${CONFIG_DIR}/tff-combos.yaml"
else
    echo "  ${CONFIG_DIR}/tff-combos.yaml already exists (preserving current configuration)"
fi

# 5. Install Unix man page
echo "[4/6] Installing man page to ${MAN_DIR}/tff.1..."
if [[ -f "docs/man/tff.1" ]]; then
    ${SUDO} install -d "${MAN_DIR}"
    ${SUDO} install -m 644 docs/man/tff.1 "${MAN_DIR}/tff.1"
    ${SUDO} ln -sf "tff.1" "${MAN_DIR}/tff2.1"
    ${SUDO} ln -sf "tff.1" "${MAN_DIR}/tff_linux.1"
    echo "  Installed man page and symlinks (tff.1, tff2.1, tff_linux.1)"
fi

# 6. Install shell autocompletions
echo "[5/6] Installing shell autocompletions (bash, zsh, fish)..."
if [[ -f "completions/bash/tff" ]]; then
    ${SUDO} install -d "${BASH_COMPLETION_DIR}"
    ${SUDO} install -m 644 completions/bash/tff "${BASH_COMPLETION_DIR}/tff"
    ${SUDO} ln -sf "tff" "${BASH_COMPLETION_DIR}/tff2"
    ${SUDO} ln -sf "tff" "${BASH_COMPLETION_DIR}/tff_linux"
    echo "  Installed Bash completions (tff, tff2, tff_linux)"
fi
if [[ -f "completions/zsh/_tff" ]]; then
    ${SUDO} install -d "${ZSH_COMPLETION_DIR}"
    ${SUDO} install -m 644 completions/zsh/_tff "${ZSH_COMPLETION_DIR}/_tff"
    ${SUDO} ln -sf "_tff" "${ZSH_COMPLETION_DIR}/_tff2"
    ${SUDO} ln -sf "_tff" "${ZSH_COMPLETION_DIR}/_tff_linux"
    echo "  Installed Zsh completions (_tff, _tff2, _tff_linux)"
fi
if [[ -f "completions/fish/tff.fish" ]]; then
    ${SUDO} install -d "${FISH_COMPLETION_DIR}"
    ${SUDO} install -m 644 completions/fish/tff.fish "${FISH_COMPLETION_DIR}/tff.fish"
    ${SUDO} ln -sf "tff.fish" "${FISH_COMPLETION_DIR}/tff2.fish"
    ${SUDO} ln -sf "tff.fish" "${FISH_COMPLETION_DIR}/tff_linux.fish"
    echo "  Installed Fish completions (tff.fish, tff2.fish, tff_linux.fish)"
fi

# 7. Configure systemd service
echo "[6/6] Configuring systemd service..."
${SUDO} install -d "${SYSTEMD_DIR}"
SERVICE_DEST="${SYSTEMD_DIR}/ten-flying-fingers.service"

# Generate unit file with accurate binary and config paths
cat << EOF | ${SUDO} tee "${SERVICE_DEST}" > /dev/null
[Unit]
Description=Ten Flying Fingers (TFF) Keyboard Remapper
Documentation=https://github.com/guettli/tff2
After=multi-user.target
Wants=systemd-udev-settle.service

[Service]
Type=simple
Restart=always
RestartSec=3
Nice=-20
ExecStart=${BIN_DIR}/tff --config ${CONFIG_DIR}/tff-combos.yaml
ExecReload=/bin/kill -HUP \$MAINPID

[Install]
WantedBy=default.target
EOF

if [[ "${MODE}" == "user" ]]; then
    systemctl --user daemon-reload
    systemctl --user enable ten-flying-fingers.service
    systemctl --user restart ten-flying-fingers.service
    echo ""
    systemctl --user status ten-flying-fingers.service --no-pager || true
else
    ${SUDO} systemctl daemon-reload
    ${SUDO} systemctl enable ten-flying-fingers.service
    ${SUDO} systemctl restart ten-flying-fingers.service
    echo ""
    ${SUDO} systemctl status ten-flying-fingers.service --no-pager || true
fi

echo ""
echo "=================================================="
echo "Installation complete! TFF is installed and active."
echo "=================================================="
if [[ "${MODE}" == "user" ]]; then
    echo "Check status:  systemctl --user status ten-flying-fingers"
    echo "View logs:     journalctl --user -u ten-flying-fingers -f"
    echo "Stop service:  systemctl --user stop ten-flying-fingers"
    echo "Permissions:   tff setup-udev (run 'sudo tff setup-udev --install' for udev rules)"
else
    echo "Check status:  sudo systemctl status ten-flying-fingers"
    echo "View logs:     sudo journalctl -u ten-flying-fingers -f"
    echo "Stop service:  sudo systemctl stop ten-flying-fingers"
fi
echo "Run manually:  tff --help"
echo "=================================================="
