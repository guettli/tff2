#!/bin/bash
# Sets up the UpBoard as self-hosted GitHub Actions runner for .github/workflows/hardware.yml.
# Run as root on the UpBoard, from a checkout of this repository:
#
#   sudo scripts/hw-runner/setup.sh <registration-token>
#
# Get the token in GitHub: Settings → Actions → Runners → New self-hosted runner, or
#   gh api -X POST repos/guettli/tff2/actions/runners/registration-token --jq .token
# Without a token, everything except the runner registration is installed (safe to re-run).
set -euo pipefail

REPO_URL="https://github.com/guettli/tff2"
RUNNER_USER="gh-runner"
RUNNER_DIR="/opt/actions-runner"
LABEL="tff-rp2040-hw"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
TOKEN="${1:-}"

[ "$(id -u)" -eq 0 ] || { echo "run as root" >&2; exit 1; }

echo "==> packages"
apt-get install -y --no-install-recommends \
    curl git cmake python3 python3-serial uhubctl \
    gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib >/dev/null

echo "==> user ${RUNNER_USER} (groups: dialout, input)"
id "$RUNNER_USER" >/dev/null 2>&1 || useradd --create-home --shell /bin/bash "$RUNNER_USER"
usermod -aG dialout,input "$RUNNER_USER"

echo "==> root helpers"
install -m 0755 "$HERE/tff-hw" /usr/local/sbin/tff-hw
install -m 0755 "$ROOT/setup_fake_keyboard.sh" /usr/local/sbin/tff-setup-fake-keyboard
cat > /etc/sudoers.d/tff-hw <<SUDO
# The hardware CI runner may flash and power-cycle the RP2040, nothing else.
${RUNNER_USER} ALL=(root) NOPASSWD: /usr/local/sbin/tff-hw
SUDO
chmod 0440 /etc/sudoers.d/tff-hw
visudo -cf /etc/sudoers.d/tff-hw >/dev/null

echo "==> udev: /dev/hidg* for group input"
# The kernel creates /dev/hidg0 again (mode 0600) whenever the gadget is re-attached.
cat > /etc/udev/rules.d/70-tff-hidg.rules <<RULE
KERNEL=="hidg[0-9]*", GROUP="input", MODE="0660"
RULE
udevadm control --reload
udevadm trigger --subsystem-match=hidg 2>/dev/null || true

echo "==> fake keyboard gadget at boot"
cat > /etc/systemd/system/tff-fake-keyboard.service <<UNIT
[Unit]
Description=TFF test rig: USB-OTG fake keyboard gadget (/dev/hidg0)
After=systemd-modules-load.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/local/sbin/tff-setup-fake-keyboard

[Install]
WantedBy=multi-user.target
UNIT
systemctl daemon-reload
systemctl enable --now tff-fake-keyboard.service >/dev/null

echo "==> GitHub Actions runner in ${RUNNER_DIR}"
if [ ! -x "$RUNNER_DIR/config.sh" ]; then
    version="$(curl -fsSL https://api.github.com/repos/actions/runner/releases/latest |
        python3 -c 'import json, sys; print(json.load(sys.stdin)["tag_name"].lstrip("v"))')"
    mkdir -p "$RUNNER_DIR"
    curl -fsSL "https://github.com/actions/runner/releases/download/v${version}/actions-runner-linux-x64-${version}.tar.gz" |
        tar -xz -C "$RUNNER_DIR"
    chown -R "$RUNNER_USER:" "$RUNNER_DIR"
fi

if [ -f "$RUNNER_DIR/.runner" ]; then
    echo "runner already registered"
elif [ -n "$TOKEN" ]; then
    sudo -u "$RUNNER_USER" "$RUNNER_DIR/config.sh" --unattended --url "$REPO_URL" --token "$TOKEN" \
        --name "$(hostname)" --labels "$LABEL" --work _work
    (cd "$RUNNER_DIR" && ./svc.sh install "$RUNNER_USER" && ./svc.sh start)
else
    echo "no registration token given: runner not registered yet"
fi
echo "done. Check: sudo /usr/local/sbin/tff-hw status"
