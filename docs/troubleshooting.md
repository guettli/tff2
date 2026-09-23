# TFF2 Troubleshooting & Diagnostic Guide

This guide helps diagnose and resolve common issues encountered when running Ten Flying Fingers (TFF) on Linux (userspace daemon) or Raspberry Pi Pico RP2040 (hardware interceptor).

---

## Quick Diagnostic Checklist

If keyboard remapping is not behaving as expected, run these four diagnostic commands:

```bash
# 1. Check permissions and udev setup
tff setup-udev

# 2. List all detected keyboards and their event paths
tff --list

# 3. Validate your YAML configuration file for syntax/semantic errors
tff validate /etc/tff/tff-combos.yaml

# 4. Run the interactive live event monitor to inspect key events in real time
tff monitor
```

---

## 1. Linux Permissions & Device Access

### Symptom: `Failed to open /dev/uinput: Permission denied` or `Failed to grab /dev/input/event*: Permission denied`

**Cause:**  
By default on many Linux distributions, `/dev/uinput` and `/dev/input/event*` devices are restricted to the `root` user or members of the `input` group.

**Resolution:**
1. Run the automated udev rule installer:
   ```bash
   sudo tff setup-udev --install
   ```
   This generates and installs `/etc/udev/rules.d/99-tff.rules` granting read/write access to `/dev/uinput` and `/dev/input/event*` for users in the `input` group, and triggers `udevadm control --reload-rules && udevadm trigger`.

2. Ensure your user account is in the `input` group:
   ```bash
   sudo usermod -aG input "$USER"
   ```

3. Log out and log back in (or run `newgrp input` in your current terminal) for group membership to take effect.

4. Verify device permissions:
   ```bash
   ls -l /dev/uinput
   # Output should show: crw-rw---- 1 root input ... /dev/uinput
   ```

---

## 2. Device Grabbing Conflicts (`EBUSY`)

### Symptom: `Failed to grab device /dev/input/eventX: Device or resource busy`

**Cause:**  
Linux `EVIOCGRAB` grants an exclusive lock on an input device. If another remapping daemon (e.g. `kanata`, `kmonad`, `interception-tools`, or another running instance of `tff`) has already grabbed the keyboard, the kernel returns `EBUSY` (error code 16).

**Resolution:**
1. Check if another instance of `tff` or a systemd service is already active:
   ```bash
   ps aux | grep -E "tff|kanata|kmonad|intercept"
   systemctl is-active ten-flying-fingers.service
   systemctl --user is-active ten-flying-fingers.service
   ```

2. Identify which process has the event node open:
   ```bash
   sudo fuser /dev/input/event*
   # or
   sudo lsof /dev/input/event*
   ```

3. Stop the conflicting service or process before starting `tff`:
   ```bash
   sudo systemctl stop conflicting-service
   ```

4. If you intentionally want to run without exclusive grabs (for snooping or debugging), use `--no-grab`:
   ```bash
   tff --no-grab config/tff-combos.yaml
   ```

---

## 3. Wayland and Desktop Environment Specifics

### How TFF Interacts with Wayland (GNOME, KDE Plasma, Sway, Hyprland)
TFF operates at the Linux kernel `evdev` and `uinput` layer, **below** Wayland compositors and X11 servers. It captures raw hardware scan codes before the compositor sees them, and injects remapped keystrokes into a virtual `/dev/uinput` keyboard.

- **Compositor Compatibility:** TFF works identically across Wayland (GNOME Mutter, KDE KWin, Sway, Hyprland) and X11 sessions.
- **Screen Lockers:** When the screen locks, Wayland compositors continue reading from the virtual `/dev/uinput` device. Remappings and chords remain functional for password entry.
- **Rootless Execution:** Wayland compositors never run as root. Following the udev setup in [Section 1](#1-linux-permissions--device-access) enables running TFF entirely as your regular user under systemd user services.

---

## 4. Stuck Keys & Modifier Recovery

### Symptom: A modifier key (Ctrl, Shift, Alt, or Super) acts as if it is held down after closing TFF or switching windows.

**Safeguards Built into TFF:**
- **Emergency State Machine Reset (Hold ESC for 3 Seconds)**: If you are locked in an unexpected modal layer, locked toggle layer (`toggle_layer`), or stuck state, press and hold `ESC` continuously for 3.0 seconds. TFF will trigger a full state machine reset: clearing all modal layers, releasing held tap-holds, one-shots, auto-shift states, and leader buffers, and cleanly swallowing the `ESC` key release. This safeguard is always active without needing configuration.
- **Clean Signal Termination**: TFF registers signal handlers for `SIGINT` and `SIGTERM`. When stopped cleanly (`Ctrl+C` or `systemctl stop`), the engine executes `finish()` and `reset()`, automatically releasing any active tap-hold modifiers or chorded keys and releasing all `EVIOCGRAB` locks.

**If Process was Forcefully Killed (`kill -9`):**
1. Press and release the physical modifier keys on your keyboard:
   - Tap and release Left Shift, Right Shift, Left Ctrl, Right Ctrl, Left Alt, and Super once.
2. Alternatively, use standard input tools to send release events:
   ```bash
   # On Wayland (using ydotool):
   ydotool key 42:0 29:0 56:0 125:0
   # On X11 (using xdotool):
   xdotool keyup Shift_L Shift_R Control_L Control_R Alt_L Super_L
   ```

---

## 5. Hotplugging & Inotify Watch Limits

### Symptom: New keyboards plugged in via USB are not automatically detected

**Diagnosis:**
1. Check if hotplugging was explicitly disabled:
   Verify whether `--no-hotplug` was passed in the systemd service command or CLI invocation. (Hotplugging is enabled by default in TFF).
2. Check if the Linux kernel inotify watch table is exhausted:
   ```bash
   cat /proc/sys/fs/inotify/max_user_watches
   ```
   If development tools or file indexers have exhausted inotify watches, `inotify_add_watch` on `/dev/input` fails with `ENOSPC`.
3. Increase the inotify limit temporarily:
   ```bash
   sudo sysctl fs.inotify.max_user_watches=524288
   ```
   To make it permanent, add `fs.inotify.max_user_watches = 524288` to `/etc/sysctl.d/99-inotify.conf`.

---

## 6. Systemd Service Troubleshooting

### Monitoring Daemon Logs
```bash
# For system-wide service:
sudo journalctl -u ten-flying-fingers.service -f -n 50

# For user session service:
journalctl --user -u ten-flying-fingers.service -f -n 50
```

### Hot-Reloading Configuration Without Dropping Keyboard Grab
You do not need to restart the daemon to apply changes to `tff-combos.yaml`. Send a `SIGHUP` signal (natively invoked by `systemctl reload`):
```bash
# Standard systemd reload:
sudo systemctl reload ten-flying-fingers.service

# Or for user session service:
systemctl --user reload ten-flying-fingers.service

# Or directly via pkill:
pkill -HUP -f tff_linux
```
The daemon logs:
```text
Received SIGHUP, reloading configuration...
Configuration reloaded successfully.
```

---

## 7. RP2040 Hardware Troubleshooting

### Symptom: RP2040 does not appear as a USB drive for flashing
1. Unplug the USB cable from the RP2040.
2. Press and hold down the white **BOOTSEL** button on the Raspberry Pi Pico board.
3. Plug the USB cable back into the computer while continuing to hold BOOTSEL.
4. Release the button after 2 seconds.
5. The board will mount as a mass-storage drive named `RPI-RP2`.
6. Copy `build/tff_rp2040.uf2` to the drive. The Pico will flash and immediately reboot.

### Symptom: Keyboard plugged into RP2040 host port does not respond
1. **USB Host Port vs USB Device Port:**
   - The **USB-A Host connector** (or MAX3421E host breakout) is where your physical keyboard must be plugged in.
   - The **RP2040 micro-USB/USB-C connector** connects to the target PC/laptop.
2. **USB Power Budget:**
   - High-power RGB gaming keyboards may draw more current than the RP2040 5V VBUS pin can supply without external power.
   - *Fix*: Connect the keyboard through a powered USB hub, or disable high-brightness RGB backlighting.
3. **BIOS / Pre-Boot Support:**
   - TFF RP2040 firmware uses standard HID Boot Keyboard protocol descriptors, ensuring full compatibility inside BIOS setup screens and bootloader menus (GRUB, systemd-boot).

---

## Still Having Issues?

- Run `tff monitor` to observe keydown and keyup events in real time.
- Join the discussion and submit issues at [https://github.com/guettli/tff2/issues](https://github.com/guettli/tff2/issues).

---

## See Also

- [Configuration Guide](configuration.md) - Full YAML configuration reference for combos, layers, and tap-hold
- [Configuration Cookbook](cookbook.md) - Copy-pasteable recipes and real-world configurations
- [Unix Manual Page](man/tff.1) - Complete command-line manual page (`man tff`)
