#!/bin/bash

# TFF-like Keyboard Remapping Build Script

set -e  # Exit on any error

echo "Building TFF-like Keyboard Remapping System..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake ..

# Build the project
echo "Building project..."
make -j$(nproc)

# Run unit tests
echo "Running unit tests..."
./test_key_detector
./test_key_mapper
./test_tff_app

# Build test executable
echo "Running demo application..."
./tff_test

echo ""
echo "Build completed successfully!"
echo ""
echo "To run unit tests again:"
echo "  cd build && ./test_key_detector && ./test_key_mapper && ./test_tff_app"
echo ""
echo "To run the demo:"
echo "  cd build && ./tff_test"