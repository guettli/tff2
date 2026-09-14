#!/bin/bash

# Script to deploy TFF configuration to RP2040
# Usage: ./deploy_tff.sh [path/to/my-combos.yaml]

echo "TFF Deployment Script"
echo "===================="

CONFIG_FILE="${1:-../tff/my-combos.yaml}"
JSON_OUTPUT="config/tff-combos.json"

echo "Converting $CONFIG_FILE to JSON..."

# Convert YAML to JSON
if [ -f "$CONFIG_FILE" ]; then
    python3 tools/convert_tff_config.py "$CONFIG_FILE" "$JSON_OUTPUT"
    if [ $? -eq 0 ]; then
        echo "✓ Configuration converted successfully"
    else
        echo "✗ Failed to convert configuration"
        exit 1
    fi
else
    echo "Warning: $CONFIG_FILE not found, using existing JSON"
fi

echo "Building RP2040 firmware..."
./build_rp2040.sh

if [ $? -eq 0 ]; then
    echo "✓ Firmware built successfully"
    echo ""
    echo "Next steps:"
    echo "1. Connect RP2040 to computer in bootloader mode"
    echo "2. Copy build/tff_rp2040.uf2 to RP2040 mass storage device"
    echo "3. Connect USB keyboard to RP2040 USB-A host port"
    echo "4. Connect RP2040 USB-C to your computer"
    echo "5. The RP2040 will now remap according to your TFF configuration!"
else
    echo "✗ Failed to build firmware"
    exit 1
fi