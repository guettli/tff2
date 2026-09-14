#!/bin/bash
# Setup script to create a fake keyboard using libcomposite on UpBoard USB-OTG
# Enables automated testing of the RP2040 TFF implementation

set -e

# If not running as root, re-run via ssh root@localhost or sudo
if [ "$(id -u)" -ne 0 ]; then
    echo "Elevating to root..."
    if ssh -o BatchMode=yes -o StrictHostKeyChecking=no root@localhost whoami >/dev/null 2>&1; then
        exec ssh root@localhost "bash -s" < "$0" "$@"
    elif command -v sudo >/dev/null 2>&1; then
        exec sudo "$0" "$@"
    else
        echo "Error: Root access required to configure USB gadget."
        exit 1
    fi
fi

echo "Setting up fake keyboard for TFF testing on USB-OTG..."

# Load required modules
modprobe libcomposite || true

# Switch UpBoard OTG port to device mode
ROLE_SWITCH="/sys/class/usb_role/intel_xhci_usb_sw-role-switch/role"
if [ -f "$ROLE_SWITCH" ]; then
    echo "device" > "$ROLE_SWITCH"
    echo "Set USB role switch to 'device'"
fi

# Gadget configuration
GADGET_DIR="/sys/kernel/config/usb_gadget/fake_keyboard"

# If gadget is currently enabled, disable it first
if [ -f "$GADGET_DIR/UDC" ]; then
    echo "" > "$GADGET_DIR/UDC" 2>/dev/null || true
fi

# Clean up old configuration if present
rm -f "$GADGET_DIR/configs/c.1/hid.usb0" 2>/dev/null || true
rmdir "$GADGET_DIR/configs/c.1/strings/0x409" 2>/dev/null || true
rmdir "$GADGET_DIR/configs/c.1" 2>/dev/null || true
rmdir "$GADGET_DIR/functions/hid.usb0" 2>/dev/null || true
rmdir "$GADGET_DIR/strings/0x409" 2>/dev/null || true
rmdir "$GADGET_DIR" 2>/dev/null || true

mkdir -p "$GADGET_DIR"
cd "$GADGET_DIR"

# Device descriptors
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB
echo 0x00 > bDeviceClass
echo 0x00 > bDeviceSubClass
echo 0x00 > bDeviceProtocol
echo 0x40 > bMaxPacketSize0
echo 0x1d6b > idVendor
echo 0x0104 > idProduct

# Device strings
mkdir -p strings/0x409
echo "0123456789" > strings/0x409/serialnumber
echo "UpBoard" > strings/0x409/manufacturer
echo "TFF Fake Keyboard" > strings/0x409/product

# Configuration
mkdir -p configs/c.1/strings/0x409
echo "Keyboard Config" > configs/c.1/strings/0x409/configuration
echo 250 > configs/c.1/MaxPower
echo 0x80 > configs/c.1/bmAttributes

# HID Function
mkdir -p functions/hid.usb0
echo 1 > functions/hid.usb0/protocol
echo 1 > functions/hid.usb0/subclass
echo 8 > functions/hid.usb0/report_length

# Standard 8-byte boot keyboard HID report descriptor
printf "\x05\x01\x09\x06\xa1\x01\x05\x07\x19\xe0\x29\xe7\x15\x00\x25\x01\x75\x01\x95\x08\x81\x02\x95\x01\x75\x08\x81\x03\x95\x06\x75\x08\x15\x00\x26\xff\x00\x05\x07\x19\x00\x2a\xff\x00\x81\x00\xc0" > functions/hid.usb0/report_desc

# Link function to configuration
ln -s functions/hid.usb0 configs/c.1/

# Enable gadget on UDC
UDC_NAME=$(ls /sys/class/udc | head -n 1)
if [ -n "$UDC_NAME" ]; then
    echo "$UDC_NAME" > UDC
    echo "Bound fake keyboard to UDC: $UDC_NAME"
else
    echo "Warning: No UDC device found in /sys/class/udc"
fi

sleep 0.5
if [ -e /dev/hidg0 ]; then
    chmod 666 /dev/hidg0
    echo "✓ /dev/hidg0 created successfully with read/write permissions."
fi

# Ensure RP2040 input device permissions
chmod 666 /dev/input/event* /dev/ttyACM* 2>/dev/null || true

echo "Fake keyboard gadget setup complete!"