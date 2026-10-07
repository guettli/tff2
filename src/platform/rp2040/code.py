"""
TFF (Ten Flying Fingers) Firmware for Adafruit Feather RP2040 USB Host
Implements real-time keyboard remapping with the Go tff state machine algorithm.
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

class Event:
    def __init__(self, code, val, timestamp_ms):
        self.code = code
        self.val = val  # 1 for DOWN, 0 for UP
        self.time = timestamp_ms

class Combo:
    def __init__(self, keys, out_keys):
        self.keys = list(keys)
        self.out_keys = list(out_keys)

# TFF Combinations matching config/tff-combos.yaml
TFF_COMBOS = [
    # Home row index finger combos
    Combo([Keycode.J, Keycode.F], [Keycode.BACKSPACE]),      # j f -> backspace
    Combo([Keycode.F, Keycode.J], [Keycode.DELETE]),         # f j -> delete

    # Pinky combos
    Combo([Keycode.SEMICOLON, Keycode.A], [Keycode.HOME]),   # ; a -> home
    Combo([Keycode.A, Keycode.SEMICOLON], [Keycode.END]),    # a ; -> end

    # Navigation combos with F
    Combo([Keycode.F, Keycode.N], [Keycode.DOWN_ARROW]),     # f n -> down
    Combo([Keycode.F, Keycode.U], [Keycode.UP_ARROW]),       # f u -> up
    Combo([Keycode.F, Keycode.M], [Keycode.DOWN_ARROW]),     # f m -> down
    Combo([Keycode.F, Keycode.K], [Keycode.LEFT_ARROW]),     # f k -> left
    Combo([Keycode.F, Keycode.L], [Keycode.RIGHT_ARROW]),    # f l -> right
    Combo([Keycode.F, Keycode.I], [Keycode.PAGE_UP]),        # f i -> pageup
    Combo([Keycode.F, Keycode.COMMA], [Keycode.PAGE_DOWN]),  # f , -> pagedown

    # Escape combos
    Combo([Keycode.G, Keycode.H], [Keycode.ESCAPE]),         # g h -> esc
    Combo([Keycode.D, Keycode.F, Keycode.J], [Keycode.ESCAPE]), # d f j -> esc (triple combo)
]

class TFFEngine:
    def __init__(self, keyboard, combos):
        self.kbd = keyboard
        self.combos = combos
        self.buf = []
        self.down_keys_written = []
        self.swallow_keys = []
        self.active_timer_next = None

        self.timeout_after_down_ms = 150
        self.min_age_ms = 140
        self.min_overlap_ms = 40

    def check_timer(self, now):
        if self.active_timer_next is not None and now >= self.active_timer_next:
            self.eval(self.active_timer_next, "timer")
            self.active_timer_next = None

    def process_event(self, ev, now):
        self.check_timer(now)

        if ev.val == 1:  # DOWN
            self.active_timer_next = now + self.timeout_after_down_ms
            self.buf.append(ev)
            self.eval(ev.time, "down")
        elif ev.val == 0:  # UP
            self.buf.append(ev)
            self.eval(ev.time, "up")

    def eval(self, curr_time, reason):
        # 1. Single character tap check
        if (len(self.buf) == 2 and
            self.buf[0].code == self.buf[1].code and
            self.buf[0].val == 1 and self.buf[1].val == 0):

            code = self.buf[0].code
            if code in self.swallow_keys:
                self.swallow_keys = [k for k in self.swallow_keys if k != code]
                self.buf.clear()
                return
            self.flush_buffer("up-down-single")
            return

        # 2. Evaluate all combos
        codes = []
        for combo in self.combos:
            res = self.eval_combo(combo, curr_time)
            codes.append(res)

        # 3. Handle WriteUpKeys first
        found = False
        for i, res in enumerate(codes):
            if res != "WriteUpKeys":
                continue
            found = True
            combo = self.combos[i]
            self.write_combo_down(combo)
            self.write_combo_up(combo)
        if found:
            return

        # 4. Handle AllDownKeysSeen
        for i, res in enumerate(codes):
            if res != "AllDownKeysSeen":
                continue
            found = True
            combo = self.combos[i]
            if combo in self.down_keys_written:
                continue
            self.write_combo_down(combo)
            self.down_keys_written.append(combo)
        if found:
            return

        # 5. Handle ComboNotFinished
        for res in codes:
            if res == "ComboNotFinished":
                return

        # 6. No match: flush buffer
        self.flush_buffer("No-match")

    def eval_combo(self, combo, curr_time):
        seen_down = []
        seen_up = []
        last_down_ev = None
        first_up_ev = None
        has_unknown = False

        for ev in self.buf:
            if ev.code not in combo.keys:
                has_unknown = True
                break
            if ev.val == 1:
                last_down_ev = ev
                seen_down.append(ev.code)
            elif ev.val == 0:
                if first_up_ev is None:
                    first_up_ev = ev
                seen_up.append(ev.code)

        if not seen_down:
            return "NoMatch"
        if has_unknown:
            return "NoMatch"

        for i, key in enumerate(combo.keys):
            if i >= len(seen_down):
                return "ComboNotFinished"
            if seen_down[i] != key:
                return "NoMatch"

        # All down keys seen
        if first_up_ev and last_down_ev and last_down_ev.time < first_up_ev.time and last_down_ev.code != first_up_ev.code:
            overlap = first_up_ev.time - last_down_ev.time
            if overlap < self.min_overlap_ms:
                return "NoMatch"

        if self.too_young(last_down_ev, curr_time):
            return "ComboNotFinished"

        if seen_up:
            return "WriteUpKeys"

        if combo in self.down_keys_written:
            return "AllDownKeysSeenAndAlreadyWritten"

        return "AllDownKeysSeen"

    def too_young(self, last_down_ev, curr_time):
        if len(self.buf) > 1:
            if self.buf[-2].code == last_down_ev.code:
                return False
        age = curr_time - last_down_ev.time
        return age < self.min_age_ms

    def write_combo_down(self, combo):
        if combo in self.down_keys_written:
            return
        for out_key in combo.out_keys:
            self.kbd.press(out_key)

    def write_combo_up(self, combo):
        if combo in self.down_keys_written:
            self.down_keys_written.remove(combo)

        seen_up = [ev.code for ev in self.buf if ev.code in combo.keys and ev.val == 0]
        missing_up = [k for k in combo.keys if k not in seen_up]
        self.swallow_keys.extend(missing_up)

        self.buf = [ev for ev in self.buf if ev.code not in seen_up]
        for out_key in combo.out_keys:
            self.kbd.release(out_key)

    def flush_buffer(self, reason):
        for ev in self.buf:
            if ev.val == 1:
                self.kbd.press(ev.code)
            elif ev.val == 0:
                self.kbd.release(ev.code)
        self.buf.clear()
        self.active_timer_next = None

    def reset_if_empty(self):
        if not self.buf:
            self.down_keys_written.clear()
            self.swallow_keys.clear()
            self.kbd.release_all()

# Modifier bits mapping to keycodes
MODIFIERS = [
    (0x01, Keycode.LEFT_CONTROL),
    (0x02, Keycode.LEFT_SHIFT),
    (0x04, Keycode.LEFT_ALT),
    (0x08, Keycode.LEFT_GUI),
    (0x10, Keycode.RIGHT_CONTROL),
    (0x20, Keycode.RIGHT_SHIFT),
    (0x40, Keycode.RIGHT_ALT),
    (0x80, Keycode.RIGHT_GUI),
]

# 4. Connect to USB keyboard on Host port
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
prev_mods = 0
engine = TFFEngine(kbd, TFF_COMBOS)

print("TFF Remapper ACTIVE and listening for events.")

while True:
    now = int(time.monotonic() * 1000)
    engine.check_timer(now)

    try:
        count = device.read(0x81, buf, timeout=10)
        if count >= 8:
            now = int(time.monotonic() * 1000)
            mods = buf[0]
            if mods != prev_mods:
                for mask, keycode in MODIFIERS:
                    if (mods & mask) and not (prev_mods & mask):
                        kbd.press(keycode)
                    elif not (mods & mask) and (prev_mods & mask):
                        kbd.release(keycode)
                prev_mods = mods

            current_keys = [k for k in buf[2:8] if k != 0]

            new_keys = [k for k in current_keys if k not in prev_keys]
            released_keys = [k for k in prev_keys if k not in current_keys]

            for k in new_keys:
                engine.process_event(Event(k, 1, now), now)

            for k in released_keys:
                engine.process_event(Event(k, 0, now), now)

            if not current_keys and not prev_keys:
                engine.reset_if_empty()

            prev_keys = current_keys

    except usb.core.USBTimeoutError:
        pass
    except Exception as e:
        time.sleep(0.01)
