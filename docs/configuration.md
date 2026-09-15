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
  g + h: esc       # Symmetric: either g then h, or h then g
  f space: ctrl+s  # Hotkey action
```

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