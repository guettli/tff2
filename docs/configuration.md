# Configuration System Documentation

## Overview

The configuration system allows users to customize keyboard chord combinations (combos) for Ten Flying Fingers (TFF). It uses human-readable YAML files that define multi-key chords and their mapped output key actions.

Configurations are parsed natively in C++ using a lightweight parser (`tff::loadYamlCombos`) that supports the standard TFF combo format.

## Configuration File Structure

The configuration file contains a list of combos under the `combos:` root key.

### Basic Structure

```yaml
combos:
  # Home row index finger combos
  - keys: j f
    outKeys: backspace

  - keys: f j
    outKeys: delete

  # Pinky combos
  - keys: semicolon a
    outKeys: home

  - keys: a semicolon
    outKeys: end

  # Navigation combos with F
  - keys: f n
    outKeys: down

  - keys: f u
    outKeys: up

  - keys: f k
    outKeys: left

  - keys: f l
    outKeys: right
```

### Formats Supported

TFF supports two equivalent YAML configuration formats:

#### 1. Compact Shorthand (Recommended)
Concise key-value mappings with zero boilerplate:

```yaml
combos:
  j f: backspace
  f j: delete
  ; a: home
  a ;: end
  g + h: esc       # Symmetric 2-key chord: g and h in any order
  d + f + j: esc   # Triple chord: d, f, and j pressed together in any order
  f space: ctrl+s  # Hotkey action
```

##### Simultaneous & Symmetric Chords (`+`)
When keys are joined with `+`, TFF treats them as an order-independent chord:
- **2-key chords**: `g + h: esc` (matches `g h` or `h g`)
- **3-key chords (Triple combos)**: `d + f + j: esc` (matches all 6 arrival permutations of `d`, `f`, and `j`)
- **4-key chords**: `a + s + d + f: mute` (matches all 24 arrival permutations)
- Supported in both compact (`d + f + j: esc`) and classic list format (`- keys: d + f + j \n outKeys: esc`).

##### Leader Key Grouping
Group related chords sharing an anchor key (e.g., `f` for navigation):

```yaml
combos:
  f:
    n: down
    u: up
    k: left
    l: right
    i: pageup
    ,: pagedown
```

#### 2. Classic List Format
```yaml
combos:
  - keys: j f
    outKeys: backspace
  - keys: f j
    outKeys: delete
```

## Text Snippets & Multi-Character Macro Expansion

TFF supports expanding multi-character text strings and code snippets directly from chords without needing external clipboard tools or third-party macro software.

### Syntax Options

#### 1. Compact Direct Quotes (Recommended)
Simply enclose the desired string in double (`"..."`) or single (`'...'`) quotes:

```yaml
combos:
  f n: "println!(\"\");"
  j k: 'Hello World'
  g s: "git status"
```

#### 2. Inline Dictionary or Explicit Attribute
Use `{ text: "..." }` or `{ type: "..." }`:

```yaml
combos:
  p y: { text: "import sys\nimport os" }
  c m: { type: "cargo test" }
```

#### 3. Symmetric Chording (`+`) and Leader Key Grouping
Snippets seamlessly compose with symmetric chords and leader key blocks:

```yaml
combos:
  # Symmetric chord: pressing d and f together in any order
  d + f: "alice@example.com"

  # Leader prefix: Space followed by a single key
  space:
    e: "alice@example.com"
    g: "git commit -m \"\""
    p: "println!(\"{}\", val);"
```

#### 4. Classic List Format
```yaml
combos:
  - in: [f, n]
    text: "println!(\"\");"
  - keys: c m
    type: "#include <iostream>"
```

### Supported Characters and Modifiers

TFF translates text strings directly into synthesized Linux kernel `evdev` input events:
- **Letters & Digits**: `a`-`z`, `A`-`Z` (automatically activates and releases `KEY_LEFTSHIFT`), `0`-`9`.
- **Whitespace & Control**: Space, `\t` (Tab), `\n` (Enter).
- **Punctuation & Symbols**: All standard ASCII symbols (`,`, `.`, `;`, `:`, `!`, `@`, `#`, `$`, `%`, `^`, `&`, `*`, `(`, `)`, `_`, `+`, `-`, `=`, `{`, `}`, `[`, `]`, `|`, `\`, `"`, `'`, `<`, `>`, `?`, `~`, `` ` ``).
- **Clean Chord Release**: When chord keys are released after triggering a text snippet, they are swallowed cleanly so that no physical chord keys leak into your text buffer or editor.

> [!NOTE]
> **Keyboard Layout Baseline**: Because TFF operates at the kernel `evdev` level before desktop environment layout translation, synthesized punctuation symbols assume standard US QWERTY keycode locations. If your desktop session uses a non-US keymap (e.g. German QWERTZ or French AZERTY), the desktop environment will translate those scancodes according to your active layout.

## Tap-vs-Hold Keys (Dual-Role Keys)

Tap-vs-Hold allows a physical key to perform two completely different functions based on how it is pressed:
- **Tap**: Pressing and quickly releasing the key sends a tap key (e.g., `esc`).
- **Hold**: Holding the key down longer than the timeout (default: 200ms), or pressing any other key while holding it, emits a hold modifier (e.g., `super` / Windows key).

### Configuration Syntax

#### Multi-line Property Block
```yaml
tap_hold:
  capslock:
    tap: esc
    hold: super
    timeout_ms: 200
```

#### Compact Inline Format
```yaml
tap_hold:
  capslock: [esc, super, 200]
```

### Chording and Permissive Hold
When a tap-hold key is pressed and another key is tapped (e.g., CapsLock + `c`), TFF immediately promotes CapsLock to Super/Windows key without waiting for the timeout to elapse. This ensures instantaneous responsiveness for modifier combinations like Win+D, Win+Tab, or Win+Arrow.

> [!NOTE]
> **Key Rollover**: Because chords immediately promote to hold, typists rolling keys very rapidly when tapping Escape in modal editors (e.g. Vim) should release CapsLock before pressing the next key to ensure it registers as a tap rather than a chord modifier.

### Validation Rules
- `tap_hold` definitions require both `tap` and `hold` (or `layer`) targets.
- `timeout_ms` must be a positive integer (default: 200 ms).
- A key configured under `tap_hold` cannot also be part of a `combos:` chord to prevent ambiguous overlapping triggers.

## Modal Keyboard Layers

Modal layers allow you to temporarily remap keyboard keys into specialized layouts (such as navigation clusters, numeric keypads, or symbol sets) while a designated layer key is held, similar to advanced custom keyboard firmware (QMK, ZMK, KMonad).

### Defining Layers

Define one or more named layers under the `layers:` root key:

```yaml
layers:
  nav:
    h: left
    j: down
    k: up
    l: right
    w: ctrl+right
    b: ctrl+left
    d: delete
    c: "println!();"
  numpad:
    m: 0
    j: 1
    k: 2
    l: 3
    u: 4
    i: 5
    o: 6
```

Each layer maps input keys to:
- Single keys (e.g. `h: left`)
- Hotkey combos and modifier chords (e.g. `w: ctrl+right`)
- Text snippets (e.g. `c: "println!();"`)

### Activating Layers with Tap-vs-Hold

Connect any layer to a momentary trigger using `tap_hold:`:

```yaml
tap_hold:
  space:
    tap: space
    layer: nav
    timeout_ms: 200

  # Or classic list syntax:
  - key: rightalt
    tap: rightalt
    layer: numpad

  # Or compact inline syntax:
  tab: [tab, nav, 250]
```

### Key Behaviors

- **Instant Permissive Chording**: When a tap-hold layer key (like Space) goes down and another key is tapped, the layer activates immediately with zero latency.
- **Key Passthrough**: Any key not explicitly mapped within an active layer passes through transparently to base typing and combos.
- **Stuck-Key Prevention**: If you press a key while a layer is active (e.g. holding Space and pressing `h` to send Left Arrow) and release Space before releasing `h`, TFF guarantees that Left Arrow UP is synthesized upon releasing `h`, preventing any stuck or hanging keys.
- **Layer Stacking (LIFO)**: If multiple layers are active simultaneously, key lookup resolves from the top of the stack downwards (most recently activated layer has priority).
- **Dual-Role Tapping**: Tapping Space quickly without pressing another key emits a normal Space keystroke.

### Toggle / Locking Layers (`toggle_layer` / `tg`)

Toggle (or locking) layers allow you to lock a layer in an active state persistently without having to keep any physical key held down. This is ideal for:
- Entering large amounts of numeric data using a **Numpad** layer.
- Switching to a dedicated **Gaming** layer where standard combos or navigation layers should be replaced.
- Enabling an alternative **Symbol** layout during code editing sessions.

Toggling a layer on pushes it onto the active layer stack; toggling it again pops it off.

#### Toggle Layer Syntax

You can trigger `toggle_layer(name)` (or shorthand `tg(name)`) from combos, layer key mappings, tap-hold dual-role keys, and sequential leader shortcuts:

##### 1. Combos
```yaml
combos:
  # Press F and Space simultaneously to toggle numpad layer on/off
  f + space: toggle_layer(numpad)
  # Or using shorthand tg():
  j + space: tg(nav)
```

##### 2. Layer Remaps (Unlocking / Exiting)
A key inside a layer can toggle the layer off (e.g. Escape to exit Numpad mode):
```yaml
layers:
  numpad:
    esc: toggle_layer(numpad)   # Pressing Escape exits numpad mode
    j: "1"
    k: "2"
    l: "3"
```

##### 3. Tap-Hold Dual-Role Keys
Tap to toggle the layer on/off, hold for a modifier:
```yaml
tap_hold:
  capslock: [toggle_layer(numpad), super, 200]
```

##### 4. Leader Key Sequences
Type a mnemonic sequence to toggle the layer:
```yaml
leader:
  key: capslock
  sequences:
    "n p": toggle_layer(numpad)
```

#### Layer Stacking & Safety
- **LIFO Layer Stacking**: If you lock a layer on (e.g. `numpad`) and then momentarily hold another layer key (e.g. `space` for `nav`), the momentary layer takes precedence. When you release `space`, the `numpad` layer remains locked on.
- **Key Release Safety**: If a physical key is pressed while a layer is toggled and released after the layer is toggled off, TFF safely swallows or cleanly releases the mapped scancode so no modifier or layer keys become stuck.

### Layer-Scoped Combos

Combos can be scoped to specific modal layers so they only trigger when that layer is active (either momentarily held or toggled). This enables ergonomic chords inside layers (e.g. Navigation or Numpad) without conflicting with your global base-layer combos.

#### 1. Inlined Directly Inside Layer Definition
You can define chords directly within any layer definition under `layers:` using symmetric (`+`) or order-dependent chords:

```yaml
layers:
  nav:
    # Single key mappings
    h: left
    j: down
    k: up
    l: right

    # Layer-scoped chords: active only while 'nav' layer is active!
    h + l: end
    h + k: home
    j + k: pagedown
    w + b: { text: "void" }
    m + n: mouse_btn_left
```

#### 2. Top-Level `combos:` with Explicit Layer Scope
Alternatively, layer-scoped combos can be placed in the `combos:` section by specifying `layer: <layer_name>`:

```yaml
combos:
  # Global combo (active across all layers)
  d + f: esc

  # Layer-scoped combo (active only when 'nav' layer is active)
  h + l:
    out: end
    layer: nav

  # Inline compact format:
  j + k: { out: pagedown, layer: nav }

  # Classic list format:
  - in: [h, l]
    out: end
    layer: nav
```

#### Priority and Layer Stacking
- **Active Layer Priority**: When a layer is active, combos scoped to that layer take precedence over global combos if the same key combination is defined in both.
- **Nested Layer Stacking**: If multiple layers are active simultaneously (e.g. toggled layer + momentary layer), combos scoped to the topmost active layer take precedence.
- **Inactive Layers**: If a layer is not active, any combos scoped to that layer are completely ignored. If one of the keys is pressed, it falls back to base typing or global combos.
- **Validation**: Any layer referenced by `layer: <name>` must be defined in the `layers:` section; referencing an undefined layer raises a configuration validation error.

### Validation Rules
- Any layer referenced by `layer:` in `tap_hold`, `combos:`, or `toggle_layer(...)` in combos, layers, tap-hold, or leader sequences must be defined in `layers:`.
- Duplicate layer names and duplicate key mappings within a layer are rejected.
- Keys inside layer mappings cannot have empty outputs.

## One-Shot / Sticky Modifiers and Layers (OSM & OSL)

One-shot keys (also known as "sticky keys") eliminate finger contortions by letting you tap a modifier or layer switch *before* typing the key you want to modify, instead of having to hold both keys simultaneously.

- **One-Shot Modifier (OSM)**: Quickly tap `shift` and release it. The next key you type (e.g. `a`) is automatically shifted (`A`). Once the key is released, `shift` automatically disengages.
- **One-Shot Layer (OSL)**: Quickly tap `space` and release it. The next key you type is dispatched from the specified layer (e.g. `k` sends `up`), after which the layer automatically deactivates.
- **Chaining**: You can chain multiple one-shot modifiers sequentially. For example, tap `ctrl`, then tap `shift`, then press `t` -> emits `Ctrl + Shift + T`.
- **Hold & Fast Chording**: If you press and hold a one-shot key, or chord it with another key while held, it acts immediately as a standard sustained modifier or momentary layer.
- **Expiration Timeout**: If you tap a one-shot key but do not type another key within the configured timeout (default: 1500 ms), the one-shot state expires automatically without emitting phantom keystrokes or leaving modifier flags stuck.

### Configuration Syntax

#### 1. Top-Level `one_shot:` Section (Compact Dictionary)

```yaml
one_shot:
  leftshift: 1500    # Tap Shift -> next key is capitalized (expires in 1500ms)
  leftctrl: 1200     # Tap Ctrl -> next key has Ctrl modifier
  space: [nav, 2000] # Tap Space -> next single key from 'nav' layer (expires in 2000ms)
```

#### 2. Top-Level `one_shot:` Section (List Format)

```yaml
one_shot:
  - key: leftshift
    modifier: shift
    timeout_ms: 1500

  - key: space
    layer: nav
    timeout_ms: 2000
```

#### 3. Dual-Role Tap-vs-Hold Integration (`osm` and `osl`)

One-shot actions can also be combined with dual-role `tap_hold:` keys using `osm(<modifier>)` and `osl(<layer>)`:

```yaml
tap_hold:
  # Tap CapsLock = Sticky Shift (next key capitalized); Hold CapsLock = Super/Win key
  capslock: [osm(shift), super, 200]

  # Tap Space = One-Shot Nav layer; Hold Space = Momentary Alt
  space: [osl(nav), alt, 250]

  # Or verbose property syntax:
  tab:
    tap: osm(ctrl)
    hold: alt
    timeout_ms: 200
```

### Validation Rules
- One-shot keys cannot also be used as combo chords or standard `tap_hold` triggers to prevent ambiguous overlapping state machines.
- `timeout_ms` must be a positive integer.
- One-shot layers referenced in `one_shot` or `osl(...)` must be defined under `layers:`.
- Modifiers in `one_shot` or `osm(...)` must be valid modifier keys (`shift`, `ctrl`, `alt`, `super`, etc.).

## Tap Dance (Multi-Tap and Tap-Hold Variations on a Single Key)

Tap Dance allows a single physical key to perform multiple distinct actions depending on whether it is tapped once, double-tapped, held down, double-tapped and held, or triple-tapped. This maximizes ergonomics and reduces finger travel on compact keyboards.

### Supported Actions on a Single Key

Each Tap Dance key can define any subset of the following actions:
- **`tap`**: Triggered on a single tap (key pressed and released once).
- **`double_tap`**: Triggered on a double tap (two rapid taps within the timeout window).
- **`hold`**: Triggered when the key is held down on the first tap longer than the timeout, or when another key is pressed while held (permissive hold).
- **`double_hold`**: Triggered when the key is tapped once and then held down on the second press.
- **`triple_tap`**: Triggered on three rapid taps.
- **`timeout_ms`** (or `timeout`): Multi-tap detection window in milliseconds (default: `200ms`). Supports suffixes like `180ms`, `0.2s`, or bare numbers `200`.

### Fast Commit Optimization

If a Tap Dance definition only specifies a single tap (and optionally a hold, but no `double_tap`, `double_hold`, or `triple_tap`), releasing the key commits the single tap **immediately** on key release without waiting for the timeout to elapse. This ensures zero latency during normal typing!

### Supported Target Actions

Tap Dance actions support full action polymorphism:
- **Key chords**: `esc`, `ctrl+c`, `super+shift+q`
- **Modal Layers**: `layer(nav)`, `layer: nav` (momentary activation while held)
- **Toggle Layers**: `toggle_layer(numpad)`, `tg(numpad)`, `{ toggle_layer: numpad }`
- **Text Snippets**: `text "console.log();"`, `text: ":wq\n"`
- **Mouse Keys**: `mouse_btn_left`, `mouse_wheel_up`

### Syntax Formats

#### 1. Verbose Property Block (Recommended)

```yaml
tap_dance:
  # CapsLock: Tap = Escape, Double-tap = CapsLock toggle, Hold = Ctrl, Double-tap-hold = Right Ctrl, Triple-tap = macro
  capslock:
    tap: esc
    double_tap: capslock
    hold: lctrl
    double_hold: rctrl
    triple_tap: text "DONE"
    timeout_ms: 220

  # Semicolon: Tap = Semicolon, Hold = Momentary Navigation Layer
  semicolon:
    tap: semicolon
    hold: layer(nav)
```

#### 2. Compact Inline Dictionary

```yaml
tap_dance:
  tab: { tap: tab, double_tap: tg(nav) }
  grave: { tap: grave, double_tap: esc, hold: lalt }
```

#### 3. Compact List Syntax

```yaml
tap_dance:
  # [tap, double_tap, timeout] or [tap, double_tap, hold, timeout]
  tab: [tab, esc, lctrl, 180ms]
  grave: [grave, esc, 150]
```

#### 4. Classic List Format

```yaml
tap_dance:
  - key: capslock
    tap: esc
    hold: lctrl
    timeout_ms: 200
```

### Validation Rules
- Tap dance keys cannot overlap with combo keys, dual-role `tap_hold` keys, one-shot keys, or dedicated leader keys.
- At least one action (`tap`, `double_tap`, `hold`, `double_hold`, or `triple_tap`) must be defined.
- Referenced layers must be defined under `layers:`.
- `timeout_ms` must be positive.

## Sequential Leader Key Sequences (Mnemonic Shortcuts)

Sequential Leader key sequences allow you to trigger complex commands, hotkeys, or multi-character text snippets by pressing a leader key, followed by a sequence of mnemonic keys typed one after another (Vim and Emacs style).

Unlike chords that require pressing keys at the exact same moment, leader sequences are typed **sequentially**:
- Tap Leader key (e.g. `capslock` or a dual-role key like `capslock: [leader, super, 200]`).
- Type `w` then `q` -> emits `:wq\n`.
- Type `g` then `s` -> emits `git status\n`.
- Type `b` -> emits `Ctrl + B`.

### Safe Typing Guarantee (Mismatch & Inactivity Replay)
- **Automatic Replay on Mismatch**: If you accidentally press the leader key or mistype a sequence (e.g. typing `w` then `x` instead of `q`), TFF immediately cancels leader mode and **replays all buffered keys** to the system. You will never lose typed text or suffer stuck keys.
- **Inactivity Timeout**: If you tap the leader key and pause without completing a sequence, the leader state expires automatically after the configured timeout (default: 1000 ms), and any buffered keystrokes are replayed cleanly.
- **Cancel by Tapping Leader Again**: Pressing the leader key a second time cancels leader mode immediately.

### Configuration Syntax

#### 1. Dedicated Leader Key (Compact Dictionary)

```yaml
leader:
  key: capslock
  timeout_ms: 1000
  sequences:
    "w q": ":wq\n"
    "g s": "git status\n"
    "g c m": "git commit -m \"\"\n"
    "b": ctrl+b
```

#### 2. Dual-Role Tap-vs-Hold Leader (`tap: leader` or `[leader, super, 200]`)

You can also configure the leader key as the short tap action of a dual-role key:

```yaml
tap_hold:
  capslock: [leader, super, 200]

leader:
  timeout_ms: 1000
  sequences:
    "w q": ":wq\n"
    "g s": "git status\n"
```

#### 3. List Format

```yaml
leader:
  key: capslock
  timeout_ms: 1000
  sequences:
    - keys: [w, q]
      text: ":wq\n"
    - keys: "b"
      out: ctrl+b
```

### Validation Rules
- Leader sequence keys must be valid key names.
- Duplicate leader sequences are rejected.
- Sequences cannot be prefixes of another sequence (e.g. configuring both `"w"` and `"w q"` is rejected to eliminate ambiguity).
- `timeout_ms` must be a positive integer.
- A dedicated leader key cannot conflict with combo keys.

## Auto-Shift (Long-Press Key Capitalization)

Auto-Shift allows typing capitalized letters or shifted punctuation by holding a key slightly longer than a normal tap, eliminating the need to reach for the Shift key.

- **Tap (< `timeout_ms`)**: Emits the unshifted character/symbol (e.g., `a`, `,`).
- **Hold (>= `timeout_ms`)**: Emits the shifted character/symbol (e.g., `A`, `<`).
- **Fast Typing Rollover**: If you press another key before the timeout expires (e.g. typing "the" quickly), the preceding key is immediately committed as an unshifted tap without waiting. Typing flow has zero lag.
- **Modifier Hotkey Coexistence**: If physical or one-shot modifiers (Ctrl, Alt, Super) are active, Auto-Shift is bypassed so desktop shortcuts (e.g. `Ctrl+C`, `Alt+Tab`) trigger instantly.

### Configuration Syntax

```yaml
auto_shift:
  enabled: true
  timeout_ms: 175
  keys: letters
```

#### Supported Presets:
- `letters` (or `alpha`): All 26 alphabet keys (`a`-`z`). This is the default if `keys` is omitted.
- `numbers` (or `digits`): Digit keys `0` through `9`.
- `symbols` (or `punctuation`): Common punctuation keys (`;`, `,`, `.`, `/`, `\`, `-`, `=`, `[`, `]`, `'`, `` ` ``).
- `all`: Combines `letters`, `numbers`, and `symbols`.
- **Explicit list or array**: e.g., `keys: [a, b, c, comma, period]` or YAML list format.

## Supported Key Names and Symbols

Key names must be lowercase (e.g., `ctrl+s`, `esc`, `delete`) and are mapped to standard Linux input event codes:

- **Letters**: `a` through `z`
- **Digits**: `0` through `9`
- **Literal Punctuation**: `;`, `,`, `.`, `/`, `\`, `-`, `=`, `[`, `]`, `'`, `` ` `` (as well as verbose names `semicolon`, `comma`, etc.)
- **Navigation & Editing**: `backspace`, `delete` (or `del`), `home`, `end`, `up` (or `arrowup`), `down` (or `arrowdown`), `left` (or `arrowleft`), `right` (or `arrowright`), `pageup` (or `pgup`), `pagedown` (or `pgdn`), `insert` (or `ins`)
- **Control & Modifiers**: `esc` (or `escape`), `enter` (or `return`), `tab`, `space`, `capslock` (or `caps`), `leftctrl` (or `ctrl`), `leftshift` (or `shift`), `leftalt` (or `alt`), `leftmeta` (or `super`, `win`, `windows`, `meta`)
- **Function Keys**: `f1` through `f12`

## CLI Cheat Sheet & Visualizer

You can visualize all active combos, tap-hold dual-role bindings, and modal layers directly in your terminal without opening the YAML file:

```bash
# Print formatted terminal cheat sheet (with ANSI colors):
tff cheatsheet

# Specify a custom config file:
tff cheatsheet /etc/tff/tff-combos.yaml

# Generate GitHub-flavored Markdown tables (great for documentation or note taking):
tff cheatsheet --markdown

# Plain text output without ANSI colors (safe for pipes and logs):
tff cheatsheet --plain
```

When piped or when the `NO_COLOR` environment variable is present, color codes are disabled automatically.

## Interactive Live Event Monitor (`tff monitor`)

When learning new combos or troubleshooting typing timing, `tff monitor` provides real-time visibility into the input stream, keycode mapping, timing deltas, chord candidate matching, active modal layers, and emitted virtual keys.

```bash
# Monitor all connected keyboards with default configuration:
tff monitor

# Monitor a specific device node (e.g. while debugging matrix rollover):
tff monitor /dev/input/event8

# Specify a custom combo configuration file:
tff monitor my-combos.yaml

# Disable ANSI color escapes (useful when redirecting to a log file):
tff monitor --plain

# Disable virtual emitted key lines or timing deltas:
tff monitor --no-emitted
tff monitor --no-deltas
```

### Example Live Terminal Output

```text
[14:23:01.120]  DOWN  'd' (code: 32)
[14:23:01.145]  DOWN  'f' (code: 33)   (+25ms)  -> CHORD CANDIDATE: d + f
[14:23:01.170]  DOWN  'j' (code: 36)   (+25ms)  -> CHORD CANDIDATE: d + f + j
[14:23:01.210]  UP    'd' (code: 32)   (+40ms)  (swallowed)
                                                -> TRIGGER COMBO: d + f + j -> esc
                                                -> EMIT: esc (code: 1, DOWN)
                                                -> EMIT: esc (code: 1, UP)
[14:23:02.050]  DOWN  'capslock' (code: 58)     -> TAP-HOLD: pending hold
[14:23:02.250]  TIMER EXPIRED                   -> TAP-HOLD: hold 'layer(nav)'
```

By default, `tff monitor` opens input devices in non-exclusive snooping mode (`grab = false`) with virtual keyboard emission disabled to avoid interfering with your active desktop session or terminal. Press `Ctrl+C` to cleanly exit at any time.

## Home-Row Mouse Keys via uinput

TFF supports full mouse emulation directly from your keyboard without reaching for a physical mouse or trackpad. Mouse events are emitted natively through the Linux `uinput` virtual device (`EV_REL` relative motion axes `REL_X`, `REL_Y`, `REL_WHEEL`, `REL_HWHEEL`, and `EV_KEY` mouse button clicks `BTN_LEFT`, `BTN_RIGHT`, `BTN_MIDDLE`, `BTN_SIDE`, `BTN_EXTRA`).

### Mouse Actions Reference

| Action Name | Type | Description |
|:---|:---|:---|
| `mouse_left`, `cursor_left`, `ms_left` | Relative Move | Moves cursor left by `move_speed` pixels (X axis) |
| `mouse_right`, `cursor_right`, `ms_right` | Relative Move | Moves cursor right by `move_speed` pixels (X axis) |
| `mouse_up`, `cursor_up`, `ms_up` | Relative Move | Moves cursor up by `move_speed` pixels (Y axis) |
| `mouse_down`, `cursor_down`, `ms_down` | Relative Move | Moves cursor down by `move_speed` pixels (Y axis) |
| `mouse_wheel_up`, `wheel_up`, `scroll_up` | Relative Scroll | Scrolls wheel up by `wheel_step` units |
| `mouse_wheel_down`, `wheel_down`, `scroll_down` | Relative Scroll | Scrolls wheel down by `wheel_step` units |
| `mouse_wheel_left`, `wheel_left`, `scroll_left` | Relative Scroll | Horizontal wheel scroll left by `wheel_step` units |
| `mouse_wheel_right`, `wheel_right`, `scroll_right` | Relative Scroll | Horizontal wheel scroll right by `wheel_step` units |
| `mouse_btn_left`, `btn_left`, `mouse_left_click` | Mouse Button | Left mouse click (`BTN_LEFT`, keycode 272) |
| `mouse_btn_right`, `btn_right`, `mouse_right_click` | Mouse Button | Right mouse click (`BTN_RIGHT`, keycode 273) |
| `mouse_btn_middle`, `btn_middle`, `mouse_middle_click` | Mouse Button | Middle mouse click (`BTN_MIDDLE`, keycode 274) |
| `mouse_btn_side`, `btn_side` | Mouse Button | Side / Back mouse button (`BTN_SIDE`, keycode 275) |
| `mouse_btn_extra`, `btn_extra` | Mouse Button | Extra / Forward mouse button (`BTN_EXTRA`, keycode 276) |

#### Custom Movement Distances & Scroll Speeds

You can override the default speed for specific keys or combos by specifying an inline parameter in parentheses:
```yaml
layers:
  mouse:
    h: mouse_left(25)     # Moves cursor left by 25 pixels
    j: mouse_down(25)     # Moves cursor down by 25 pixels
    u: wheel_up(3)        # Fast scroll up (3 units per tick)
```

### Global Mouse Settings (`mouse:`)

Configure base movement speed and scroll wheel step under the `mouse:` root block:

```yaml
mouse:
  speed: 12        # Cursor move speed in pixels (default: 10, range: 1..1000)
  wheel_step: 2    # Scroll wheel units per event (default: 1, range: 1..100)
```

### Home-Row Mouse Layer Example

A complete ergonomic setup combining a dual-role `space` key with a dedicated `mouse` modal layer:

```yaml
mouse:
  speed: 12
  wheel_step: 1

tap_hold:
  space:
    tap: space
    layer: mouse
    timeout_ms: 180

layers:
  mouse:
    # Vi-style home row cursor movements
    h: mouse_left
    j: mouse_down
    k: mouse_up
    l: mouse_right

    # Faster navigation on top row
    y: mouse_left(30)
    u: mouse_down(30)
    i: mouse_up(30)
    o: mouse_right(30)

    # Scrolling
    e: wheel_up
    d: wheel_down

    # Buttons & Dragging
    f: mouse_btn_left    # Press and hold to drag!
    s: mouse_btn_right
    a: mouse_btn_middle
```

#### Key Repeat & Click-and-Drag Support
- **Continuous Motion**: Holding a movement key (`h`, `j`, `k`, `l`) triggers kernel key-repeat, smoothly gliding the cursor across the screen.
- **Click-and-Drag**: Because mouse buttons (`BTN_LEFT`, `BTN_RIGHT`, `BTN_MIDDLE`) are standard Linux input events, holding `f` keeps the mouse button physically held down until `f` is released. You can select text or drag windows seamlessly!

### Combos with Mouse Actions

Combos can trigger mouse clicks or sudden cursor shifts:

```yaml
combos:
  d + f: mouse_btn_left     # Chord D and F together to left-click
  s + d: mouse_btn_right    # Chord S and D together to right-click
```

## Global Settings (`settings:`)

TFF supports an optional top-level `settings:` section to customize engine timing and platform behavior directly from YAML:

```yaml
settings:
  combo_timeout_ms: 40       # Overlap detection window for chords (default: 40ms)
  tap_hold_timeout_ms: 200   # Default timeout for dual-role keys (default: 200ms)
  exclusive_grab: true       # Exclusively grab physical keyboards (default: true)
  hotplug: true              # Monitor /dev/input for hotplugged keyboards (default: true)
```

### Options

| Option | Type | Default | Description |
|:---|:---|:---|:---|
| `combo_timeout_ms` | integer (1–5000) | `40` | Overlap window in milliseconds required to trigger simultaneous chords. Fast typists can decrease this (e.g. `30`–`35ms`) to avoid accidental chords during rapid rolls; others can increase it (e.g. `60`–`80ms`). |
| `tap_hold_timeout_ms` | integer (1–10000) | `200` | Default timeout in milliseconds for dual-role keys. Any `tap_hold:` key without an explicit timeout inherits this value. |
| `exclusive_grab` | boolean | `true` | When true, grabs keyboard devices exclusively via `ioctl(fd, EVIOCGRAB, 1)` so original keys are intercepted. |
| `hotplug` | boolean | `true` | When true, uses Linux `inotify` to automatically detect and attach newly connected USB keyboards. |

## Configuration Wizard and Presets (`tff init`)

You can generate a custom configuration file through an interactive interview or install curated templates using `tff init`:

```bash
# Interactive configuration interview:
tff init

# List available presets:
tff init --list-presets

# Install a preset to ~/.config/tff/tff-combos.yaml:
tff init --preset vim-nav

# Preview preset without saving to disk:
tff init --preset minimal --print
```

### Available Presets
- `minimal`: Essential home-row index finger chords (`j+f` -> backspace, `f+j` -> delete, `d+f+j` -> escape) and dual-role CapsLock (`esc` on tap, `super` on hold).
- `vim-nav`: Vim-style `h/j/k/l` arrow navigation, `0/4` -> `home/end`, `u/d` -> `page_up/page_down` on Space-hold.
- `home-row-mods`: Dual-role modifiers on home row (`a/s/d/f` = Super, Alt, Ctrl, Shift on hold).
- `full`: Comprehensive setup with combos, Space-Nav layer, dual-role CapsLock, and Auto-Shift.

## CLI Validation

You can validate any configuration file before running the daemon:

```bash
tff validate /etc/tff/tff-combos.yaml
```

If any key name or syntax is invalid, the validator prints the error details and exits with code 1.

## Dynamic Reloading

The Linux daemon (`tff_linux` / `tff`) monitors the configuration file using `inotify`. When you edit and save the configuration file:
1. The daemon detects the change.
2. It validates the new YAML syntax.
3. If valid, the combos are reloaded in memory without restarting the daemon or losing active device grabs.
4. If invalid, the error is logged to stderr (and journald) and existing mappings remain active safely.

## Programmatic C++ Usage

### Loading via `tff::loadYamlConfig`

```cpp
#include "tff_parser.h"
#include <iostream>

std::string yaml_content = "...";
tff::Config config;
std::string err_msg;

if (tff::loadYamlConfig(yaml_content, config, err_msg)) {
    std::cout << "Loaded " << config.combos.size() << " combos, "
              << config.tap_hold_keys.size() << " tap-hold keys, and "
              << config.layers.size() << " layers.\n";
} else {
    std::cerr << "YAML error: " << err_msg << "\n";
}
```

### Loading via `tff::loadYamlCombos`

```cpp
#include "tff_parser.h"
#include <vector>
#include <string>
#include <iostream>

std::string yaml_content = "...";
std::vector<tff::Combo> combos;
std::string err_msg;

if (tff::loadYamlCombos(yaml_content, combos, err_msg)) {
    std::cout << "Successfully parsed " << combos.size() << " combos.\n";
} else {
    std::cerr << "YAML error: " << err_msg << "\n";
}
```