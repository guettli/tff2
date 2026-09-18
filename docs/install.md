# Linux Installation & Systemd Guide for Ten Flying Fingers (TFF)

This guide explains how to install and run **Ten Flying Fingers (TFF)** as a background service on Linux with automatic startup, keyboard device management, and combo configuration.

---

## 1. Binary Installation via mise (Recommended)

If you use [mise-en-place (mise)](https://mise.jdx.dev/), you can install and manage Ten Flying Fingers without compiling from source:

```bash
# Install globally via GitHub backend:
mise use -g github:guettli/tff2

# Or install globally via ubi backend:
mise use -g ubi:guettli/tff2
```

`mise` automatically downloads the pre-built, statically-linked Linux release binary for your architecture and places `tff`, `tff2`, and `tff_linux` on your `PATH`.

To update to the latest release at any time:
```bash
mise upgrade github:guettli/tff2
```

---

## 2. Pre-Built Binary Download (Manual)

Pre-built binaries with statically linked C++ runtimes (`-static-libgcc -static-libstdc++`) are published on [GitHub Releases](https://github.com/guettli/tff2/releases):

```bash
# Download and extract the latest Linux x86_64 archive
curl -LO https://github.com/guettli/tff2/releases/latest/download/tff2_Linux_x86_64.tar.gz
tar -xzf tff2_Linux_x86_64.tar.gz

# Install binary to ~/.local/bin or /usr/local/bin
install -m 0755 tff ~/.local/bin/tff
install -m 0755 tff_linux ~/.local/bin/tff_linux

# Install default configuration
mkdir -p ~/.config/tff
cp tff-combos.yaml ~/.config/tff/tff-combos.yaml
```

---

## 3. Quick Installation from Source (install.sh)

If building from source in a cloned repository, use the provided [`install.sh`](../install.sh) script.

### System-Wide Installation (Recommended for multi-user / root)

Installs the `tff` binary to `/usr/local/bin`, installs default configuration to `/etc/tff/tff-combos.yaml`, and registers a system-wide systemd service:

```bash
sudo ./install.sh
```

### User-Level Installation (No root required)

Installs the `tff` binary to `~/.local/bin/tff`, configuration to `~/.config/tff/tff-combos.yaml`, and registers a user systemd service:

```bash
./install.sh --user
```

*Note: If run without `sudo` on a machine without passwordless sudo, the script automatically defaults to user-level mode.*

---

## 4. Shell Autocompletions (Bash, Zsh, Fish)

When running `install.sh`, shell autocompletions for `bash`, `zsh`, and `fish` are automatically installed into their respective completion directories:
- **System-wide (`sudo ./install.sh`)**:
  - Bash: `/usr/share/bash-completion/completions/tff` (with `tff2` and `tff_linux` symlinks)
  - Zsh: `/usr/share/zsh/site-functions/_tff` (with `_tff2` and `_tff_linux` symlinks)
  - Fish: `/usr/share/fish/vendor_completions.d/tff.fish`
- **User-level (`./install.sh --user`)**:
  - Bash: `~/.local/share/bash-completion/completions/tff`
  - Zsh: `~/.local/share/zsh/site-functions/_tff`
  - Fish: `~/.config/fish/completions/tff.fish`

### Manual Completion Activation

If you installed manually or via `mise`, you can activate completions directly:

**Bash:**
```bash
source completions/bash/tff
```

**Zsh:**
```zsh
# Add the completions directory to your fpath in ~/.zshrc before compinit:
fpath=(/path/to/tff2/completions/zsh $fpath)
autoload -Uz compinit && compinit
```

**Fish:**
```fish
cp completions/fish/tff.fish ~/.config/fish/completions/
```

---

## 5. Managing the Systemd Service

### Service Control Commands

| Action | System-Wide Mode (`sudo`) | User Mode |
| :--- | :--- | :--- |
| **Start Service** | `sudo systemctl start ten-flying-fingers` | `systemctl --user start ten-flying-fingers` |
| **Stop Service** | `sudo systemctl stop ten-flying-fingers` | `systemctl --user stop ten-flying-fingers` |
| **Restart Service** | `sudo systemctl restart ten-flying-fingers` | `systemctl --user restart ten-flying-fingers` |
| **Reload Config (SIGHUP)** | `sudo systemctl reload ten-flying-fingers` | `systemctl --user reload ten-flying-fingers` |
| **Check Status** | `sudo systemctl status ten-flying-fingers` | `systemctl --user status ten-flying-fingers` |
| **Enable Auto-Start** | `sudo systemctl enable ten-flying-fingers` | `systemctl --user enable ten-flying-fingers` |
| **Disable Auto-Start**| `sudo systemctl disable ten-flying-fingers` | `systemctl --user disable ten-flying-fingers` |
| **View Live Logs** | `sudo journalctl -u ten-flying-fingers -f` | `journalctl --user -u ten-flying-fingers -f` |

---

## 6. Keyboard Device Configuration

### Step 1: Discover Connected Keyboards

Run `tff --list` (or `tff list`) to view all detected keyboards and their persistent device paths:

```bash
tff --list
```

Example output:

```text
Scanning for keyboards in /dev/input/...
Found 1 keyboard device(s):

  [1] /dev/input/event8
      Name:  Adafruit Feather RP2040 USB Host Keyboard
      Alias: /dev/input/by-id/usb-Adafruit_Feather_RP2040_USB_Host_DF6544F3CF741134-if03-event-kbd

Hint: In systemd service files, use the persistent Alias path to survive reboots/replugs.
```

### Step 2: Auto-Discovery vs Explicit Devices

TFF supports two modes of keyboard detection:

1. **Auto-Discovery (Default)**:
   If no keyboard devices are specified, TFF automatically scans `/dev/input/event*`, filters out non-keyboards (mice, power buttons, video bus), and grabs all detected keyboards.

   ```ini
   ExecStart=/usr/local/bin/tff --config /etc/tff/tff-combos.yaml
   ```

2. **Explicit Persistent Symlinks (Recommended for multi-keyboard setups)**:
   Specify the persistent symlink from `/dev/input/by-id/` or `/dev/input/by-path/`. This ensures the service binds to the exact keyboard even if USB ports are swapped or event numbers change across reboots:

   ```ini
   ExecStart=/usr/local/bin/tff --config /etc/tff/tff-combos.yaml /dev/input/by-id/usb-Adafruit_Feather_RP2040_USB_Host_DF6544F3CF741134-if03-event-kbd
   ```

---

## 4. Systemd Service File Examples

### System-Wide Service (`/etc/systemd/system/ten-flying-fingers.service`)

See [`ten-flying-fingers.service.example`](../ten-flying-fingers.service.example):

```ini
[Unit]
Description=Ten Flying Fingers (TFF) Keyboard Remapper
Documentation=https://github.com/guettli/tff2
After=multi-user.target
Wants=systemd-udev-settle.service

[Service]
Type=simple
Restart=always
RestartSec=3

# Highest CPU scheduling priority to minimize keyboard input latency
Nice=-20

# Run with auto-discovery or explicit device list
ExecStart=/usr/local/bin/tff --config /etc/tff/tff-combos.yaml
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=default.target
```

### User Service (`~/.config/systemd/user/ten-flying-fingers.service`)

If installed via `./install.sh --user` or manual binary download:
```ini
[Unit]
Description=Ten Flying Fingers (TFF) Keyboard Remapper
Documentation=https://github.com/guettli/tff2
After=default.target

[Service]
Type=simple
Restart=always
RestartSec=3
Nice=-20
ExecStart=%h/.local/bin/tff --config %h/.config/tff/tff-combos.yaml
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=default.target
```

If installed via **`mise`**, use the mise shim or `mise exec`:
```ini
[Unit]
Description=Ten Flying Fingers (TFF) Keyboard Remapper
Documentation=https://github.com/guettli/tff2
After=default.target

[Service]
Type=simple
Restart=always
RestartSec=3
Nice=-20
# Option A: using mise shim path
ExecStart=%h/.local/share/mise/shims/tff --config %h/.config/tff/tff-combos.yaml
# Option B (alternative): using mise exec
# ExecStart=%h/.local/bin/mise exec -- tff --config %h/.config/tff/tff-combos.yaml
ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=default.target
```

---

## 5. Linux Permissions & Udev Rules

TFF needs access to:
1. `/dev/uinput` to emit virtual keystrokes and mouse movements.
2. `/dev/input/event*` to read physical keystrokes and acquire exclusive grab (`ioctl(fd, EVIOCGRAB, 1)`).

### Automatic Setup (Recommended)

TFF provides a built-in helper to diagnose permissions and install udev rules:

1. **Check your current permissions and diagnostics**:
   ```bash
   tff setup-udev
   ```
   This inspects group membership, `/dev/uinput`, `/dev/input/event*` device access, and existing udev rules. If anything is missing, it provides clear, step-by-step instructions.

2. **Automatically install udev rules and reload**:
   ```bash
   sudo tff setup-udev --install
   ```
   This automatically installs the required udev rules to `/etc/udev/rules.d/99-tff.rules`, configures `/etc/modules-load.d/uinput.conf`, and reloads rules with `udevadm`.

3. **Inspect the generated udev rules**:
   ```bash
   tff setup-udev --print
   ```

### Running as Root
When running as a system-wide systemd service (default), root has full access automatically.

### Running as Non-Root User (Manual Setup)
If you prefer manual setup instead of `tff setup-udev --install`:

1. Add your user to the `input` group:
   ```bash
   sudo usermod -aG input "$USER"
   ```

2. Create `/etc/udev/rules.d/99-tff.rules`:
   ```udev
   # /dev/uinput: virtual keyboard and mouse event emission
   KERNEL=="uinput", SUBSYSTEM=="misc", TAG+="uaccess", OPTIONS+="static_node=uinput", MODE="0660", GROUP="input"

   # /dev/input/event*: physical keyboard event grabbing
   KERNEL=="event*", SUBSYSTEM=="input", MODE="0660", GROUP="input"
   ```

3. Reload udev rules:
   ```bash
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

4. Activate your new group membership in your current shell:
   ```bash
   newgrp input
   ```
   *(Or log out and log back in to your desktop session).*

---

## 6. Validating and Customizing Combos

### Validate Configuration

Before applying changes, validate your YAML combo file:

```bash
tff validate /etc/tff/tff-combos.yaml
```

Output:
```text
Configuration is valid! Loaded 13 combo(s) from /etc/tff/tff-combos.yaml
```

### Apply Changes

After editing your combos file, restart the service to apply changes immediately:

```bash
# System service
sudo systemctl restart ten-flying-fingers

# User service
systemctl --user restart ten-flying-fingers
```

---

## 7. Troubleshooting

- **Check if keyboard is grabbed**:
  Run `tff --list` to verify device paths.
- **Inspect live logs**:
  `sudo journalctl -u ten-flying-fingers -f`
- **Run manually in verbose mode**:
  Stop the background service and run in a terminal to see real-time key events:
  ```bash
  tff --config /etc/tff/tff-combos.yaml --verbose
  ```
- **Clean exit & keyboard restore**:
  TFF intercepts `SIGINT` (Ctrl+C) and `SIGTERM`. When stopped, it automatically ungrabs all physical keyboards (`ioctl(fd, EVIOCGRAB, 0)`) and destroys the virtual device, ensuring you never get locked out.
