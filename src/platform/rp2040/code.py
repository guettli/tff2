"""
TFF (Ten Flying Fingers) Firmware for Adafruit Feather RP2040 USB Host
Implements real-time keyboard remapping with overlapping key combination detection.
"""

import array
import board
import digitalio
import time
import usb.core
import usb_hid
import usb_host
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keycode import Keycode
import adafruit_usb_host_descriptors

print("========================================")
print("   TFF RP2040 Keyboard Remapper       ")
print("========================================")

# 1. Enable 5V boost converter on USB-A host port
try:
    pwr = digitalio.DigitalInOut(board.USB_HOST_5V_POWER)
    pwr.switch_to_output(value=True)
    print("5V power enabled on USB Host port.")
except Exception as e:
    print(f"5V power notice: {e}")

# 2. Initialize PIO-USB Host port
host_port = usb_host.Port(board.USB_HOST_DATA_PLUS, board.USB_HOST_DATA_MINUS)
print("USB Host Port initialized.")

# 3. Initialize USB Device Keyboard (RP2040 -> Host PC/UpBoard)
kbd = Keyboard(usb_hid.devices)
print("USB Device Keyboard ready.")

# Timing threshold for overlapping key combinations
OVERLAP_THRESHOLD_MS = 120

# TFF Combinations map: (first_key, second_key) -> output_keycode
TFF_COMBOS = {
    # Home row index finger combos
    (Keycode.J, Keycode.F): Keycode.BACKSPACE,     # j f -> backspace
    (Keycode.F, Keycode.J): Keycode.DELETE,        # f j -> delete

    # Pinky combos
    (Keycode.SEMICOLON, Keycode.A): Keycode.HOME,  # ; a -> home
    (Keycode.A, Keycode.SEMICOLON): Keycode.END,   # a ; -> end

    # Navigation combos with F
    (Keycode.F, Keycode.N): Keycode.DOWN_ARROW,    # f n -> down
    (Keycode.F, Keycode.U): Keycode.UP_ARROW,      # f u -> up
    (Keycode.F, Keycode.M): Keycode.DOWN_ARROW,    # f m -> down
    (Keycode.F, Keycode.K): Keycode.LEFT_ARROW,    # f k -> left
    (Keycode.F, Keycode.L): Keycode.RIGHT_ARROW,   # f l -> right
    (Keycode.F, Keycode.I): Keycode.PAGE_UP,       # f i -> pageup
    (Keycode.F, Keycode.COMMA): Keycode.PAGE_DOWN, # f , -> pagedown

    # Escape combos
    (Keycode.G, Keycode.H): Keycode.ESCAPE,        # g h -> esc
}

# Connect to USB keyboard on Host port
print("Waiting for USB keyboard on Host port...")
device = None
while device is None:
    for d in usb.core.find(find_all=True):
        device = d
        break
    time.sleep(0.2)

print(f"Found USB device: VID=0x{device.idVendor:04x} PID=0x{device.idProduct:04x}")
try:
    config_desc = adafruit_usb_host_descriptors.get_configuration_descriptor(device, 0)
    device.set_configuration()
    if device.is_kernel_driver_active(0):
        device.detach_kernel_driver(0)
    print("Device configured successfully!")
except Exception as e:
    print(f"Configuration notice: {e}")

buf = array.array("B", [0] * 8)
prev_keys = []
key_down_time = {}
consumed_keys = set()
pending_keys = []

print("TFF Remapper ACTIVE and listening for events.")

while True:
    now = int(time.monotonic() * 1000)

    # 1. Check for pending keys whose overlap window has expired
    expired = []
    for k in pending_keys:
        if now - key_down_time.get(k, 0) > OVERLAP_THRESHOLD_MS:
            kbd.press(k)
            expired.append(k)
            print(f"Key held passthrough: 0x{k:02x}")

    for k in expired:
        pending_keys.remove(k)

    # 2. Read reports from USB Host
    try:
        count = device.read(0x81, buf, timeout=10)
        if count >= 8:
            current_keys = [k for k in buf[2:8] if k != 0]

            new_keys = [k for k in current_keys if k not in prev_keys]
            released_keys = [k for k in prev_keys if k not in current_keys]

            # Handle newly pressed keys in exact order
            for k in new_keys:
                key_down_time[k] = now
                combo_found = False

                # Check if this new key forms a combo with any pending key
                for prior_key in list(pending_keys):
                    diff = now - key_down_time.get(prior_key, 0)
                    if diff <= OVERLAP_THRESHOLD_MS:
                        pair = (prior_key, k)
                        if pair in TFF_COMBOS:
                            out_key = TFF_COMBOS[pair]
                            print(f"TFF COMBO: (0x{prior_key:02x}, 0x{k:02x}) -> 0x{out_key:02x} (diff={diff}ms)")
                            kbd.press(out_key)
                            time.sleep(0.01)
                            kbd.release(out_key)

                            consumed_keys.add(prior_key)
                            consumed_keys.add(k)
                            if prior_key in pending_keys:
                                pending_keys.remove(prior_key)
                            combo_found = True
                            break

                if not combo_found:
                    pending_keys.append(k)

            # Handle released keys
            for k in released_keys:
                if k in consumed_keys:
                    consumed_keys.remove(k)
                    if k in pending_keys:
                        pending_keys.remove(k)
                elif k in pending_keys:
                    # Released without combo -> emit single key!
                    print(f"TFF Single Tap: 0x{k:02x}")
                    kbd.press(k)
                    time.sleep(0.01)
                    kbd.release(k)
                    pending_keys.remove(k)
                else:
                    kbd.release(k)

                if k in key_down_time:
                    del key_down_time[k]

            # If all physical keys are now released, ensure all state is clean
            if not current_keys:
                pending_keys.clear()
                consumed_keys.clear()
                key_down_time.clear()
                kbd.release_all()

            prev_keys = current_keys

    except usb.core.USBTimeoutError:
        pass
    except Exception as e:
        time.sleep(0.01)
