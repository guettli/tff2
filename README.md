# TFF-like Keyboard Remapping System

A cross-platform keyboard remapping solution that allows overlapping key combinations for enhanced productivity. Inspired by the Ten Flying Fingers (TFF) project but implemented in C++ for better performance and lower latency.

## Features

- **Overlapping Key Detection**: Detect combinations like F+J vs J+F to trigger different actions
- **Cross-Platform**: Runs on both Linux and RP2040 hardware
- **Configurable Mappings**: Custom key combinations defined in configuration files
- **Low Latency**: Optimized for real-time keyboard processing
- **Hardware Agnostic Core**: Same logic runs on multiple platforms
- **Fast Unit Tests**: Core functionality can be tested without hardware

## How It Works

1. **Key Capture**: Reads input from connected keyboards
2. **Overlap Detection**: Identifies when keys are pressed within configurable time windows
3. **Mapping Lookup**: Translates detected combinations to output key sequences
4. **Key Output**: Sends mapped keys to the host computer

### Example Combinations

- Press F and J together (F+J) → Outputs "1"
- Press J and F together (J+F) → Outputs "2"  
- Press F and Space together → Saves document (Ctrl+S)
- Press J and Space together → Undoes last action (Ctrl+Z)

## Platforms

### Linux Version
- **Input**: Reads physical keyboard events via evdev (`/dev/input/event*`) with optional exclusive grab (`ioctl(fd, EVIOCGRAB, 1)`)
- **Output**: Emits remapped virtual keyboard events via uinput (`/dev/uinput`)
- **Core**: 100% shared `tff::TFFEngine` matching the Go `tff` state machine and CircuitPython RP2040 firmware
- **CLI Daemon**: `tff_linux` supports auto-discovery, custom YAML configs, multi-keyboard polling, and graceful signal handling

```bash
# List discovered keyboards
./build/tff_linux --list

# Run with custom combo configuration
./build/tff_linux config/tff-combos.yaml

# Run for specific device with verbose logs
./build/tff_linux -c config/tff-combos.yaml -d /dev/input/event8 -v
```

### RP2040 Version  
- **Input**: USB host port reads from connected keyboard
- **Output**: USB device port presents as keyboard to computer
- **Development**: Cross-compilation required

## Configuration

Define custom key mappings in YAML format. See [configuration documentation](docs/configuration.md) for detailed information.

```yaml
settings:
  overlap_threshold_ms: 100
  
mappings:
  - name: "navigation"
    combo: ["f", "j"] 
    output: "right"
    
  - name: "numbers"
    combo: ["j", "f"]
    output: "1"
    
layers:
  - name: "main"
    active: true
    mappings:
      - combo: ["f", "space"]
        output: ["ctrl", "s"]
```

The configuration system uses a ConfigManager class to:
- Load key mappings from YAML files
- Save current mappings to YAML files
- Support multiple layers for different contexts
- Allow runtime modification of key mappings

Example configuration files can be found in the [config](config/) directory.

## Building

### Prerequisites

See [local-dev.md](local-dev.md) for detailed setup instructions.

### Linux Build

```bash
mkdir build-linux
cd build-linux
cmake ..
make
```

### RP2040 Build

Requires Raspberry Pi Pico SDK:

```bash
# Set up environment
export PICO_SDK_PATH=/path/to/pico-sdk

# Use provided build script
./build_rp2040.sh

# Or build manually:
mkdir build-rp2040
cd build-rp2040
cmake .. -DPICO_BUILD=ON
make
```

The firmware will be generated as a UF2 file that can be flashed to the RP2040 by copying it to the device when in bootloader mode.

## Testing

### Unit Tests (Hardware-Independent C++)

Run all native unit tests (including the full Go-parity test suite running in ~0.02s without requiring RP2040 or special privileges):

```bash
./build.sh
```

Or run the ported Go unit test suite directly:

```bash
./build/test_tff_go_suite
```

### Automated Hardware Testing (USB-OTG Loop)

End-to-end hardware testing can be run using the UpBoard's micro-USB OTG port connected to the RP2040 USB-A host port:

```bash
./test_tff_automated.sh
```

See [AUTOMATED_TESTING_SOLUTION.md](AUTOMATED_TESTING_SOLUTION.md) for full architectural details and test cases.

## Hardware Setup

### RP2040 Wiring

1. Connect keyboard to USB-A host port
2. Connect RP2040 USB-C to computer
3. System appears as standard USB keyboard to computer

## Contributing

The core logic is platform-independent, making it easy to:
- Add new key mapping features
- Extend to support different keyboard layouts
- Implement additional combination detection algorithms