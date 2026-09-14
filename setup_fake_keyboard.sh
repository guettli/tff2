#!/bin/bash

# Setup script to create a fake keyboard using libcomposite
# This will allow automated testing of the RP2040 TFF implementation

echo "Setting up fake keyboard for TFF testing..."

# Load required modules
modprobe libcomposite
modprobe dummy_hcd

# Create gadget directory
GADGET_DIR="/sys/kernel/config/usb_gadget/fake_keyboard"
mkdir -p $GADGET_DIR

# Configure gadget
cd $GADGET_DIR

# Set device descriptors
echo 0x1d6b > idVendor
echo 0x0104 > idProduct
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB

# Create configuration
mkdir configs/c.1
mkdir functions/hid.usb0

# Set HID function parameters
echo 0x01 > functions/hid.usb0/protocol
echo 0x01 > functions/hid.usb0/subclass
echo 0x08 > functions/hid.usb0/report_length

# Create HID report descriptor (simple keyboard)
echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x26\\xff\\x00\\x05\\x07\\x19\\x00\\x2a\\xff\\x00\\x81\\x00\\xc0 > functions/hid.usb0/report_desc

# Link function to configuration
ln -s functions/hid.usb0 configs/c.1/

# Enable gadget
ls /sys/class/udc > UDC

echo "Fake keyboard gadget created successfully!"
echo "You can now send keyboard events to test the RP2040 TFF implementation."