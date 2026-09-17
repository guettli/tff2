# Automated Testing Solution for TFF via USB-OTG

## Overview

The Ten Flying Fingers (TFF) hardware loop is fully verified and automated using the UpBoard's micro-USB OTG port and an Adafruit Feather RP2040 USB Host microcontroller board. No physical keyboard or manual key typing is required.

## Hardware Architecture Loop

```
┌────────────────────────────────────────────────────────────────────────┐
│ UpBoard (Host & Test Controller)                                       │
│                                                                        │
│ 1. /dev/hidg0 (USB Gadget in Device Mode via libcomposite)             │
│    Sends programmatic raw 8-byte HID keyboard reports                  │
└───────────────────┬────────────────────────────────────────────────────┘
                    │ Micro-USB OTG Cable
                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ Adafruit Feather RP2040 with USB Host (CircuitPython 10.3.0)           │
│                                                                        │
│ 2. USB-A Host Port (board.USB_HOST_DATA_PLUS/MINUS + 5V Boost Enable)  │
│    Reads raw HID reports using usb_host.Port & descriptor parsing      │
│                                                                        │
│ 3. TFF Remapper Core (code.py)                                         │
│    - Detects key press timestamps and sequence                         │
│    - Evaluates overlapping combinations (OVERLAP_THRESHOLD_MS = 120ms) │
│    - Emits remapped keys or single-key passthroughs                    │
│                                                                        │
│ 4. USB-C Device Port (adafruit_hid Keyboard)                           │
│    Emits standard USB HID reports back to host computer                │
└───────────────────┬────────────────────────────────────────────────────┘
                    │ USB-C to USB-A Cable
                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ UpBoard (Linux Input Subsystem)                                        │
│                                                                        │
│ 5. /dev/input/by-id/usb-Adafruit_Feather_RP2040_USB_Host_*-event-kbd   │
│    Captures Linux evdev key events and asserts expected keycodes       │
└────────────────────────────────────────────────────────────────────────┘
```

## Setup & Configuration

### 1. USB-OTG Role & Gadget Setup (`setup_fake_keyboard.sh`)
The UpBoard uses an Intel Braswell/Cherry Trail OTG controller (`dwc3.1.auto`). To act as a USB peripheral (device mode):
```bash
echo "device" > /sys/class/usb_role/intel_xhci_usb_sw-role-switch/role
```
The script configures `libcomposite` with:
- Standard 8-byte boot keyboard HID report descriptor
- Vendor ID `0x1d6b`, Product ID `0x0104`
- Read/write permissions (`0666`) on `/dev/hidg0`

### 2. RP2040 Firmware (`src/platform/rp2040/code.py`)
Mounted at `/mnt/circuitpy/code.py`. Key features:
- Activates onboard 5V boost power via `board.USB_HOST_5V_POWER`.
- Initializes PIO USB Host via `usb_host.Port(board.USB_HOST_DATA_PLUS, board.USB_HOST_DATA_MINUS)`.
- Monitors USB HID endpoints, tracking key sequences with microsecond resolution.
- Evaluates configured combo pairs (`J + F` -> `Backspace`, `F + J` -> `Delete`, pinky combos, navigation combos, escape combo).
- Sends remapped HID scancodes via CircuitPython `adafruit_hid.keyboard.Keyboard`.

## Running the Automated Test Suite

Execute the test suite directly:

```bash
./test_tff_automated.sh
```

Or run the Python test runner directly:

```bash
python3 test_tff_automated.py
```

### Verified Test Cases (20/20 Passing)

| Test Case | Simulated Combination / Action | Timing / Order | Expected Output Key(s) | Status |
|-----------|--------------------------------|----------------|------------------------|--------|
| 1 | J + F | Rapid overlap (<120ms) | `KEY_BACKSPACE` (14) | **PASS** |
| 2 | F + J | Rapid overlap (<120ms) | `KEY_DELETE` (111) | **PASS** |
| 3 | Semicolon + A | Rapid overlap (<120ms) | `KEY_HOME` (102) | **PASS** |
| 4 | A + Semicolon | Rapid overlap (<120ms) | `KEY_END` (107) | **PASS** |
| 5 | F + N | Rapid overlap (<120ms) | `KEY_DOWN` (108) | **PASS** |
| 6 | F + U | Rapid overlap (<120ms) | `KEY_UP` (103) | **PASS** |
| 7 | F + M | Rapid overlap (<120ms) | `KEY_DOWN` (108) | **PASS** |
| 8 | F + K | Rapid overlap (<120ms) | `KEY_LEFT` (105) | **PASS** |
| 9 | F + L | Rapid overlap (<120ms) | `KEY_RIGHT` (106) | **PASS** |
| 10 | F + I | Rapid overlap (<120ms) | `KEY_PAGEUP` (104) | **PASS** |
| 11 | F + Comma | Rapid overlap (<120ms) | `KEY_PAGEDOWN` (109) | **PASS** |
| 12 | G + H | Rapid overlap (<120ms) | `KEY_ESC` (1) | **PASS** |
| 13 | D + F + J | Triple combo (<120ms) | `KEY_ESC` (1) | **PASS** |
| 14 | J then F | Sequential (>250ms) | `KEY_J` (36) then `KEY_F` (33) | **PASS** |
| 15 | LeftShift + A | Physical Modifier Pass-through | `KEY_LEFTSHIFT` (42) + `KEY_A` (30) | **PASS** |
| 16 | LeftCtrl + C | Physical Modifier Pass-through | `KEY_LEFTCTRL` (29) + `KEY_C` (46) | **PASS** |
| 17 | J + F twice | Consecutive Repeated Combo | `KEY_BACKSPACE` (14) x2 | **PASS** |
| 18 | A -> (J+F) -> A | Interleaved Typing & Combo | `KEY_A` (30) -> `KEY_BACKSPACE` (14) -> `KEY_A` (30) | **PASS** |
| 19 | F alone | Non-Combo Key Release (< timeout) | `KEY_F` (33) then `KEY_A` (30) | **PASS** |
| 20 | Shift + (J + F) | Combo with Physical Modifier | `KEY_LEFTSHIFT` (42) + `KEY_BACKSPACE` (14) | **PASS** |