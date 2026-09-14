#!/usr/bin/env python3
"""
Automated End-to-End TFF Testing with USB-OTG and RP2040
======================================================
Tests the full loop:
1. UpBoard USB-OTG (/dev/hidg0) sends fake keyboard reports.
2. RP2040 USB Host port reads raw HID reports and executes TFF remapping logic.
3. RP2040 USB Device port sends remapped HID reports to UpBoard.
4. UpBoard (/dev/input/event*) captures the remapped key event and asserts correctness.
"""

import os
import sys
import time
import struct
import select
import subprocess
import glob

# Linux input event constants
EV_KEY = 1
KEY_UP = 0
KEY_DOWN = 1

# Linux input keycodes
LINUX_KEY_CODES = {
    'ESC': 1,
    'BACKSPACE': 14,
    'A': 30,
    'F': 33,
    'J': 36,
    'SEMICOLON': 39,
    'HOME': 102,
    'UP': 103,
    'PAGEUP': 104,
    'LEFT': 105,
    'RIGHT': 106,
    'END': 107,
    'DOWN': 108,
    'PAGEDOWN': 109,
    'DELETE': 111,
}

# HID keycodes (USB boot keyboard)
HID_KEYS = {
    'a': 0x04,
    'd': 0x07,
    'f': 0x09,
    'g': 0x0A,
    'h': 0x0B,
    'i': 0x0C,
    'j': 0x0D,
    'k': 0x0E,
    'l': 0x0F,
    'm': 0x10,
    'n': 0x11,
    'u': 0x18,
    'semicolon': 0x33,
    'comma': 0x36,
}

TEST_CASES = [
    {
        'name': 'Home Row Index: J + F -> Backspace',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_BACKSPACE (14)',
        'type': 'combo',
    },
    {
        'name': 'Home Row Index: F + J -> Delete',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['j'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['DELETE'],
        'expected_name': 'KEY_DELETE (111)',
        'type': 'combo',
    },
    {
        'name': 'Pinky Combo: Semicolon + A -> Home',
        'key1': HID_KEYS['semicolon'],
        'key2': HID_KEYS['a'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['HOME'],
        'expected_name': 'KEY_HOME (102)',
        'type': 'combo',
    },
    {
        'name': 'Pinky Combo: A + Semicolon -> End',
        'key1': HID_KEYS['a'],
        'key2': HID_KEYS['semicolon'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['END'],
        'expected_name': 'KEY_END (107)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + N -> Down Arrow',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['n'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['DOWN'],
        'expected_name': 'KEY_DOWN (108)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + U -> Up Arrow',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['u'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['UP'],
        'expected_name': 'KEY_UP (103)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + M -> Down Arrow',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['m'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['DOWN'],
        'expected_name': 'KEY_DOWN (108)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + K -> Left Arrow',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['k'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['LEFT'],
        'expected_name': 'KEY_LEFT (105)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + L -> Right Arrow',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['l'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['RIGHT'],
        'expected_name': 'KEY_RIGHT (106)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + I -> Page Up',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['i'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['PAGEUP'],
        'expected_name': 'KEY_PAGEUP (104)',
        'type': 'combo',
    },
    {
        'name': 'Nav Combo: F + Comma -> Page Down',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['comma'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['PAGEDOWN'],
        'expected_name': 'KEY_PAGEDOWN (109)',
        'type': 'combo',
    },
    {
        'name': 'Escape Combo: G + H -> Escape',
        'key1': HID_KEYS['g'],
        'key2': HID_KEYS['h'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['ESC'],
        'expected_name': 'KEY_ESC (1)',
        'type': 'combo',
    },
    {
        'name': 'Sequential Typing: J then F (>100ms threshold, no combo)',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 200,
        'expected_sequence': [LINUX_KEY_CODES['J'], LINUX_KEY_CODES['F']],
        'expected_name': 'KEY_J (36) then KEY_F (33)',
        'type': 'sequential',
    }
]

def find_keyboard_input_device():
    """Find the event device for RP2040 keyboard"""
    by_id = glob.glob('/dev/input/by-id/*RP2040*kbd*')
    if by_id:
        return os.path.realpath(by_id[0])
    return None

def flush_input(in_fd):
    """Flush any queued events from the input event buffer"""
    while True:
        r, _, _ = select.select([in_fd], [], [], 0.02)
        if in_fd in r:
            try:
                os.read(in_fd, 24)
            except OSError:
                break
        else:
            break

def read_input_events(in_fd, timeout=0.6):
    """Read EV_KEY events from input device within timeout"""
    events = []
    t0 = time.time()
    while time.time() - t0 < timeout:
        r, _, _ = select.select([in_fd], [], [], 0.05)
        if in_fd in r:
            try:
                chunk = os.read(in_fd, 24)
                if len(chunk) == 24:
                    sec, usec, ev_type, code, val = struct.unpack('qqHHi', chunk)
                    if ev_type == EV_KEY:
                        events.append((code, val))
            except OSError:
                break
    return events

def run_tests():
    print("=" * 65)
    print("      TFF AUTOMATED USB-OTG HARDWARE TEST SUITE")
    print("=" * 65)

    # 1. Check /dev/hidg0
    hidg_path = '/dev/hidg0'
    if not os.path.exists(hidg_path):
        print(f"Error: {hidg_path} does not exist. Run ./setup_fake_keyboard.sh first.")
        sys.exit(1)

    # 2. Check input event device
    input_path = find_keyboard_input_device()
    if not input_path or not os.path.exists(input_path):
        print("Error: RP2040 Keyboard input device not found in /dev/input/by-id/.")
        sys.exit(1)

    print(f"USB-OTG Output Gadget: {hidg_path}")
    print(f"RP2040 Input Device:   {input_path}")
    print(f"Total Test Cases:      {len(TEST_CASES)}")
    print("-" * 65)

    try:
        out_fd = os.open(hidg_path, os.O_WRONLY)
    except Exception as e:
        print(f"Error opening {hidg_path}: {e}")
        sys.exit(1)

    try:
        in_fd = os.open(input_path, os.O_RDONLY | os.O_NONBLOCK)
    except Exception as e:
        print(f"Error opening {input_path}: {e}")
        os.close(out_fd)
        sys.exit(1)

    passed_count = 0
    failed_count = 0
    results = []

    for idx, test in enumerate(TEST_CASES, 1):
        test_name = test['name']
        print(f"[{idx:2d}/{len(TEST_CASES):2d}] Testing: {test_name}...", end=' ', flush=True)

        # Ensure clean state before test
        os.write(out_fd, bytearray(8))
        time.sleep(0.05)
        flush_input(in_fd)

        if test['type'] == 'combo':
            # Send Key 1 press
            report1 = bytearray([0, 0, test['key1'], 0, 0, 0, 0, 0])
            os.write(out_fd, report1)

            time.sleep(test['delay_ms'] / 1000.0)

            # Send Key 1 + Key 2 overlapping press
            report2 = bytearray([0, 0, test['key1'], test['key2'], 0, 0, 0, 0])
            os.write(out_fd, report2)

            time.sleep(0.04)

            # Send Release
            report_empty = bytearray(8)
            os.write(out_fd, report_empty)

            events = read_input_events(in_fd, timeout=0.6)

            # Verification: expect press and release of expected_linux_code
            expected_code = test['expected_linux_code']
            passed = (
                len(events) >= 2
                and events[0] == (expected_code, KEY_DOWN)
                and events[1] == (expected_code, KEY_UP)
            )

            if passed:
                print("PASS")
                passed_count += 1
                results.append((test_name, "PASS", f"Output {test['expected_name']}"))
            else:
                print(f"FAIL (got {events})")
                failed_count += 1
                results.append((test_name, "FAIL", f"Expected {test['expected_name']}, got {events}"))

        elif test['type'] == 'sequential':
            # Send Key 1 press
            os.write(out_fd, bytearray([0, 0, test['key1'], 0, 0, 0, 0, 0]))
            time.sleep(0.03)
            # Release Key 1
            os.write(out_fd, bytearray(8))

            # Delay greater than threshold
            time.sleep(test['delay_ms'] / 1000.0)

            # Send Key 2 press
            os.write(out_fd, bytearray([0, 0, test['key2'], 0, 0, 0, 0, 0]))
            time.sleep(0.03)
            # Release Key 2
            os.write(out_fd, bytearray(8))

            events = read_input_events(in_fd, timeout=0.6)

            k1_exp = test['expected_sequence'][0]
            k2_exp = test['expected_sequence'][1]

            # Should contain press and release for both keys
            pressed_codes = [c for c, v in events if v == KEY_DOWN]
            passed = (pressed_codes == [k1_exp, k2_exp])

            if passed:
                print("PASS")
                passed_count += 1
                results.append((test_name, "PASS", f"Sequential output {test['expected_name']}"))
            else:
                print(f"FAIL (got {events})")
                failed_count += 1
                results.append((test_name, "FAIL", f"Expected {test['expected_name']}, got {events}"))

        time.sleep(0.2)

    os.close(in_fd)
    os.close(out_fd)

    print("\n" + "=" * 65)
    print("                      TEST RESULTS SUMMARY")
    print("=" * 65)
    for name, status, detail in results:
        status_str = f"\033[92m{status}\033[0m" if status == "PASS" else f"\033[91m{status}\033[0m"
        print(f"[{status_str}] {name:<45} | {detail}")

    print("-" * 65)
    print(f"Total: {len(TEST_CASES)} | Passed: {passed_count} | Failed: {failed_count}")
    print("=" * 65)

    if failed_count == 0:
        print("\033[92mALL AUTOMATED USB-OTG TFF TESTS PASSED SUCCESSFULLY! ✓\033[0m")
        sys.exit(0)
    else:
        print(f"\033[91m{failed_count} TEST(S) FAILED!\033[0m")
        sys.exit(1)

if __name__ == '__main__':
    run_tests()
