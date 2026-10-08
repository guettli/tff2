# RP2040 Implementation Guide

## Overview

The RP2040 implementation provides hardware support for the TFF-like keyboard remapping system using the Adafruit Feather RP2040 USB Host board. This implementation handles both USB host input from keyboards and USB device output to the host computer.

## Hardware Architecture

### Components

1. **Adafruit Feather RP2040 USB Host**
   - RP2040 microcontroller with dual-core ARM Cortex-M0+
   - USB Type-A host port for connecting physical keyboards
   - USB-C device port for connecting to host computer

2. **Connections**
   ```
   Physical Keyboard --> USB-A Host Port
   Computer <---> USB-C Device Port
   ```

## Software Architecture

### Concurrency & Dual-Core Architecture

The RP2040 firmware uses both ARM Cortex-M0+ cores for true parallel execution:

```
┌──────────────────────────────────────┐     ┌──────────────────────────────────────┐
│ Core 1 (USB Host Stack)              │     │ Core 0 (Device, Remapper & Timers)   │
│                                      │     │                                      │
│ - tuh_task()                         │     │ - tud_task() [Device HID + CDC]      │
│ - Pico-PIO-USB bit-banging           │     │ - tff::TFFEngine event processing    │
│   (GPIO 16 D+, GPIO 17 D-)           │     │ - Timer callbacks (tap-hold/combos)  │
│ - tuh_hid_report_received_cb()       │     │ - sendDeviceKeys()                   │
│                                      │     │ - 1200-baud touch / BOOTSEL monitor  │
└──────────────────┬───────────────────┘     └──────────────────▲───────────────────┘
                   │                                            │
                   └──────► Lock-Free SPSC Circular Queue ──────┘
                            (HostKeyboardReport, cap: 32)
```

1. **System Clock (120 MHz)**:
   - Configured via `set_sys_clock_khz(120000, true)` at boot.
   - 120 MHz provides exact integer clock dividers for Pico-PIO-USB 48 MHz USB host state machines.

2. **Core 1 (Dedicated USB Host)**:
   - Launched via `multicore_launch_core1()`.
   - Dedicated exclusively to running `tuh_task()` and receiving reports from keyboards plugged into the USB-A host port.
   - When a report arrives, `tuh_hid_report_received_cb` enqueues it into a lock-free Single-Producer Single-Consumer (SPSC) circular queue and immediately re-arms the USB transfer asynchronously.
   - Clears key state automatically on physical keyboard disconnection (`tuh_hid_umount_cb`).

3. **Core 0 (USB Device, Remapper & Timers)**:
   - Manages the native USB device controller (`tud_task()`) connected to the host computer via USB-C.
   - Dequeues keyboard reports from the lock-free SPSC queue.
   - Evaluates key press and release transitions with zero dynamic heap allocations.
   - Executes the shared `tff::TFFEngine` remapping logic (chords, tap-hold, modal layers, auto-shift).
   - Emits remapped 8-byte HID keyboard reports to the host computer.
   - Enforces release-before-press ordering to prevent ghost chords.
   - Guards reports with `tud_mounted()` checks to prevent blocking when USB-C is unplugged.

4. **Hardware Watchdog & Double-Reset BOOTSEL**:
   - Monitored by the RP2040 hardware watchdog (`hardware_watchdog`).
   - Supports `pico_bootsel_via_double_reset`: quickly double-pressing the reset button reboots into BOOTSEL mode.

### USB Descriptors & Interfaces

The RP2040 presents a composite Interface Association Descriptor (IAD) device with two logical functions:
1. **USB HID Keyboard**: Interface 0 (IN endpoint `0x81`, 1ms poll interval) for standard 8-byte boot keyboard reports.
2. **USB CDC Serial Port**:
   - Interface 1: CDC Communication / ACM (Notification endpoint `0x82`).
   - Interface 2: CDC Data (OUT endpoint `0x02`, IN endpoint `0x83`).
   - Supports 1200-baud touch reset: opening the port at 1200 baud triggers an immediate jump to the `RPI-RP2` bootloader via `reset_usb_boot(0, 0)`.
   - Supports text command reboot: sending `BOOTSEL\r\n` or `BOOTSEL\n` over serial triggers `reset_usb_boot(0, 0)`.

## Pre-Built Firmware

Every release of Ten Flying Fingers includes pre-compiled RP2040 `.uf2` binaries attached as release assets:
- `tff_rp2040.uf2`: Production bare-metal C++ firmware for the Adafruit Feather RP2040 USB Host.
- `tff_rp2040.elf`: ELF binary with debug symbols for GDB / SWD debugging.
- `SHA256SUMS.txt`: Cryptographic SHA-256 checksums to verify binary integrity.

### Downloading the Binary Directly

You do not need to compile the firmware from source. You can download the latest pre-built `.uf2` binary directly from GitHub Releases:

```bash
# Download latest UF2 binary directly via curl:
curl -fLO https://github.com/guettli/tff2/releases/latest/download/tff_rp2040.uf2

# Or download using GitHub CLI:
gh release download --pattern "tff_rp2040*.uf2"
```

Or download it manually from the [GitHub Releases page](https://github.com/guettli/tff2/releases).

## Production Firmware (`tff_rp2040.uf2`)

Ten Flying Fingers on RP2040 is a 100% pure bare-metal C++ firmware:
- **Zero Python Runtime**: Compiled C++ using Pico SDK 2.1.1, TinyUSB, and `Pico-PIO-USB` 0.7.2.
- **Microsecond Latency**: Direct hardware interrupt and PIO state machine execution.
- **Hardware Integration**:
  - Automatically enables 5V boost converter on **GPIO 18** (`board.USB_HOST_5V_POWER`).
  - Bit-bangs full-speed/low-speed USB Host via PIO on GPIO 16 (D+) and GPIO 17 (D-).
  - Emulates composite HID keyboard + CDC serial on native USB-C.
- **Automated Flashing**:
  - 1200-baud touch reboot over CDC.
  - `BOOTSEL` text command reboot.
  - Double-click RESET button via `pico_bootsel_via_double_reset`.

---

## Hardware Cabling & Connectors

When connecting the Adafruit Feather RP2040 USB Host:

```
Physical Keyboard / Fake OTG Input  -->  Normal USB (USB-A Host Port)
Host Computer / UpBoard             <--  USB-C (Device Port)
```

- **USB-C (Device Port)**: Connects to your PC / laptop / UpBoard. The RP2040 emulates a virtual USB HID keyboard + CDC serial device and receives power over this connection.
- **Normal USB / USB-A (Host Port)**: Connects to your physical USB keyboard (or UpBoard USB-OTG port for automated testing).

---

## Flashing & Rebooting the RP2040

### Method 1: Automated Flashing via `scripts/deploy_rp2040.sh`
The deployment script handles everything automatically: detects the compiled `build-rp2040/tff_rp2040.uf2` firmware, triggers an automated reboot over USB CDC into `RPI-RP2` (no buttons needed), mounts the drive, copies the binary, and reboots the board:

```bash
# Auto-detects and flashes build-rp2040/tff_rp2040.uf2:
./scripts/deploy_rp2040.sh

# Or flash a specific UF2 file:
./scripts/deploy_rp2040.sh path/to/tff_rp2040.uf2
```

### Method 2: Automated Software Reboot (`scripts/reboot_rp2040_bootloader.py`)
To put the running board into `RPI-RP2` bootloader mode from software without flashing:

```bash
python3 scripts/reboot_rp2040_bootloader.py
```

This triggers the 1200-baud touch reset on the C++ CDC interface (or sends the `BOOTSEL` command). The board immediately remounts as **`RPI-RP2`**.

### Method 3: Double-Click Reset Button
Thanks to the linked `pico_bootsel_via_double_reset` library, you can simply **double-click the physical RESET button** on the Feather board within 500ms to reboot into BOOTSEL mode without needing to hold down BOOTSEL!

### Method 4: Hardware Button Recovery (Fallback)
If the board is ever unresponsive or frozen:

1. **Press and hold** the **BOOT** (or **BOOTSEL**) button on the RP2040 board.
2. While holding BOOT, **click (press and release)** the **RESET** button.
3. **Release** the BOOT button.
4. The board will mount as **`RPI-RP2`** for drag-and-drop UF2 flashing.

---

## Configuration on RP2040

1. Edit `DEFAULT_YAML_CONFIG` in `src/platform/rp2040/rp2040_platform.cpp`.
2. Recompile with `./build_rp2040.sh`.
3. Copy `build-rp2040/tff_rp2040.uf2` to `RPI-RP2` (or use `./scripts/deploy_rp2040.sh`).

## Building from Source

Ten Flying Fingers provides a unified automated cross-compilation script (`./build_rp2040.sh` or `scripts/build_rp2040.sh`) that manages toolchain verification, Pico SDK cloning, TinyUSB initialization, CMake configuration, and UF2 binary generation.

### Prerequisites

Install the ARM GCC cross-compiler and standard libraries:
- **Debian / Ubuntu**:
  ```bash
  sudo apt-get update && sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib cmake ninja-build
  ```
- **macOS (Homebrew)**:
  ```bash
  brew install armmbed/formulae/arm-none-eabi-gcc cmake ninja
  ```

### Automated Build Script

Run the automated build script from the repository root:
```bash
./build_rp2040.sh
```

The script automatically:
1. Detects `arm-none-eabi-gcc` and verifies compiler version.
2. Checks `$PICO_SDK_PATH`. If unset, it automatically shallow-clones the Raspberry Pi Pico SDK (v2.1.1) and initializes the `lib/tinyusb` submodule.
3. Configures CMake with `-DPICO_BUILD=ON` for the target board (`adafruit_feather_rp2040` by default).
4. Compiles the firmware target `tff_rp2040`.
5. Verifies the generated `build-rp2040/tff_rp2040.uf2` binary and reports file size and SHA-256 hash.

### Build Customization

You can customize the target board or build directory via environment variables:
```bash
# Build for standard Raspberry Pi Pico board
PICO_BOARD=pico ./build_rp2040.sh

# Specify custom build directory and build type
PICO_BOARD=adafruit_feather_rp2040 CMAKE_BUILD_TYPE=Debug ./build_rp2040.sh build-feather-debug
```

### Manual CMake Build

If you prefer using CMake directly:
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
mkdir -p build-rp2040 && cd build-rp2040
cmake -DPICO_BUILD=ON -DPICO_BOARD=adafruit_feather_rp2040 ..
cmake --build . --target tff_rp2040 -j$(nproc)
```

The resulting `tff_rp2040.uf2` is generated in `build-rp2040/`.

---

## CI & Automated Release Pipeline

Every commit and pull request triggers automated firmware compilation in GitHub Actions (`.github/workflows/ci.yml`):
- Pico SDK v2.1.1 and TinyUSB are cached in CI for rapid build times.
- Cross-compilation runs under strict settings.
- The workflow validates the structural integrity of the generated `.uf2` file by checking the UF2 magic headers (`0x0A324655` / `0x9E5D5157`).
- GitHub release events (`.github/workflows/release.yml`) automatically package the `.uf2` and `.elf` files, compute SHA-256 checksums, and publish them to GitHub Releases.

## Debugging & Diagnostic Ring Buffer

The RP2040 firmware includes a zero-heap bounded circular ring buffer (`tff::DebugBuffer`, capacity: 64 entries) that continuously records recent keyboard events:
- Raw incoming USB HID reports (`IN_RAW`)
- Translated engine key events (`IN_EV`)
- Engine timer expirations (`TIMER`)
- Emitted virtual key events (`OUT_EV`)
- Outgoing USB HID reports (`OUT_RAW`)

### Triggering a Diagnostic Dump

Users can trigger a diagnostic dump without serial tools or debuggers:

1. **Hardware Keystroke Chords (Direct Typing)**:
   - **Chord A: `d + f + j + k`** (simultaneously hold all four home-row index and middle finger keys)
   - **Chord B: `LeftShift + RightShift + D`** (hold both Shift keys and tap `D`)
   - The RP2040 freezes the ring buffer and types the complete diagnostic report directly into the active window/editor as virtual keystrokes.
   - Trigger keys are swallowed and not leaked into user text.

2. **USB CDC ACM Command**:
   - Send `dump\n` or `debug\n` to the CDC ACM serial port (e.g. `/dev/ttyACM0`).
   - Or use the host CLI command:
     ```bash
     tff dump [--port <device>]
     ```
   - The dump is emitted over serial without typing into the focused window.

### Common Issues

1. **No USB Host Detection**
   - Check physical connections
   - Verify power supply to keyboard
   - Confirm USB host initialization

2. **No Key Output**
   - Check USB device enumeration
   - Verify key mapping configuration
   - Confirm host computer recognizes device

## Performance Considerations

### Timing

- Key overlap detection uses microsecond precision
- USB polling occurs every millisecond
- Minimal processing delay between input and output

### Memory Usage

- Stack usage optimized for embedded environment
- Heap allocation minimized
- Static memory allocation preferred where possible

## Future Enhancements

### Planned Features

1. **Configuration via USB CDC**
   - Real-time configuration updates
   - Status reporting via serial commands

2. **Multiple Keyboard Support**
   - Simultaneous input from multiple keyboards
   - Per-keyboard configuration profiles

3. **Advanced Mapping Features**
   - Sequential leader key sequences
   - One-shot / sticky modifiers (OSM)
   - Home-row mouse key emulation via USB HID Mouse report

### Optimization Opportunities

1. **Power Management**
   - Sleep modes during idle periods
   - Dynamic CPU frequency scaling

2. **Memory Optimization**
   - Custom allocators for embedded use
   - Zero-copy data processing

3. **Latency Reduction**
   - Direct memory access for USB transfers
   - Interrupt-driven processing