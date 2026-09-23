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

### Key Classes

#### RP2040Platform
Main platform handler that manages:
- USB host initialization and event processing (TinyUSB host)
- USB device initialization and key output (TinyUSB device)
- Integration with the shared `tff::TFFEngine` (combos, tap-vs-hold, text snippets, and modal layers)
- Hardware-specific timing and timer handling via `checkTimers()`

### USB Integration

#### USB Host (Keyboard Input)
- Uses TinyUSB host stack to read from connected keyboards
- Processes HID keyboard reports (Usage Page 0x07)
- Converts USB key codes to internal Linux `KeyCode`s via `convertUsbToKeyCode`
- Tracks key press/release events with microsecond timestamps

#### USB Device (Keyboard Output)
- Presents as standard HID keyboard to host computer
- Sends mapped key combinations, tap-hold outputs, and layer remappings
- Converts internal `KeyCode`s back to USB HID codes via `convertKeyCodeToUsb`
- Handles key press/release sequences with proper event release swallowing

## Implementation Details

### Shared Core Engine (`tff::TFFEngine`)
The RP2040 firmware uses the exact same `tff::TFFEngine` as the Linux platform daemon:
- **Zero Drift**: All combo matching, triple chords, tap-hold dual role logic, modal layers, and macro expansions are evaluated identically.
- **YAML Configurable**: Full configuration can be supplied via YAML strings or the built-in default configuration.

### Event Processing Flow

1. **USB Host Event**: Keyboard report received via `tuh_hid_report_received_cb`
2. **Key Code Conversion**: USB HID codes converted to internal `tff::KeyCode`s via `convertUsbToKeyCode`
3. **Timestamp Capture**: Current millisecond timestamp captured from `get_absolute_time()`
4. **Core Processing**: Shared `tff::TFFEngine::processEvent` evaluates chords, layers, and tap-hold state
5. **Output Writing**: Remapped events translated to USB HID codes and sent via `tud_hid_keyboard_report`
6. **Timer Servicing**: Hardware loop calls `checkTimers()` to service tap-hold timeouts and combo expiration

## Pre-Built Firmware

Every release of Ten Flying Fingers includes pre-compiled RP2040 `.uf2` binaries attached as release assets:
- `tff_rp2040_<version>.uf2`: Ready-to-flash binary for the Adafruit Feather RP2040 USB Host (and compatible RP2040 boards).
- `tff_rp2040_<version>.elf`: ELF binary with debug symbols for GDB / SWD debugging.
- `SHA256SUMS.txt`: Cryptographic SHA-256 checksums to verify binary integrity.

You can download the latest pre-built firmware directly from the [GitHub Releases page](https://github.com/guettli/tff2/releases).

---

## Flashing the Firmware

### Method 1: Drag-and-Drop BOOTSEL Mode (Recommended)

1. Unplug the RP2040 board from your computer.
2. Press and hold down the **BOOTSEL** button on the board.
3. While continuing to hold the button, plug the RP2040 board's USB-C cable into your computer.
4. Release the **BOOTSEL** button. The board will mount as a USB mass storage drive named **`RPI-RP2`**.
5. Drag and drop (or copy) `tff_rp2040.uf2` directly onto the `RPI-RP2` drive:
   ```bash
   cp tff_rp2040.uf2 /media/$USER/RPI-RP2/
   ```
6. The board will automatically flash the firmware, unmount the drive, and reboot immediately running Ten Flying Fingers.

### Method 2: SWD Programming

1. Connect a debug probe (e.g. Raspberry Pi Debug Probe or Picoprobe) to the SWD header pins (SWCLK, SWDIO, GND).
2. Flash using `picotool` or OpenOCD:
   ```bash
   picotool load tff_rp2040.elf
   picotool reboot
   ```

---

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

## Debugging

### Serial Output

The firmware outputs debug information via USB serial:
```bash
screen /dev/ttyACM0 115200
```

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