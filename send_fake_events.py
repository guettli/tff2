#!/usr/bin/env python3
"""
Send fake keyboard events to test TFF implementation
"""

import os
import struct
import time
import sys

def send_key_event(key_code, pressed=True):
    """
    Send a keyboard event via /dev/hidg0 (fake keyboard gadget)
    """
    try:
        # Keyboard report format: [modifier, reserved, key1, key2, key3, key4, key5, key6]
        report = bytearray(8)

        if pressed:
            report[2] = key_code  # First key pressed

        with open('/dev/hidg0', 'wb') as f:
            f.write(report)

        if not pressed:
            # Send key release (all zeros)
            report = bytearray(8)
            with open('/dev/hidg0', 'wb') as f:
                f.write(report)

        print(f"Sent key {key_code} {'press' if pressed else 'release'}")
        return True
    except Exception as e:
        print(f"Failed to send key event: {e}")
        return False

def test_tff_combinations():
    """
    Test TFF key combinations from my-combos.yaml
    """
    print("Testing TFF combinations with fake keyboard events...")
    print("=" * 50)

    # Test j f -> backspace
    print("\n1. Testing j f -> backspace")
    print("   Sending 'j' key press...")
    send_key_event(0x0d, True)  # J key
    time.sleep(0.01)
    send_key_event(0x0d, False)  # J key release
    time.sleep(0.05)

    print("   Sending 'f' key press...")
    send_key_event(0x09, True)  # F key
    time.sleep(0.01)
    send_key_event(0x09, False)  # F key release
    time.sleep(0.5)

    # Test f j -> delete
    print("\n2. Testing f j -> delete")
    print("   Sending 'f' key press...")
    send_key_event(0x09, True)  # F key
    time.sleep(0.01)
    send_key_event(0x09, False)  # F key release
    time.sleep(0.05)

    print("   Sending 'j' key press...")
    send_key_event(0x0d, True)  # J key
    time.sleep(0.01)
    send_key_event(0x0d, False)  # J key release
    time.sleep(0.5)

    # Test sequential keys (should not trigger combination)
    print("\n3. Testing sequential keys (should not trigger combination)")
    print("   Sending 'j' key press...")
    send_key_event(0x0d, True)  # J key
    time.sleep(0.01)
    send_key_event(0x0d, False)  # J key release
    time.sleep(0.15)  # Longer delay (>100ms threshold)

    print("   Sending 'f' key press...")
    send_key_event(0x09, True)  # F key
    time.sleep(0.01)
    send_key_event(0x09, False)  # F key release
    time.sleep(0.5)

    print("\nTFF combination testing complete!")
    print("Check if the RP2040 output the correct remapped keys.")

if __name__ == "__main__":
    # First setup the fake keyboard gadget
    print("Setting up fake keyboard gadget...")

    # This would normally run the setup_fake_keyboard.sh script
    # For now, we'll assume it's already set up

    try:
        test_tff_combinations()
    except KeyboardInterrupt:
        print("\nTesting interrupted by user.")
    except Exception as e:
        print(f"\nError during testing: {e}")
        print("Make sure the fake keyboard gadget is properly configured.")