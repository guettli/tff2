# TFF-like Keyboard Remapping System - Implementation Summary

## Project Overview

This project implements a cross-platform keyboard remapping solution that allows overlapping key combinations for enhanced productivity. Inspired by the Ten Flying Fingers (TFF) project but implemented in C++ for better performance and lower latency.

## Core Components

### 1. Key Detection System
- **KeyDetector**: Detects overlapping key presses within configurable time windows
- **Timing Precision**: Microsecond-level timing for accurate overlap detection
- **Threshold Configuration**: Configurable overlap detection window (default 100ms)

### 2. Key Mapping Engine
- **KeyMapper**: Translates detected key combinations to output key sequences
- **Configuration Driven**: YAML-based configuration files for custom mappings
- **Layer Support**: Multiple mapping layers for different contexts

### 3. Application Framework
- **TFFApp**: Main application class combining detection and mapping
- **Cross-Platform**: Same core logic runs on Linux and RP2040
- **Hardware Agnostic**: Abstract platform interface for easy porting

## Platform Implementations

### Linux Platform
- **Input**: Uses evdev to read from system keyboards
- **Output**: Uses uinput to present as virtual keyboard
- **Testing**: Native compilation and testing environment
- **Development**: Ideal for rapid prototyping and unit testing

### RP2040 Platform
- **Input**: USB host port reads from connected keyboard
- **Output**: USB device port presents as keyboard to computer
- **Hardware**: Designed for Adafruit Feather RP2040 USB Host
- **Integration**: Uses TinyUSB for USB host/device functionality

## Key Features

### Overlapping Key Detection
- F+J pressed together → Output "1"
- J+F pressed together → Output "2"
- F+Space pressed together → Save document (Ctrl+S)
- Configurable time threshold for overlap detection

### Configuration System
- YAML-based configuration files
- Multiple mapping layers
- Runtime configuration reloading (planned)
- Example configurations provided

### Testing Framework
- Unit tests for core logic
- Platform-specific tests
- Hardware testing setup for UpBoard
- Fast execution without hardware dependencies

## Development Status

### Completed
- ✅ Core TFF logic implemented
- ✅ Linux platform support with testing framework
- ✅ RP2040 platform implementation
- ✅ Configuration manager for YAML files
- ✅ Unit tests for all components
- ✅ Build system configured for both platforms
- ✅ Comprehensive documentation

### Next Steps
- [ ] Implement full USB host functionality on RP2040
- [ ] Implement USB device functionality for keyboard output
- [ ] Hardware testing with actual RP2040 board
- [ ] Performance optimization for embedded environment
- [ ] Advanced configuration features

## Building and Testing

### Prerequisites
- Ubuntu 26.04 (or compatible Linux distribution)
- CMake 3.12+
- Raspberry Pi Pico SDK for RP2040 build
- Standard build tools (gcc, make)

### Linux Build
```bash
mkdir build-linux
cd build-linux
cmake ..
make
```

### RP2040 Build
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

## Testing

Run unit tests on Linux:
```bash
./build-linux/tests/unit_tests
```

Or run all tests:
```bash
cd build && make test
```

## Hardware Setup

### RP2040 Wiring
1. Connect keyboard to USB-A host port
2. Connect RP2040 USB-C to computer
3. System appears as standard USB keyboard to computer

### UpBoard Testing Setup
```
UpBoard USB-A Port --> USB-OTG Adapter --> RP2040 USB-C (Device Port)
                                          |
                                          --> RP2040 USB-A (Host Port) --> UpBoard USB-OTG (Fake Keyboard Input)
```

This setup allows:
1. UpBoard to send fake keyboard events to RP2040 USB-A host port (simulating physical keyboard)
2. RP2040 to process key combinations and send mapped output to UpBoard USB-C device port
3. Testing the complete remapping logic without physical keyboard

## Contributing

The core logic is platform-independent, making it easy to:
- Add new key mapping features
- Extend to support different keyboard layouts
- Implement additional combination detection algorithms
- Port to other microcontroller platforms

Documentation is provided for:
- [RP2040 Implementation](docs/rp2040_implementation.md)
- [Configuration System](docs/configuration.md)
- [Local Development Setup](local-dev.md)