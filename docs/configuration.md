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

### Key Formats

The parser supports multiple convenient ways to express keys:

#### Space-separated string:
```yaml
- keys: j f
  outKeys: backspace
```

#### YAML sequence / list:
```yaml
- keys:
    - j
    - f
  outKeys:
    - backspace
```

#### Single key or multi-key output sequences:
```yaml
- keys: f space
  outKeys: ctrl s
```

## Supported Key Names

Key names are case-insensitive and mapped to standard Linux input event codes (`KEY_*`):

- **Letters**: `a` through `z`
- **Digits**: `0` through `9`
- **Navigation & Editing**: `backspace`, `delete`, `home`, `end`, `up`, `down`, `left`, `right`, `pageup`, `pagedown`, `insert`
- **Control & Modifiers**: `esc`, `enter`, `tab`, `space`, `capslock`, `leftctrl`, `rightctrl`, `leftshift`, `rightshift`, `leftalt`, `rightalt`, `leftmeta`, `rightmeta`
- **Punctuation**: `semicolon`, `colon`, `comma`, `dot`, `slash`, `backslash`, `minus`, `equal`, `leftbrace`, `rightbrace`, `apostrophe`, `grave`
- **Function Keys**: `f1` through `f12`

## CLI Validation

You can validate any configuration file before running the daemon:

```bash
tff validate /etc/tff/tff-combos.yaml
```

If any key name or syntax is invalid, the validator prints the exact line and error details and exits with code 1.

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
if (mapper.loadTffConfiguration("config/tff-combos.yaml")) {
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