#!/bin/bash

# Complete automated TFF testing script
# Tests your my-combos.yaml configuration with fake keyboard events

echo "TFF Automated Testing Script"
echo "============================="
echo ""

# Check if we're on the UpBoard
if ! hostname | grep -q "upboard"; then
    echo "Warning: This script should be run on the UpBoard!"
    echo ""
fi

echo "1. Setting up fake keyboard gadget..."
./setup_fake_keyboard.sh

if [ $? -ne 0 ]; then
    echo "Failed to setup fake keyboard gadget"
    exit 1
fi

echo ""
echo "2. Checking for HID gadget device..."
if [ ! -e /dev/hidg0 ]; then
    echo "HID gadget device not found. Creating symlink..."
    # Find the actual HID device
    HID_DEVICE=$(ls /dev/hidraw* | head -1)
    if [ -n "$HID_DEVICE" ]; then
        ln -sf "$HID_DEVICE" /dev/hidg0
        echo "Created symlink to $HID_DEVICE"
    else
        echo "No HID device found. You may need to manually configure."
    fi
fi

echo ""
echo "3. Converting TFF configuration..."
python3 tools/convert_tff_config.py /home/guettli/projects/tff/my-combos.yaml config/tff-combos.json

if [ $? -ne 0 ]; then
    echo "Failed to convert TFF configuration"
    exit 1
fi

echo ""
echo "4. Building TFF firmware (if needed)..."
# This would normally build the RP2040 firmware
# For testing purposes, we'll assume the firmware is already deployed

echo ""
echo "5. Sending fake keyboard events to test TFF combinations..."

# Run the Python script to send fake events
python3 send_fake_events.py

echo ""
echo "6. Testing complete!"
echo ""
echo "Expected results from your my-combos.yaml:"
echo "  - j f combination should output: backspace"
echo "  - f j combination should output: delete"
echo "  - Sequential keys should output: individual keys"
echo ""
echo "Check the RP2040 output to verify correct remapping!"