#!/bin/bash
# TFF-like Keyboard Remapping Build Script
set -e

echo "Building TFF C++ System..."

# Configure and build with CMake
cmake -B build
cmake --build build -j"$(nproc)"

# Run all unit tests via CTest
echo "Running unit tests..."
ctest --test-dir build --output-on-failure

echo ""
echo "Build and tests completed successfully!"
echo "Binary created: build/tff_linux (symlinked as build/tff and build/tff2)"