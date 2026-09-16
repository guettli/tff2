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

## Building for RP2040

### Prerequisites

1. Raspberry Pi Pico SDK installed
2. PICO_SDK_PATH environment variable set
3. ARM GCC toolchain

### Build Process

```bash
# Using provided script
./build_rp2040.sh

# Or manual build
mkdir build-rp2040
cd build-rp2040
cmake .. -DPICO_BUILD=ON
make
```

### Output

The build produces a UF2 file that can be flashed to the RP2040:
- `tff_rp2040.uf2` - Main firmware image

## Flashing the Firmware

### Method 1: Bootloader Mode

1. Press BOOTSEL button while plugging in USB
2. Copy `tff_rp2040.uf2` to the mounted drive
3. Device resets and runs new firmware

### Method 2: SWD Programming

1. Connect debugger to SWD pins
2. Use `picotool` or debugger software to flash

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
   - Tap/hold detection for modifiers
   - Sequence-based macros
   - Dynamic layer switching

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