#!/usr/bin/env python3
"""
Test script to send fake keyboard events to HID devices
"""

import os
import struct
import time

def send_key_event(hid_device_path, key_code, pressed):
    """
    Send a keyboard event to a HID device
    """
    try:
        # Keyboard report format: [modifier, reserved, key1, key2, key3, key4, key5, key6]
        report = bytearray(8)
        report[0] = 0  # No modifier

        if pressed:
            report[2] = key_code  # First key pressed

        with open(hid_device_path, 'wb') as f:
            f.write(report)

        print(f"Sent key {key_code} {'press' if pressed else 'release'} to {hid_device_path}")
        return True
    except Exception as e:
        print(f"Failed to send to {hid_device_path}: {e}")
        return False

def test_fake_keyboard_events():
    """
    Test sending fake keyboard events to simulate TFF combinations
    """
    print("Testing fake keyboard events...")

    # Test hidraw devices
    hidraw_devices = [f"/dev/hidraw{i}" for i in range(4)]

    for device in hidraw_devices:
        if os.path.exists(device):
            print(f"\nTesting device: {device}")

            # Try to send a simple 'f' key press/release
            print("Sending 'f' key press...")
            send_key_event(device, 0x09, True)  # F key
            time.sleep(0.05)
            send_key_event(device, 0x09, False)  # F key release
            time.sleep(0.1)

            # Try to send 'j' key press/release
            print("Sending 'j' key press...")
            send_key_event(device, 0x0a, True)  # J key
            time.sleep(0.05)
            send_key_event(device, 0x0a, False)  # J key release
            time.sleep(0.1)

if __name__ == "__main__":
    test_fake_keyboard_events()