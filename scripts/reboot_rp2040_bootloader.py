#!/usr/bin/env python3
"""
Reboot RP2040 microcontroller into BOOTSEL bootloader mode via USB CDC.
Supports:
1. Native C++ TFF firmware: 1200-baud touch reset (standard Pico/TinyUSB convention)
2. Native C++ TFF firmware: "BOOTSEL\\r\\n" serial command
3. CircuitPython firmware: Ctrl+C + microcontroller.reset() command
Allows 100% automated reflashing without physically pressing BOOTSEL/RESET buttons.
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
        print("Note: pyserial not installed. Attempting 1200-baud touch via 'stty'...")
        try:
            import subprocess
            subprocess.run(["stty", "-F", port, "1200"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if wait_for_bootloader(2.0):
                print("✓ RP2040 rebooted into BOOTSEL mode via stty 1200-baud touch.")
                return True
        except Exception:
            pass
        print("Error: pyserial is required for full serial communication. Install via: pip install pyserial")
        return False

    print(f"Connecting to RP2040 on {port}...")

    # Method 1: 1200-baud touch reset (Standard TinyUSB CDC convention used by native C++ firmware)
    print(f"Attempting 1200-baud touch reset on {port}...")
    try:
        s = serial.Serial(port, 1200, timeout=0.5)
        time.sleep(0.1)
        s.close()
    except Exception as e:
        print(f"Note: 1200-baud touch open returned: {e}")

    if wait_for_bootloader(2.0):
        print("✓ RP2040 rebooted into BOOTSEL mode via 1200-baud touch.")
        return True

    # Method 2: Serial commands (C++ 'BOOTSEL' command and CircuitPython fallback)
    # Check if port is still available
    if os.path.exists(port):
        print(f"Sending BOOTSEL and reboot commands on {port}...")
        try:
            s = serial.Serial(port, 115200, timeout=1)
            # Send C++ firmware command
            s.write(b"BOOTSEL\r\n")
            time.sleep(0.1)

            # Also send CircuitPython Ctrl+C + Python reboot command
            s.write(b"\x03\r\n")
            time.sleep(0.1)
            cmd = (
                b"import microcontroller\r\n"
                b"microcontroller.on_next_reset(microcontroller.RunMode.BOOTLOADER)\r\n"
                b"microcontroller.reset()\r\n"
            )
            s.write(cmd)
            time.sleep(0.2)
            s.close()
        except Exception as e:
            print(f"Note: Serial command attempt returned: {e}")

    if wait_for_bootloader(3.0):
        print("✓ RP2040 rebooted into BOOTSEL mode via serial command.")
        return True

    print("Reboot signal sent. Checking for RPI-RP2 drive...")
    if is_bootloader_active():
        print("✓ RP2040 is now in BOOTSEL bootloader mode (RPI-RP2).")
        return True

    print("Warning: RPI-RP2 drive not detected automatically.")
    print("If automatic reboot did not succeed, use the hardware buttons:")
    print("  1. Press and hold the BOOT button on the RP2040 board.")
    print("  2. Click (press and release) the RESET button.")
    print("  3. Release the BOOT button.")
    return False

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else None
    success = reboot_to_bootloader(port)
    sys.exit(0 if success else 1)
