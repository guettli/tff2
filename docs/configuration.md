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

### Validation Rules
- Any layer referenced by `layer:` in `tap_hold` must be defined in `layers:`.
- Duplicate layer names and duplicate key mappings within a layer are rejected.
- Keys inside layer mappings cannot have empty outputs.

## Supported Key Names and Symbols

Key names must be lowercase (e.g., `ctrl+s`, `esc`, `delete`) and are mapped to standard Linux input event codes:

- **Letters**: `a` through `z`
- **Digits**: `0` through `9`
- **Literal Punctuation**: `;`, `,`, `.`, `/`, `\`, `-`, `=`, `[`, `]`, `'`, `` ` `` (as well as verbose names `semicolon`, `comma`, etc.)
- **Navigation & Editing**: `backspace`, `delete` (or `del`), `home`, `end`, `up` (or `arrowup`), `down` (or `arrowdown`), `left` (or `arrowleft`), `right` (or `arrowright`), `pageup` (or `pgup`), `pagedown` (or `pgdn`), `insert` (or `ins`)
- **Control & Modifiers**: `esc` (or `escape`), `enter` (or `return`), `tab`, `space`, `capslock` (or `caps`), `leftctrl` (or `ctrl`), `leftshift` (or `shift`), `leftalt` (or `alt`), `leftmeta` (or `super`, `win`, `windows`, `meta`)
- **Function Keys**: `f1` through `f12`

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

### Loading via `KeyMapper`

```cpp
#include "key_mapper.h"
#include <iostream>

KeyMapper mapper;
if (mapper.loadConfiguration("config/tff-combos.yaml")) {
    std::cout << "Loaded " << mapper.getMappingCount() << " combos.\n";
} else {
    std::cerr << "Failed to load TFF configuration.\n";
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