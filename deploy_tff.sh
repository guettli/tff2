#!/bin/bash
# Script to build and deploy TFF firmware for RP2040
set -e

echo "TFF RP2040 Deployment Script"
echo "============================="

echo "Building RP2040 firmware..."
./build_rp2040.sh

echo ""
echo "✓ Firmware built successfully: build-rp2040/tff_rp2040.uf2"
echo ""
echo "Next steps:"
echo "1. Connect RP2040 to computer in bootloader mode (hold BOOTSEL button while connecting USB)"
echo "2. Copy build-rp2040/tff_rp2040.uf2 to the RP2040 mass storage drive (RPI-RP2)"
echo "3. Connect USB keyboard to RP2040 USB-A host port"
echo "4. Connect RP2040 USB-C to your computer"
echo "5. The RP2040 will now remap keystrokes with TFF combinations in real time!"