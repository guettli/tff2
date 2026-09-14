# Linux Installation & Systemd Guide for Ten Flying Fingers (TFF)

This guide explains how to install and run **Ten Flying Fingers (TFF)** as a background service on Linux with automatic startup, keyboard device management, and combo configuration.

---

## 1. Quick Installation (Automated)

The easiest way to install TFF is using the provided [`install.sh`](file:///home/tff2/tff2/install.sh) script.

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

## 2. Managing the Systemd Service

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

## 3. Keyboard Device Configuration

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

See [`ten-flying-fingers.service.example`](file:///home/tff2/tff2/ten-flying-fingers.service.example):

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

[Install]
WantedBy=default.target
```

### User Service (`~/.config/systemd/user/ten-flying-fingers.service`)

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

[Install]
WantedBy=default.target
```

---

## 5. Linux Permissions & Udev Rules

TFF needs access to:
1. `/dev/uinput` to emit virtual keystrokes.
2. `/dev/input/event*` to read physical keystrokes and acquire exclusive grab (`ioctl(fd, EVIOCGRAB, 1)`).

### Running as Root
When running as a system-wide systemd service (default), root has full access automatically.

### Running as Non-Root User
To run TFF under a regular user account without `sudo`, grant access to the `input` group:

1. Add your user to the `input` group:
   ```bash
   sudo usermod -aG input "$USER"
   ```

2. Create a udev rule for `/dev/uinput` in `/etc/udev/rules.d/99-tff-uinput.rules`:
   ```udev
   KERNEL=="uinput", GROUP="input", MODE="0660", OPTIONS+="static_node=uinput"
   ```

3. Reload udev rules:
   ```bash
   sudo udevadm control --reload-rules && sudo udevadm trigger
   ```

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
