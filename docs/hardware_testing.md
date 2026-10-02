# Automated Testing Solution for TFF via USB-OTG

## Overview

The Ten Flying Fingers (TFF) hardware loop can be fully verified and automated using any Linux device with a USB-OTG controller (such as an UpBoard, Raspberry Pi Zero, or single-board computer with USB gadget support) connected to an Adafruit Feather RP2040 USB Host microcontroller board. No physical keyboard or manual typing is required.

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
│ Adafruit Feather RP2040 with USB Host (bare-metal C++ / TinyUSB)       │
│                                                                        │
│ 2. USB-A Host Port (TinyUSB host stack)                                │
│    Reads raw HID keyboard reports and parses descriptors               │
│                                                                        │
│ 3. TFF Remapper Core (shared tff::TFFEngine, compiled firmware)        │
│    - Detects key press timestamps and sequence                         │
│    - Evaluates overlapping combos within the configured window         │
│    - Emits remapped keys or single-key passthroughs                    │
│                                                                        │
│ 4. USB-C Device Port (TinyUSB HID keyboard)                            │
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

### 2. RP2040 Firmware (`src/platform/rp2040/`)
The RP2040 runs bare-metal compiled C++ firmware (not CircuitPython). Build it
with `scripts/build_rp2040.sh`, which produces `build-rp2040/tff_rp2040.uf2`;
flash it by holding BOOTSEL and copying the `.uf2` onto the `RPI-RP2` drive. See
[docs/rp2040_implementation.md](rp2040_implementation.md) for the full firmware
architecture. Key features:
- Reads raw HID keyboard reports from connected keyboards via the **TinyUSB host** stack.
- Runs the **shared `tff::TFFEngine`** — the exact same combo, tap-hold, layer,
  and macro logic as the Linux daemon, so hardware behavior matches the unit tests.
- Evaluates combo pairs (`J + F` -> `Backspace`, `F + J` -> `Delete`, pinky combos,
  navigation combos, escape combo) within the configured combo window.
- Sends remapped HID scancodes back to the host via the **TinyUSB device** stack.

## Running the Automated Test Suite

Execute the test suite directly:

```bash
./test_tff_usb_otg.sh
```

Or run the Python test runner directly:

```bash
python3 test_tff_automated.py
```

### Verified Test Cases (30/30 Passing)

| Test Case | Simulated Combination / Action | Timing / Order | Expected Output Key(s) | Status |
|-----------|--------------------------------|----------------|------------------------|--------|
| 1 | J + F | Rapid overlap (within combo window) | `KEY_BACKSPACE` (14) | **PASS** |
| 2 | F + J | Rapid overlap (within combo window) | `KEY_DELETE` (111) | **PASS** |
| 3 | Semicolon + A | Rapid overlap (within combo window) | `KEY_HOME` (102) | **PASS** |
| 4 | A + Semicolon | Rapid overlap (within combo window) | `KEY_END` (107) | **PASS** |
| 5 | F + N | Rapid overlap (within combo window) | `KEY_DOWN` (108) | **PASS** |
| 6 | F + U | Rapid overlap (within combo window) | `KEY_UP` (103) | **PASS** |
| 7 | F + M | Rapid overlap (within combo window) | `KEY_DOWN` (108) | **PASS** |
| 8 | F + K | Rapid overlap (within combo window) | `KEY_LEFT` (105) | **PASS** |
| 9 | F + L | Rapid overlap (within combo window) | `KEY_RIGHT` (106) | **PASS** |
| 10 | F + I | Rapid overlap (within combo window) | `KEY_PAGEUP` (104) | **PASS** |
| 11 | F + Comma | Rapid overlap (within combo window) | `KEY_PAGEDOWN` (109) | **PASS** |
| 12 | G + H | Rapid overlap (within combo window) | `KEY_ESC` (1) | **PASS** |
| 13 | D + F + J | Triple combo (within combo window) | `KEY_ESC` (1) | **PASS** |
| 14 | J then F | Sequential (>250ms) | `KEY_J` (36) then `KEY_F` (33) | **PASS** |
| 15 | LeftShift + A | Physical Modifier Pass-through | `KEY_LEFTSHIFT` (42) + `KEY_A` (30) | **PASS** |
| 16 | LeftCtrl + C | Physical Modifier Pass-through | `KEY_LEFTCTRL` (29) + `KEY_C` (46) | **PASS** |
| 17 | J + F twice | Consecutive Repeated Combo | `KEY_BACKSPACE` (14) x2 | **PASS** |
| 18 | A -> (J+F) -> A | Interleaved Typing & Combo | `KEY_A` (30) -> `KEY_BACKSPACE` (14) -> `KEY_A` (30) | **PASS** |
| 19 | F alone | Non-Combo Key Release (< timeout) | `KEY_F` (33) then `KEY_A` (30) | **PASS** |
| 20 | Shift + (J + F) | Combo with Physical Modifier | `KEY_LEFTSHIFT` (42) + `KEY_BACKSPACE` (14) | **PASS** |
| 21 | J + F (release J first) | Staggered Key Release | `KEY_BACKSPACE` (14) | **PASS** |
| 22 | J + F (release F first) | Reverse Staggered Key Release | `KEY_BACKSPACE` (14) | **PASS** |
| 23 | F + J (release F first) | Staggered Key Release | `KEY_DELETE` (111) | **PASS** |
| 24 | D + F + J | Triple Staggered Release (J->F->D) | `KEY_ESC` (1) | **PASS** |
| 25 | Ctrl + (J + F) | Combo with Ctrl Modifier | `KEY_LEFTCTRL` (29) + `KEY_BACKSPACE` (14) | **PASS** |
| 26 | (J + F) -> (F + J) | Alternating Combos (Backspace then Delete) | `KEY_BACKSPACE` (14) then `KEY_DELETE` (111) | **PASS** |
| 27 | D + F + J twice | Consecutive Repeated Triple Combo | `KEY_ESC` (1) x2 | **PASS** |
| 28 | J + F held 350ms | Sustained Combo Hold | `KEY_BACKSPACE` (14) | **PASS** |
| 29 | F then N (>250ms) | Sequential Rollover (no nav combo) | `KEY_F` (33) then `KEY_N` (49) | **PASS** |
| 30 | A -> (F+N) -> (J+F) -> A | Multi-Action Typing Flow | `KEY_A` -> `DOWN` -> `BACKSPACE` -> `KEY_A` | **PASS** |