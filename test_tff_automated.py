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
    'LEFTCTRL': 29,
    'A': 30,
    'D': 32,
    'F': 33,
    'J': 36,
    'SEMICOLON': 39,
    'LEFTSHIFT': 42,
    'C': 46,
    'N': 49,
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
    'c': 0x06,
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
        'name': 'Triple Combo: D + F + J -> Escape',
        'keys': [HID_KEYS['d'], HID_KEYS['f'], HID_KEYS['j']],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['ESC'],
        'expected_name': 'KEY_ESC (1)',
        'type': 'triple',
    },
    {
        'name': 'Sequential Typing: J then F (>100ms threshold, no combo)',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 250,
        'expected_sequence': [LINUX_KEY_CODES['J'], LINUX_KEY_CODES['F']],
        'expected_name': 'KEY_J (36) then KEY_F (33)',
        'type': 'sequential',
    },
    {
        'name': 'Physical Modifier: LeftShift + A -> Shifted A',
        'mod_byte': 0x02,
        'key': HID_KEYS['a'],
        'expected_mod': LINUX_KEY_CODES['LEFTSHIFT'],
        'expected_key': LINUX_KEY_CODES['A'],
        'expected_name': 'KEY_LEFTSHIFT (42) + KEY_A (30)',
        'type': 'modifier',
    },
    {
        'name': 'Physical Modifier: LeftCtrl + C -> Ctrl + C',
        'mod_byte': 0x01,
        'key': HID_KEYS['c'],
        'expected_mod': LINUX_KEY_CODES['LEFTCTRL'],
        'expected_key': LINUX_KEY_CODES['C'],
        'expected_name': 'KEY_LEFTCTRL (29) + KEY_C (46)',
        'type': 'modifier',
    },
    {
        'name': 'Consecutive Repeated Combo: J + F twice -> Backspace x2',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_BACKSPACE (14) twice',
        'type': 'repeat_combo',
    },
    {
        'name': 'Interleaved Typing & Combo: A -> Backspace (J+F) -> A',
        'single_key': HID_KEYS['a'],
        'combo_k1': HID_KEYS['j'],
        'combo_k2': HID_KEYS['f'],
        'delay_ms': 20,
        'expected_sequence': [LINUX_KEY_CODES['A'], LINUX_KEY_CODES['BACKSPACE'], LINUX_KEY_CODES['A']],
        'expected_name': 'KEY_A (30) -> KEY_BACKSPACE (14) -> KEY_A (30)',
        'type': 'interleaved',
    },
    {
        'name': 'Non-Combo Key Release: F alone (< timeout, released before combo)',
        'key': HID_KEYS['f'],
        'next_key': HID_KEYS['a'],
        'expected_key': LINUX_KEY_CODES['F'],
        'expected_next': LINUX_KEY_CODES['A'],
        'expected_name': 'KEY_F (33) then KEY_A (30)',
        'type': 'single_tap',
    },
    {
        'name': 'Combo with Physical Modifier: Shift + (J + F) -> Shifted Backspace',
        'mod_byte': 0x02,
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 20,
        'expected_mod': LINUX_KEY_CODES['LEFTSHIFT'],
        'expected_key': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_LEFTSHIFT (42) + KEY_BACKSPACE (14)',
        'type': 'combo_with_mod',
    },
    {
        'name': 'Staggered Release: J + F (release J first) -> Backspace',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 20,
        'first_release': 1,
        'expected_linux_code': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_BACKSPACE (14)',
        'type': 'staggered_release',
    },
    {
        'name': 'Reverse Staggered Release: J + F (release F first) -> Backspace',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 20,
        'first_release': 2,
        'expected_linux_code': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_BACKSPACE (14)',
        'type': 'staggered_release',
    },
    {
        'name': 'Staggered Release: F + J (release F first) -> Delete',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['j'],
        'delay_ms': 20,
        'first_release': 1,
        'expected_linux_code': LINUX_KEY_CODES['DELETE'],
        'expected_name': 'KEY_DELETE (111)',
        'type': 'staggered_release',
    },
    {
        'name': 'Triple Combo Staggered Release: D + F + J -> Escape',
        'keys': [HID_KEYS['d'], HID_KEYS['f'], HID_KEYS['j']],
        'delay_ms': 20,
        'expected_linux_code': LINUX_KEY_CODES['ESC'],
        'expected_name': 'KEY_ESC (1)',
        'type': 'staggered_triple_release',
    },
    {
        'name': 'Combo with Ctrl Modifier: Ctrl + (J + F) -> Ctrl + Backspace',
        'mod_byte': 0x01,
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'delay_ms': 20,
        'expected_mod': LINUX_KEY_CODES['LEFTCTRL'],
        'expected_key': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_LEFTCTRL (29) + KEY_BACKSPACE (14)',
        'type': 'combo_with_mod',
    },
    {
        'name': 'Alternating Combos: (J + F) -> Backspace then (F + J) -> Delete',
        'c1_k1': HID_KEYS['j'],
        'c1_k2': HID_KEYS['f'],
        'c2_k1': HID_KEYS['f'],
        'c2_k2': HID_KEYS['j'],
        'expected_sequence': [LINUX_KEY_CODES['BACKSPACE'], LINUX_KEY_CODES['DELETE']],
        'expected_name': 'KEY_BACKSPACE (14) then KEY_DELETE (111)',
        'type': 'alternating_combos',
    },
    {
        'name': 'Triple Combo Repeated: D + F + J twice -> Esc x2',
        'keys': [HID_KEYS['d'], HID_KEYS['f'], HID_KEYS['j']],
        'expected_linux_code': LINUX_KEY_CODES['ESC'],
        'expected_name': 'KEY_ESC (1) twice',
        'type': 'repeat_triple',
    },
    {
        'name': 'Sustained Combo Hold: J + F held for 350ms -> Backspace',
        'key1': HID_KEYS['j'],
        'key2': HID_KEYS['f'],
        'hold_duration_s': 0.35,
        'expected_linux_code': LINUX_KEY_CODES['BACKSPACE'],
        'expected_name': 'KEY_BACKSPACE (14) sustained',
        'type': 'sustained_hold',
    },
    {
        'name': 'Sequential Rollover: F then N (>200ms gap, no nav combo)',
        'key1': HID_KEYS['f'],
        'key2': HID_KEYS['n'],
        'delay_ms': 250,
        'expected_sequence': [LINUX_KEY_CODES['F'], LINUX_KEY_CODES['N']],
        'expected_name': 'KEY_F (33) then KEY_N (49)',
        'type': 'sequential',
    },
    {
        'name': 'Multi-Action Typing Flow: A -> (F+N) -> (J+F) -> A',
        'k1': HID_KEYS['a'],
        'c1_k1': HID_KEYS['f'],
        'c1_k2': HID_KEYS['n'],
        'c2_k1': HID_KEYS['j'],
        'c2_k2': HID_KEYS['f'],
        'k2': HID_KEYS['a'],
        'expected_sequence': [LINUX_KEY_CODES['A'], LINUX_KEY_CODES['DOWN'], LINUX_KEY_CODES['BACKSPACE'], LINUX_KEY_CODES['A']],
        'expected_name': 'A (30) -> DOWN (108) -> BACKSPACE (14) -> A (30)',
        'type': 'multi_interleaved',
    },
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
                    if ev_type == EV_KEY and val != 2:
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

def execute_test_attempt(test, out_fd, in_fd):
    """Execute a single attempt of a test case and return (passed, detail, events)"""
    os.write(out_fd, bytearray(8))
    time.sleep(0.25)
    flush_input(in_fd)

    if test['type'] == 'combo':
        report1 = bytearray([0, 0, test['key1'], 0, 0, 0, 0, 0])
        os.write(out_fd, report1)
        time.sleep(test['delay_ms'] / 1000.0)

        report2 = bytearray([0, 0, test['key1'], test['key2'], 0, 0, 0, 0])
        os.write(out_fd, report2)
        time.sleep(0.18)

        os.write(out_fd, bytearray(8))
        events = read_input_events(in_fd, timeout=0.6)

        expected_code = test['expected_linux_code']
        passed = (
            len(events) >= 2
            and events[0] == (expected_code, KEY_DOWN)
            and events[1] == (expected_code, KEY_UP)
        )
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'triple':
        os.write(out_fd, bytearray([0, 0, test['keys'][0], 0, 0, 0, 0, 0]))
        time.sleep(test['delay_ms'] / 1000.0)
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], 0, 0, 0, 0]))
        time.sleep(test['delay_ms'] / 1000.0)
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], test['keys'][2], 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        expected_code = test['expected_linux_code']
        passed = (
            len(events) >= 2
            and events[0] == (expected_code, KEY_DOWN)
            and events[1] == (expected_code, KEY_UP)
        )
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'sequential':
        os.write(out_fd, bytearray([0, 0, test['key1'], 0, 0, 0, 0, 0]))
        time.sleep(0.05)
        os.write(out_fd, bytearray(8))
        time.sleep(test['delay_ms'] / 1000.0)

        os.write(out_fd, bytearray([0, 0, test['key2'], 0, 0, 0, 0, 0]))
        time.sleep(0.05)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        k1_exp = test['expected_sequence'][0]
        k2_exp = test['expected_sequence'][1]
        pressed_codes = [c for c, v in events if v == KEY_DOWN]
        passed = (pressed_codes == [k1_exp, k2_exp])
        detail = f"Sequential output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'modifier':
        report = bytearray([test['mod_byte'], 0, test['key'], 0, 0, 0, 0, 0])
        os.write(out_fd, report)
        time.sleep(0.08)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        pressed_codes = [c for c, v in events if v == KEY_DOWN]
        passed = (test['expected_mod'] in pressed_codes and test['expected_key'] in pressed_codes)
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'repeat_combo':
        # 1st combo execution
        os.write(out_fd, bytearray([0, 0, test['key1'], test['key2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))
        time.sleep(0.25)

        # 2nd combo execution
        os.write(out_fd, bytearray([0, 0, test['key1'], test['key2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        expected_code = test['expected_linux_code']
        down_events = [c for c, v in events if v == KEY_DOWN and c == expected_code]
        passed = (len(down_events) >= 2)
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'interleaved':
        # 1. Single key tap
        os.write(out_fd, bytearray([0, 0, test['single_key'], 0, 0, 0, 0, 0]))
        time.sleep(0.04)
        os.write(out_fd, bytearray(8))
        time.sleep(0.25)

        # 2. Combo
        os.write(out_fd, bytearray([0, 0, test['combo_k1'], test['combo_k2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))
        time.sleep(0.25)

        # 3. Single key tap again
        os.write(out_fd, bytearray([0, 0, test['single_key'], 0, 0, 0, 0, 0]))
        time.sleep(0.04)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        pressed = [c for c, v in events if v == KEY_DOWN]
        passed = (pressed == test['expected_sequence'])
        detail = f"Sequence {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'single_tap':
        os.write(out_fd, bytearray([0, 0, test['key'], 0, 0, 0, 0, 0]))
        time.sleep(0.04)
        os.write(out_fd, bytearray(8))
        time.sleep(0.25)

        os.write(out_fd, bytearray([0, 0, test['next_key'], 0, 0, 0, 0, 0]))
        time.sleep(0.04)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        pressed = [c for c, v in events if v == KEY_DOWN]
        expected = [test['expected_key'], test['expected_next']]
        passed = (pressed == expected)
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'combo_with_mod':
        # Send Mod + Key 1 + Key 2
        os.write(out_fd, bytearray([test['mod_byte'], 0, test['key1'], test['key2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        pressed_codes = [c for c, v in events if v == KEY_DOWN]
        passed = (test['expected_mod'] in pressed_codes and test['expected_key'] in pressed_codes)
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'staggered_release':
        report1 = bytearray([0, 0, test['key1'], 0, 0, 0, 0, 0])
        os.write(out_fd, report1)
        time.sleep(test['delay_ms'] / 1000.0)

        report2 = bytearray([0, 0, test['key1'], test['key2'], 0, 0, 0, 0])
        os.write(out_fd, report2)
        time.sleep(0.18)

        if test.get('first_release', 1) == 1:
            # Release key1 first
            os.write(out_fd, bytearray([0, 0, test['key2'], 0, 0, 0, 0, 0]))
            time.sleep(0.06)
            os.write(out_fd, bytearray(8))
        else:
            # Release key2 first
            os.write(out_fd, bytearray([0, 0, test['key1'], 0, 0, 0, 0, 0]))
            time.sleep(0.06)
            os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        expected_code = test['expected_linux_code']
        passed = (
            len(events) >= 2
            and events[0] == (expected_code, KEY_DOWN)
            and events[1] == (expected_code, KEY_UP)
        )
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'staggered_triple_release':
        os.write(out_fd, bytearray([0, 0, test['keys'][0], 0, 0, 0, 0, 0]))
        time.sleep(test['delay_ms'] / 1000.0)
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], 0, 0, 0, 0]))
        time.sleep(test['delay_ms'] / 1000.0)
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], test['keys'][2], 0, 0, 0]))
        time.sleep(0.18)

        # Release keys one by one in reverse order
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], 0, 0, 0, 0]))
        time.sleep(0.05)
        os.write(out_fd, bytearray([0, 0, test['keys'][0], 0, 0, 0, 0, 0]))
        time.sleep(0.05)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        expected_code = test['expected_linux_code']
        passed = (
            len(events) >= 2
            and events[0] == (expected_code, KEY_DOWN)
            and events[1] == (expected_code, KEY_UP)
        )
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'alternating_combos':
        # Combo 1
        os.write(out_fd, bytearray([0, 0, test['c1_k1'], test['c1_k2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))
        time.sleep(0.20)

        # Combo 2
        os.write(out_fd, bytearray([0, 0, test['c2_k1'], test['c2_k2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        down_codes = [c for c, v in events if v == KEY_DOWN]
        passed = (down_codes == test['expected_sequence'])
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'repeat_triple':
        # Triple 1
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], test['keys'][2], 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))
        time.sleep(0.20)

        # Triple 2
        os.write(out_fd, bytearray([0, 0, test['keys'][0], test['keys'][1], test['keys'][2], 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        down_codes = [c for c, v in events if v == KEY_DOWN and c == test['expected_linux_code']]
        passed = (len(down_codes) >= 2)
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'sustained_hold':
        os.write(out_fd, bytearray([0, 0, test['key1'], test['key2'], 0, 0, 0, 0]))
        time.sleep(test['hold_duration_s'])
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.6)
        expected_code = test['expected_linux_code']
        passed = (
            len(events) >= 2
            and events[0] == (expected_code, KEY_DOWN)
            and events[-1] == (expected_code, KEY_UP)
        )
        detail = f"Output {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    elif test['type'] == 'multi_interleaved':
        # 1. Tap A
        os.write(out_fd, bytearray([0, 0, test['k1'], 0, 0, 0, 0, 0]))
        time.sleep(0.04)
        os.write(out_fd, bytearray(8))
        time.sleep(0.20)

        # 2. Combo F+N (Down)
        os.write(out_fd, bytearray([0, 0, test['c1_k1'], test['c1_k2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))
        time.sleep(0.20)

        # 3. Combo J+F (Backspace)
        os.write(out_fd, bytearray([0, 0, test['c2_k1'], test['c2_k2'], 0, 0, 0, 0]))
        time.sleep(0.18)
        os.write(out_fd, bytearray(8))
        time.sleep(0.20)

        # 4. Tap A
        os.write(out_fd, bytearray([0, 0, test['k2'], 0, 0, 0, 0, 0]))
        time.sleep(0.04)
        os.write(out_fd, bytearray(8))

        events = read_input_events(in_fd, timeout=0.8)
        down_codes = [c for c, v in events if v == KEY_DOWN]
        passed = (down_codes == test['expected_sequence'])
        detail = f"Sequence {test['expected_name']}" if passed else f"Expected {test['expected_name']}, got {events}"
        return passed, detail, events

    return False, "Unknown test type", []

def run_tests():
    print("=" * 65)
    print("      TFF AUTOMATED USB-OTG HARDWARE TEST SUITE")
    print("=" * 65)

    hidg_path = '/dev/hidg0'
    if not os.path.exists(hidg_path):
        print(f"Error: {hidg_path} does not exist. Run ./setup_fake_keyboard.sh first.")
        sys.exit(1)

    input_path = find_keyboard_input_device()
    if not input_path or not os.path.exists(input_path):
        print("Error: RP2040 Keyboard input device not found in /dev/input/by-id/.")
        sys.exit(1)

    filter_arg = sys.argv[1].lower() if len(sys.argv) > 1 else None
    active_tests = [t for t in TEST_CASES if not filter_arg or filter_arg in t['name'].lower()]

    print(f"USB-OTG Output Gadget: {hidg_path}")
    print(f"RP2040 Input Device:   {input_path}")
    print(f"Total Test Cases:      {len(active_tests)}")
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

    for idx, test in enumerate(active_tests, 1):
        test_name = test['name']
        print(f"[{idx:2d}/{len(active_tests):2d}] Testing: {test_name}...", end=' ', flush=True)

        passed, detail, events = execute_test_attempt(test, out_fd, in_fd)
        if not passed:
            # Retry once to handle transient USB host polling or GC pauses
            time.sleep(0.3)
            passed, detail, events = execute_test_attempt(test, out_fd, in_fd)

        if passed:
            print("PASS")
            passed_count += 1
            results.append((test_name, "PASS", detail))
        else:
            print(f"FAIL (got {events})")
            failed_count += 1
            results.append((test_name, "FAIL", detail))

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
    print(f"Total: {len(active_tests)} | Passed: {passed_count} | Failed: {failed_count}")
    print("=" * 65)

    if failed_count == 0:
        print("\033[92mALL AUTOMATED USB-OTG TFF TESTS PASSED SUCCESSFULLY! ✓\033[0m")
        sys.exit(0)
    else:
        print(f"\033[91m{failed_count} TEST(S) FAILED!\033[0m")
        sys.exit(1)

if __name__ == '__main__':
    run_tests()
