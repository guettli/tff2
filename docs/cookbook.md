# TFF2 Configuration Cookbook & Real-World Recipes

This cookbook provides production-tested configurations, tips, and copy-pasteable recipes for common workflows using Ten Flying Fingers (TFF).

---

## Recipe 1: The Vim & Developer Powerhouse

**Goal:** Turn Caps Lock into a dual-role key (Escape on tap, Super/Windows on hold) and map home-row keys (`h`, `j`, `k`, `l`) to cursor navigation.

```yaml
combos:
  # Home-row cursor arrows via symmetric chords
  h + j: left
  j + k: down
  k + l: up
  j + l: right

  # Fast word navigation
  f + h: home
  f + l: end
  f + u: pageup
  f + d: pagedown

  # Quick backspace / delete from home row
  j + f: backspace
  f + j: delete

tap_hold:
  # Caps Lock: tap for Escape (Vim normal mode), hold for Super / Mod key
  - key: capslock
    tap: esc
    hold: leftmeta
    timeout_ms: 190
```

---

## Recipe 2: Coding Snippets & Boilerplate Macros

**Goal:** Insert commonly used language boilerplate, brackets, and syntax structures using multi-character text snippets.

```yaml
combos:
  # JavaScript/TypeScript arrow function
  a + f: "() => "

  # Rust / C++ scope resolution and arrows
  c + o: "::"
  r + a: " -> "

  # Equality and comparison operators
  e + q: " === "
  n + e: " !== "

  # Print statement boilerplate
  p + r: "console.log();"
```

---

## Recipe 3: Tiling Window Manager Chords (i3 / Sway / Hyprland)

**Goal:** Control workspace layouts, spawn terminals, and close windows with ergonomic two-finger chords rather than awkward three-key modifier gymnastics.

```yaml
combos:
  # Spawn terminal (Mod + Enter)
  w + e: leftmeta+enter

  # Kill focused window (Mod + Shift + Q)
  q + w: leftmeta+leftshift+q

  # Toggle full screen (Mod + F)
  s + d: leftmeta+f

  # Workspace switching (Mod + 1, Mod + 2, Mod + 3)
  1 + 2: leftmeta+1
  2 + 3: leftmeta+2
  3 + 4: leftmeta+3

  # Audio volume adjustment without reaching for Fn keys
  v + u: volumeup
  v + d: volumedown
  v + m: mute
```

---

## Recipe 4: German (QWERTZ) & International Programmer Layout

**Goal:** On German and European keyboard layouts, brackets (`[]`), braces (`{}`), backslash (`\`), and pipe (`|`) require cumbersome `AltGr` combinations. Map them to simple home-row chords.

```yaml
combos:
  # Brackets and Braces
  u + i: "["
  i + o: "]"
  u + o: "{"
  i + p: "}"

  # Common symbols
  s + l: "/"
  b + s: "\\"
  p + i: "|"
  t + i: "~"
```

---

## Recipe 5: Sequential Leader Sequences

**Goal:** Execute complex multi-step sequences inspired by Vim leader keys. Press a designated leader key (e.g. `capslock` or `space`), followed by a sequence of keystrokes.

```yaml
combos:
  d + f: esc

leader:
  key: capslock
  timeout_ms: 1000
  sequences:
    "w s": leftctrl+s
    "g s": "git status\n"
    "g c": "git commit -m \""
```

---

## Recipe 6: Home-Row Mouse Navigation

**Goal:** Control mouse cursor movement, left/right clicks, and scrolling directly from the keyboard home row without reaching for a physical mouse.

```yaml
combos:
  d + f + m: esc

modal_layers:
  - name: mouse_nav
    hold_key: rightalt
    mappings:
      h: mouse_left
      j: mouse_down
      k: mouse_up
      l: mouse_right
      u: mouse_wheel_up
      d: mouse_wheel_down
      space: mouse_btn_left
      f: mouse_btn_right
      s: mouse_btn_middle
```

---

## Recipe 7: Auto-Shift for High-Speed Typists

**Goal:** Eliminate physical Shift key usage for letters and symbols: tapping emits lowercase, holding for >175ms automatically shifts to uppercase.

```yaml
combos:
  d + f: esc

auto_shift:
  enabled: true
  timeout_ms: 175
  keys: letters
```

---

## Recipe 8: Complete Integrated Developer Configuration

Save this complete configuration to `~/.config/tff/tff-combos.yaml`:

```yaml
settings:
  combo_timeout_ms: 40
  tap_hold_timeout_ms: 190

tap_hold:
  - key: capslock
    tap: esc
    hold: leftmeta
    timeout_ms: 190

combos:
  # Chords
  j + f: backspace
  f + j: delete
  d + f + j: esc

  # Cursor arrows
  h + j: left
  j + k: down
  k + l: up
  j + l: right

  # Code snippets
  a + f: "() => "
  r + a: " -> "

  # Window management
  w + e: leftmeta+enter
  q + w: leftmeta+leftshift+q

  # Volume
  v + u: volumeup
  v + d: volumedown
  v + m: mute
```

Validate and verify this configuration with:
```bash
tff validate ~/.config/tff/tff-combos.yaml
tff cheatsheet ~/.config/tff/tff-combos.yaml
```
