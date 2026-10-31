#!/usr/bin/env python3
"""Linux end-to-end test: a virtual keyboard (uinput) types into the real tff_linux, which writes
to its "TFF Virtual Keyboard"; the output is decoded with xkbcommon (layout + Compose) into text.

Run as root (uinput, grabbing devices):
    sudo python3 tests/linux/e2e.py build/tff_linux [case-name-filter]
V=1 prints text and key trace of passing cases too. Needs libxkbcommon and the en_US.UTF-8
Compose table (xkb-data, libx11-data). The cases are in e2e_cases.py."""
import ctypes
import fcntl
import glob
import os
import re
import select
import struct
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools"))
import gen_layout_tables as g  # noqa: E402  (xkbcommon via ctypes, Compose table)

x = g.x

KEYS = {}
for line in open("/usr/include/linux/input-event-codes.h"):
    m = re.match(r"#define KEY_(\w+)\s+(0x[0-9a-fA-F]+|\d+)\b", line)
    if m:
        KEYS[m.group(1).lower()] = int(m.group(2), 0)
NAMES = {}
for k, v in KEYS.items():
    NAMES.setdefault(v, k)

EV_SYN, EV_KEY = 0, 1
UI_SET_EVBIT, UI_SET_KEYBIT = 0x40045564, 0x40045565
UI_DEV_SETUP, UI_DEV_CREATE, UI_DEV_DESTROY = 0x405C5503, 0x5501, 0x5502
EVIOCGRAB = 0x40044590
EVFMT = "llHHi"
EVSZ = struct.calcsize(EVFMT)
SRC_NAME = "tff-e2e-src"
OUT_NAME = "TFF Virtual Keyboard"


def make_source():
    fd = os.open("/dev/uinput", os.O_WRONLY | os.O_NONBLOCK)
    fcntl.ioctl(fd, UI_SET_EVBIT, EV_KEY)
    for code in range(1, 249):
        fcntl.ioctl(fd, UI_SET_KEYBIT, code)
    fcntl.ioctl(fd, UI_DEV_SETUP, struct.pack("HHHH80sI", 3, 0x1111, 0x2222, 1, SRC_NAME.encode(), 0))
    fcntl.ioctl(fd, UI_DEV_CREATE)
    return fd


def devices(name):
    found = set()
    for p in glob.glob("/sys/class/input/event*/device/name"):
        try:
            if open(p).read().strip() == name:
                found.add("/dev/input/" + p.split("/")[4])
        except OSError:
            pass  # removed meanwhile
    return found


def find_new_device(name, before, proc=None, timeout=10):
    """A device called `name` that isn't in `before` (e.g. not the one of a running TFF)."""
    end = time.time() + timeout
    while time.time() < end:
        if proc is not None and proc.poll() is not None:
            return None
        new = devices(name) - before
        if new:
            return max(new, key=lambda d: int(d.rsplit("event", 1)[1]))
        time.sleep(0.05)
    raise RuntimeError("input device not found: " + name)


def wait_for_grab(dev, proc, timeout=10):
    """True once another process (tff_linux) grabbed `dev`: our own grab then fails."""
    end = time.time() + timeout
    while time.time() < end and proc.poll() is None:
        fd = os.open(dev, os.O_RDONLY | os.O_NONBLOCK)
        try:
            fcntl.ioctl(fd, EVIOCGRAB, 1)
            fcntl.ioctl(fd, EVIOCGRAB, 0)
        except OSError:
            return True
        finally:
            os.close(fd)
        time.sleep(0.02)
    return False


def emit(fd, code, value):
    os.write(fd, struct.pack(EVFMT, 0, 0, EV_KEY, code, value))
    os.write(fd, struct.pack(EVFMT, 0, 0, EV_SYN, 0, 0))


class Decoder:
    """Turns key events into text like an X11/Wayland client with the given xkb layout."""

    def __init__(self, layout, variant):
        names = g.RuleNames(b"evdev", b"pc105", layout.encode(), variant.encode(), b"")
        self.keymap = x.xkb_keymap_new_from_names(g.ctx, ctypes.byref(names), 0)
        if not self.keymap:
            raise RuntimeError("xkb layout not found: %s(%s)" % (layout, variant))
        self.state = x.xkb_state_new(self.keymap)
        x.xkb_compose_state_reset(g.compose)
        self.text = ""

    def key(self, code, value):
        if value == 2:
            return  # autorepeat
        x.xkb_state_update_key(self.state, code + 8, 1 if value == 1 else 0)
        if value != 1:
            return
        sym = x.xkb_state_key_get_one_sym(self.state, code + 8)
        x.xkb_compose_state_feed(g.compose, sym)
        status = x.xkb_compose_state_get_status(g.compose)
        if status == 0:  # nothing to compose
            cp = x.xkb_keysym_to_utf32(sym)
            if cp:
                self.text += chr(cp)
        elif status == 2:  # composed
            buf = ctypes.create_string_buffer(16)
            x.xkb_compose_state_get_utf8(g.compose, buf, 16)
            self.text += buf.value.decode()
            x.xkb_compose_state_reset(g.compose)
        elif status == 3:  # cancelled
            x.xkb_compose_state_reset(g.compose)


def run_case(tff, case, workdir):
    """Steps: "d:key" down, "u:key" up, "t:key" tap, "r:key" autorepeat, "<ms>" wait.
    Steps joined by "," are sent back to back ("d:k,u:d": k pressed as d is released)."""
    cfg = os.path.join(workdir, "cfg.yaml")
    log_path = os.path.join(workdir, "tff.log")
    with open(cfg, "w") as f:
        f.write(case["config"])
    events = []
    src_before = devices(SRC_NAME)
    out_before = devices(OUT_NAME)
    src = make_source()
    ofd = None
    try:
        with open(log_path, "w") as log:
            src_dev = find_new_device(SRC_NAME, src_before)
            proc = subprocess.Popen([tff, cfg, src_dev], stdout=log, stderr=subprocess.STDOUT)
            try:
                out = find_new_device(OUT_NAME, out_before, proc=proc)
                if out is None or not wait_for_grab(src_dev, proc):
                    return "", "tff_linux exited", open(log_path).read()
                ofd = os.open(out, os.O_RDONLY | os.O_NONBLOCK)
                fcntl.ioctl(ofd, EVIOCGRAB, 1)  # keep the output away from the console

                def drain(seconds):
                    end = time.time() + seconds
                    while (left := end - time.time()) > 0:
                        if not select.select([ofd], [], [], left)[0]:
                            continue
                        data = os.read(ofd, EVSZ * 64)
                        for i in range(0, len(data), EVSZ):
                            _, _, typ, code, value = struct.unpack(EVFMT, data[i:i + EVSZ])
                            if typ == EV_KEY:
                                events.append((code, value))

                drain(0.1)
                for step in case["steps"].split():
                    if step.isdigit():
                        drain(int(step) / 1000)
                        continue
                    for part in step.split(","):
                        action, key = part.split(":")
                        code = KEYS[key]
                        if action == "r":
                            emit(src, code, 2)
                        if action in ("d", "t"):
                            emit(src, code, 1)
                        if action == "t":
                            drain(0.012)
                        if action in ("u", "t"):
                            emit(src, code, 0)
                    drain(0.03 if action == "r" else 0.012)
                drain(0.6)
            finally:
                proc.terminate()
                try:
                    proc.wait(5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
    finally:
        if ofd is not None:
            os.close(ofd)
        fcntl.ioctl(src, UI_DEV_DESTROY)
        os.close(src)
    decoder = Decoder(*case.get("xkb", ("us", "")))
    trace = []
    for code, value in events:
        decoder.key(code, value)
        trace.append(NAMES.get(code, str(code)) + {0: "/", 1: "_", 2: "*"}[value])
    return decoder.text, " ".join(trace), open(log_path).read()


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    tff = os.path.abspath(sys.argv[1])
    name_filter = sys.argv[2] if len(sys.argv) > 2 else ""
    cases = {}
    exec(open(os.path.join(HERE, "e2e_cases.py")).read(), cases)
    failures = 0
    selected = [c for c in cases["CASES"] if name_filter in c["name"]]
    if not selected:
        sys.exit("no case matches " + repr(name_filter))
    with tempfile.TemporaryDirectory() as workdir:
        for case in selected:
            assert "text" in case or "trace" in case, case["name"] + ": no expected result"
            try:
                text, trace, log = run_case(tff, case, workdir)
            except Exception as e:  # report and go on with the next case
                text, trace, log = "", "", "%s: %s" % (type(e).__name__, e)
            ok = case.get("text", text) == text and case.get("trace", trace) == trace
            print(("PASS " if ok else "FAIL ") + case["name"])
            if not ok or os.environ.get("V"):
                print("   text :", repr(text))
                print("   trace:", trace)
            if not ok:
                print("   want :", repr(case.get("text", "")), case.get("trace", ""))
                print("   log  :", log[-1500:].replace("\n", "\n          "))
                failures += 1
    print("failures:", failures)
    sys.exit(1 if failures else 0)


main()
