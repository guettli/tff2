# Local Development Setup for TFF-like Keyboard Remapping

## System Information

- **Operating System**: Ubuntu 26.04
- **Target Hardware**: Adafruit Feather RP2040 USB Host
- **Development Environment**: Native C++ with cross-compilation for RP2040

## Prerequisites

### Ubuntu Packages

```bash
sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib libusb-1.0-0-dev libudev-dev \
    pkg-config libgtest-dev catch2
```

### Raspberry Pi Pico SDK

```bash
# Clone the Pico SDK
git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk
git submodule update --init

# Set environment variable
echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.bashrc
source ~/.bashrc
```

### TinyUSB Library

The TinyUSB library is included as part of the Pico SDK.

## Building for RP2040

```bash
# Set up environment
export PICO_SDK_PATH=/path/to/pico-sdk

# Use provided build script
./build_rp2040.sh

# Or build manually:
mkdir build-rp2040
cd build-rp2040
cmake .. -DPICO_BUILD=ON
make -j$(nproc)
```

The RP2040 implementation uses TinyUSB for both USB host and device functionality:
- USB Host: Reads keyboard input from the USB-A port
- USB Device: Presents as a keyboard to the computer via USB-C port

## Building for Linux

```bash
mkdir build-linux
cd build-linux
cmake ..
make -j$(nproc)
```

## Hardware Testing Setup

### Test Configuration with UpBoard

```
UpBoard USB-A Port --> USB-OTG Adapter --> RP2040 USB-C (Device Port)
                                          |
                                          --> RP2040 USB-A (Host Port) --> UpBoard USB-OTG (Fake Keyboard Input)
```

This setup allows:
1. UpBoard to send fake keyboard events to RP2040 USB-A host port (simulating physical keyboard)
2. RP2040 to process key combinations and send mapped output to UpBoard USB-C device port
3. Testing the complete remapping logic without physical keyboard

## Development Workflow

1. **Native Linux Development**: Develop and test core logic on Linux
2. **Unit Testing**: Run tests on Linux with mocked hardware
3. **Cross Compilation**: Build for RP2040 target
4. **Deployment**: Copy UF2 file to RP2040 or use SWD programmer

## Code Quality & Verification

A unified local check script runs all formatting checks, strict compiler builds, unit tests, static analysis, and smoke checks:

```bash
# Run all pre-push verification checks (clang-format, -Werror, ctest, cppcheck, CLI smoke tests):
./scripts/check.sh

# Run with AddressSanitizer and UndefinedBehaviorSanitizer:
./scripts/check.sh --sanitizers
```

### Code Formatting with `clang-format`

Code style is enforced via `.clang-format` (LLVM/Google base, 4-space indent, 100 column limit).

```bash
# In-place code formatting:
cmake --build build --target format

# Check formatting without modifying files:
cmake --build build --target format-check
```

### Git Pre-Commit Hook

Install the repository pre-commit hook to automatically verify staged C/C++ code and YAML configuration files:

```bash
./scripts/check.sh --install-hooks
# or
cmake --build build --target install-hooks
```


## Documentation

Detailed documentation is available:
- [System Architecture & Sequence Diagrams](architecture.md)
- [C++ Core API Reference](api.md)
- [RP2040 Implementation Guide](rp2040_implementation.md)
- [Configuration System](configuration.md)

## Debugging

### Serial Debug Output

The RP2040 can output debug information via UART/USB serial:

```bash
screen /dev/ttyACM0 115200
```

### Linux Debugging

Use standard debugging tools:

```bash
# Monitor input events
sudo evtest

# Monitor USB devices
lsusb -v

# Monitor uinput devices
cat /proc/bus/input/devices
```