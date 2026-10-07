#!/usr/bin/env python3
"""
Reboot RP2040 (running CircuitPython) into BOOTSEL bootloader mode via USB CDC.
Allows automated reflashing without physically pressing the BOOTSEL/RESET buttons.
"""

import sys
import time
import os
import glob

def find_serial_port():
    candidates = glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*')
    if candidates:
        return candidates[0]
    return None

def reboot_to_bootloader(port=None):
    if port is None:
        port = find_serial_port()

    if not port or not os.path.exists(port):
        print("Error: RP2040 USB serial device (/dev/ttyACM*) not found.")
        print("")
        print("If the board is running firmware without USB CDC active (or is unresponsive):")
        print("  1. Press and hold the BOOT button on the RP2040 board.")
        print("  2. Click (press and release) the RESET button.")
        print("  3. Release the BOOT button.")
        print("The board will then mount as 'RPI-RP2' for flashing.")
        return False

    try:
        import serial
    except ImportError:
        print("Error: pyserial is required. Install via: pip install pyserial")
        return False

    print(f"Connecting to RP2040 on {port}...")
    try:
        s = serial.Serial(port, 115200, timeout=1)
        # Interrupt any running code with Ctrl+C
        s.write(b"\x03\r\n")
        time.sleep(0.1)
        # Send Python command to set next reset mode to BOOTLOADER and reboot
        cmd = b"import microcontroller\r\nmicrocontroller.on_next_reset(microcontroller.RunMode.BOOTLOADER)\r\nmicrocontroller.reset()\r\n"
        s.write(cmd)
        time.sleep(0.5)
        s.close()
        print("Reboot command sent successfully! Waiting for RPI-RP2 drive...")
        
        # Wait up to 5 seconds for RPI-RP2 label to appear
        for _ in range(10):
            time.sleep(0.5)
            if glob.glob('/dev/disk/by-label/RPI-RP2') or os.path.exists('/mnt/rpi-rp2'):
                print("✓ RP2040 is now in BOOTSEL bootloader mode (RPI-RP2).")
                return True
        print("Reboot signal sent. Check 'lsblk' or '/dev/disk/by-label/RPI-RP2'.")
        return True
    except Exception as e:
        print(f"Failed to communicate with {port}: {e}")
        return False

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else None
    success = reboot_to_bootloader(port)
    sys.exit(0 if success else 1)
