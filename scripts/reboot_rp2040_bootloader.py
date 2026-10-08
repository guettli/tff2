#!/usr/bin/env python3
"""
Reboot RP2040 microcontroller into BOOTSEL bootloader mode via USB CDC.
Uses standard TinyUSB / Pico 1200-baud touch reset.
Fails fast if dependencies are missing or reset fails.
"""

import sys
import time
import os
import glob
import serial  # Fail fast: requires pyserial (pip install pyserial)

def find_serial_port():
    candidates = glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*')
    if not candidates:
        return None
    if len(candidates) > 1:
        print(f"Error: Multiple serial ports detected ({', '.join(candidates)}).", file=sys.stderr)
        print("Please specify the exact RP2040 port explicitly:", file=sys.stderr)
        print(f"  {sys.argv[0]} <port>", file=sys.stderr)
        sys.exit(1)
    return candidates[0]

def is_bootloader_active():
    return bool(glob.glob('/dev/disk/by-label/RPI-RP2') or os.path.exists('/mnt/rpi-rp2'))

def wait_for_bootloader(timeout_sec=5.0):
    t0 = time.time()
    while time.time() - t0 < timeout_sec:
        time.sleep(0.3)
        if is_bootloader_active():
            return True
    return False

def reboot_to_bootloader(port=None):
    if is_bootloader_active():
        print("✓ RP2040 is already in BOOTSEL bootloader mode (RPI-RP2).")
        return True

    if port is None:
        port = find_serial_port()

    if not port or not os.path.exists(port):
        print("Error: RP2040 USB serial device (/dev/ttyACM*) not found.", file=sys.stderr)
        print("", file=sys.stderr)
        print("If the board is unresponsive or needs manual recovery:", file=sys.stderr)
        print("  1. Press and hold the BOOT button on the RP2040 board.", file=sys.stderr)
        print("  2. Click (press and release) the RESET button.", file=sys.stderr)
        print("  3. Release the BOOT button.", file=sys.stderr)
        print("The board will then mount as 'RPI-RP2' for flashing.", file=sys.stderr)
        return False

    print(f"Connecting to RP2040 on {port}...")

    # 1200-baud touch reset (Standard TinyUSB CDC convention)
    print(f"Triggering 1200-baud touch reset on {port}...")
    try:
        s = serial.Serial(port, 1200, timeout=0.5)
        time.sleep(0.1)
        s.close()
    except Exception as e:
        print(f"Error opening {port} at 1200 baud: {e}", file=sys.stderr)
        return False

    if wait_for_bootloader(3.0):
        print("✓ RP2040 rebooted into BOOTSEL mode via 1200-baud touch.")
        return True

    print(f"Error: Failed to reboot RP2040 on {port} into BOOTSEL mode within timeout.", file=sys.stderr)
    print("Please use the hardware buttons on the board:", file=sys.stderr)
    print("  1. Press and hold the BOOT button on the RP2040 board.", file=sys.stderr)
    print("  2. Click (press and release) the RESET button.", file=sys.stderr)
    print("  3. Release the BOOT button.", file=sys.stderr)
    return False

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else None
    success = reboot_to_bootloader(port)
    sys.exit(0 if success else 1)
