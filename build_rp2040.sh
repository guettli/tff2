#!/bin/bash

# Build script for RP2040 platform

# Check if pico-sdk is installed
if [ ! -d "$PICO_SDK_PATH" ]; then
    echo "Error: PICO_SDK_PATH is not set or pico-sdk not found"
    echo "Please set PICO_SDK_PATH to point to your pico-sdk installation"
    echo "Example: export PICO_SDK_PATH=/path/to/pico-sdk"
    exit 1
fi

# Create build directory
mkdir -p build_rp2040
cd build_rp2040

# Configure with PICO_BUILD flag
cmake -DPICO_BUILD=ON ..

# Build
make -j$(nproc)

echo "Build complete. Firmware can be found in build_rp2040/tff_rp2040.uf2"