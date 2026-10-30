#!/usr/bin/env python3
"""Generates src/core/tff_layout_tables.inc from the xkb keyboard layouts (libxkbcommon).

For every layout TFF supports (settings.layout), it lists each character that can be typed
on the main key block, as one or two key strokes: a key with Shift and/or the level-3 key
(AltGr; on Neo the Mod3 key), or a dead key followed by a key (dead ^ + space = "^",
dead acute + e = "é", resolved with the en_US.UTF-8 Compose table). Every entry is checked by
simulating its key presses with xkb.

Usage: tools/gen_layout_tables.py > src/core/tff_layout_tables.inc
"""
import ctypes
import ctypes.util
import sys

# TFF layout name -> (xkb layout, xkb variant, level-3 key name in tff_key_codes)
# us, dvorak and colemak are used on ANSI keyboards, which have no ISO <> key.
ANSI = {"us", "dvorak", "colemak"}
LAYOUTS = [
    ("us", "us", "", "KEY_RIGHTALT"),
    ("de", "de", "", "KEY_RIGHTALT"),
    ("fr", "fr", "", "KEY_RIGHTALT"),
    ("gb", "gb", "", "KEY_RIGHTALT"),
    ("es", "es", "", "KEY_RIGHTALT"),
    ("it", "it", "", "KEY_RIGHTALT"),
    ("neo", "de", "neo", "KEY_BACKSLASH"),
    ("dvorak", "us", "dvorak", "KEY_RIGHTALT"),
    ("colemak", "us", "colemak", "KEY_RIGHTALT"),
]

# evdev key codes of the main key block (digits row to bottom row, space, ISO <> key)
MAIN_KEYS = list(range(2, 14)) + list(range(16, 28)) + list(range(30, 42)) + \
    list(range(43, 54)) + [57, 86]
EVDEV_NAMES = {
    2: "1", 3: "2", 4: "3", 5: "4", 6: "5", 7: "6", 8: "7", 9: "8", 10: "9", 11: "0",
    12: "MINUS", 13: "EQUAL", 16: "Q", 17: "W", 18: "E", 19: "R", 20: "T", 21: "Y", 22: "U",
    23: "I", 24: "O", 25: "P", 26: "LEFTBRACE", 27: "RIGHTBRACE", 30: "A", 31: "S", 32: "D",
    33: "F", 34: "G", 35: "H", 36: "J", 37: "K", 38: "L", 39: "SEMICOLON", 40: "APOSTROPHE",
    41: "GRAVE", 43: "BACKSLASH", 44: "Z", 45: "X", 46: "C", 47: "V", 48: "B", 49: "N", 50: "M",
    51: "COMMA", 52: "DOT", 53: "SLASH", 57: "SPACE", 86: "102ND",
}
SHIFT, LEVEL3 = 1, 2

x = ctypes.CDLL(ctypes.util.find_library("xkbcommon"))


class RuleNames(ctypes.Structure):
    _fields_ = [(n, ctypes.c_char_p) for n in ("rules", "model", "layout", "variant", "options")]


x.xkb_context_new.restype = ctypes.c_void_p
x.xkb_keymap_new_from_names.restype = ctypes.c_void_p
x.xkb_keymap_new_from_names.argtypes = [ctypes.c_void_p, ctypes.POINTER(RuleNames), ctypes.c_int]
x.xkb_keymap_mod_get_index.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
x.xkb_keymap_num_levels_for_key.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32]
x.xkb_keymap_key_get_syms_by_level.argtypes = [
    ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32,
    ctypes.POINTER(ctypes.POINTER(ctypes.c_uint32))]
x.xkb_keymap_key_get_mods_for_level.argtypes = [
    ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint32,
    ctypes.POINTER(ctypes.c_uint32), ctypes.c_size_t]
x.xkb_keysym_to_utf32.argtypes = [ctypes.c_uint32]
x.xkb_keysym_to_utf32.restype = ctypes.c_uint32
x.xkb_compose_table_new_from_locale.restype = ctypes.c_void_p
x.xkb_compose_table_new_from_locale.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
x.xkb_compose_state_new.restype = ctypes.c_void_p
x.xkb_compose_state_new.argtypes = [ctypes.c_void_p, ctypes.c_int]
x.xkb_compose_state_feed.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
x.xkb_compose_state_get_status.argtypes = [ctypes.c_void_p]
x.xkb_compose_state_get_utf8.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]
x.xkb_compose_state_reset.argtypes = [ctypes.c_void_p]
x.xkb_state_new.restype = ctypes.c_void_p
x.xkb_state_new.argtypes = [ctypes.c_void_p]
x.xkb_state_unref.argtypes = [ctypes.c_void_p]
x.xkb_state_update_key.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_int]
x.xkb_state_key_get_one_sym.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
x.xkb_state_key_get_one_sym.restype = ctypes.c_uint32
COMPOSE_NOTHING, COMPOSE_COMPOSED = 0, 2
KEY_LEFTSHIFT = 42
LEVEL3_CODES = {"KEY_RIGHTALT": 100, "KEY_BACKSLASH": 43}

ctx = x.xkb_context_new(0)
compose = x.xkb_compose_state_new(
    x.xkb_compose_table_new_from_locale(ctx, b"en_US.UTF-8", 0), 0)


def compose_pair(dead, sym):
    x.xkb_compose_state_reset(compose)
    x.xkb_compose_state_feed(compose, dead)
    x.xkb_compose_state_feed(compose, sym)
    if x.xkb_compose_state_get_status(compose) != COMPOSE_COMPOSED:
        return None
    buf = ctypes.create_string_buffer(16)
    x.xkb_compose_state_get_utf8(compose, buf, 16)
    s = buf.value.decode()
    return s if len(s) == 1 else None


def load_keymap(layout, variant):
    names = RuleNames(b"evdev", b"pc105", layout.encode(), variant.encode(), b"")
    km = x.xkb_keymap_new_from_names(ctx, ctypes.byref(names), 0)
    if not km:
        sys.exit(f"xkb layout {layout}({variant}) not found")
    return km


def typed(km, level3_code, strokes):
    """Simulates the key presses (Shift = left shift, level 3 = level3_code) and returns the
    character they type, or None."""
    state = x.xkb_state_new(km)
    x.xkb_compose_state_reset(compose)
    out = None
    for code, flags in strokes:
        held = ([KEY_LEFTSHIFT] if flags & SHIFT else []) + ([level3_code] if flags & LEVEL3 else [])
        for k in held:
            x.xkb_state_update_key(state, k + 8, 1)
        sym = x.xkb_state_key_get_one_sym(state, code + 8)
        for k in reversed(held):
            x.xkb_state_update_key(state, k + 8, 0)
        x.xkb_compose_state_feed(compose, sym)
        status = x.xkb_compose_state_get_status(compose)
        out = None
        if status == COMPOSE_NOTHING:
            cp = x.xkb_keysym_to_utf32(sym)
            out = chr(cp) if cp else None
        elif status == COMPOSE_COMPOSED:
            buf = ctypes.create_string_buffer(16)
            x.xkb_compose_state_get_utf8(compose, buf, 16)
            out = buf.value.decode()
    x.xkb_state_unref(state)
    return out


def keys_of(km):
    """Yields (evdev code, flags, keysym) for every key/level reachable with Shift/level 3."""
    shift = 1 << x.xkb_keymap_mod_get_index(km, b"Shift")
    level3 = 1 << x.xkb_keymap_mod_get_index(km, b"Mod5")
    for code in MAIN_KEYS:
        kc = code + 8
        for level in range(x.xkb_keymap_num_levels_for_key(km, kc, 0)):
            syms = ctypes.POINTER(ctypes.c_uint32)()
            if x.xkb_keymap_key_get_syms_by_level(km, kc, 0, level, ctypes.byref(syms)) != 1:
                continue
            masks = (ctypes.c_uint32 * 8)()
            n = x.xkb_keymap_key_get_mods_for_level(km, kc, 0, level, masks, 8)
            best = None
            for mask in masks[:n]:
                if mask & ~(shift | level3):
                    continue  # needs another modifier (Lock, level 5, ...)
                flags = (SHIFT if mask & shift else 0) | (LEVEL3 if mask & level3 else 0)
                if best is None or flags < best:
                    best = flags
            if best is not None:
                yield code, best, syms[0]


def is_dead(sym):
    return 0xFE50 <= sym <= 0xFE8F


def table(name, layout, variant, level3):
    km = load_keymap(layout, variant)
    keys = list(keys_of(km))
    avoid_iso = name in ANSI
    chars = {}  # code point -> strokes, fewest modifiers first

    def add(cp, strokes):
        # Fewest strokes, then avoid the ISO <> key on ANSI layouts, then the fewest modifiers
        # (Shift before level 3), then the lowest key code
        cost = (len(strokes), avoid_iso and any(c == 86 for c, _ in strokes),
                sum(f for _, f in strokes),
                [c for c, _ in strokes])
        if cp not in chars or cost < chars[cp][0]:
            chars[cp] = (cost, strokes)

    for code, flags, sym in keys:
        cp = x.xkb_keysym_to_utf32(sym)
        if not is_dead(sym) and cp >= 0x20 and cp != 0x7F and cp <= 0xFFFF:
            add(cp, [(code, flags)])
    for dcode, dflags, dsym in keys:
        if not is_dead(dsym):
            continue
        for code, flags, sym in keys:
            if is_dead(sym):
                continue
            r = compose_pair(dsym, sym)
            # Dead keys: ASCII, Latin-1 and Latin Extended-A (keeps the tables small)
            if r and 0x20 <= ord(r) <= 0x17F and ord(r) != 0x7F:
                add(ord(r), [(dcode, dflags), (code, flags)])
    # Keep only what really types the character with the level-3 key used (on us and dvorak,
    # Right Alt is a plain Alt key, so their level-3 characters can't be typed)
    level3_code = LEVEL3_CODES[level3]
    return {cp: s for cp, (_, s) in chars.items() if typed(km, level3_code, s) == chr(cp)}


def main():
    out = sys.stdout
    out.write("// Generated by tools/gen_layout_tables.py from the xkb layouts. Do not edit.\n")
    out.write("// {code point, key, flags, dead-key key, dead-key flags};"
              " flags: 1 = Shift, 2 = level 3 (AltGr)\n\n")
    for name, layout, variant, level3 in LAYOUTS:
        t = table(name, layout, variant, level3)
        out.write(f"static const LayoutChar kLayout_{name}[] = {{\n")
        for cp in sorted(t):
            s = t[cp]
            key, flags = s[-1]
            dkey, dflags = (s[0] if len(s) == 2 else (0, 0))
            dname = f"Keys::KEY_{EVDEV_NAMES[dkey]}" if dkey else "0"
            ch = chr(cp)
            comment = f"  // {ch}" if ch.isprintable() and ch not in "\\" else ""
            out.write(f"    {{0x{cp:04X}, Keys::KEY_{EVDEV_NAMES[key]}, {flags}, {dname}, "
                      f"{dflags}}},{comment}\n")
        out.write("};\n\n")
    out.write("static const LayoutTable kLayoutTables[] = {\n")
    for name, _, _, level3 in LAYOUTS:
        out.write(f"    {{\"{name}\", kLayout_{name}, sizeof(kLayout_{name}) / "
                  f"sizeof(kLayout_{name}[0]), Keys::{level3}}},\n")
    out.write("};\n")


if __name__ == "__main__":
    main()
