# Configuration System Documentation

## Overview

The configuration system allows users to customize key mappings and behavior of the TFF-like keyboard remapping system. It uses YAML files for human-readable configuration and supports multiple layers for different contexts.

## Configuration File Structure

### Basic Structure

```yaml
settings:
  overlap_threshold_ms: 100

mappings:
  - name: "example_mapping"
    combo: ["f", "j"]
    output: "1"

layers:
  - name: "main"
    active: true
    mappings:
      - combo: ["f", "space"]
        output: ["ctrl", "s"]
```

### Settings Section

The `settings` section controls global behavior:

```yaml
settings:
  overlap_threshold_ms: 100  # Time window for overlapping key detection (milliseconds)
  enable_logging: false      # Enable debug logging (future feature)
```

### Mappings Section

The `mappings` section defines global key combinations:

```yaml
mappings:
  # Basic key combination
  - name: "move_right"
    combo: ["f", "j"]
    output: "right"

  # Multiple key output
  - name: "save_document"
    combo: ["f", "space"]
    output: ["ctrl", "s"]

  # Complex combination
  - name: "complex_macro"
    combo: ["f", "j", "k"]
    output: ["ctrl", "shift", "s"]
```

### Layers Section

Layers allow context-specific mappings:

```yaml
layers:
  - name: "main"
    active: true
    mappings:
      - combo: ["f", "j"]
        output: "1"

  - name: "symbols"
    active: false
    mappings:
      - combo: ["f", "j"]
        output: "!"
```

## Key Names and Codes

### Supported Keys

The system supports standard keyboard keys:

- Letters: "a", "b", "c", ..., "z"
- Numbers: "1", "2", "3", ..., "0"
- Special keys: "space", "enter", "tab", "esc"
- Modifier keys: "ctrl", "shift", "alt", "gui"
- Navigation keys: "up", "down", "left", "right"
- Function keys: "f1", "f2", ..., "f12"

### Key Combination Syntax

Key combinations are defined as arrays of key names:

```yaml
# Two-key combination
combo: ["f", "j"]

# Three-key combination
combo: ["ctrl", "shift", "s"]

# Single key (for remapping individual keys)
combo: ["capslock"]
```

## Example Configurations

### Basic Configuration

```yaml
settings:
  overlap_threshold_ms: 100

mappings:
  - name: "navigation_fj"
    combo: ["f", "j"]
    output: "right"

  - name: "navigation_jf"
    combo: ["j", "f"]
    output: "left"

  - name: "save_document"
    combo: ["f", "space"]
    output: ["ctrl", "s"]
```

### Advanced Configuration with Layers

```yaml
settings:
  overlap_threshold_ms: 100

mappings:
  - name: "global_escape"
    combo: ["j", "j"]
    output: "esc"

layers:
  - name: "main"
    active: true
    mappings:
      - combo: ["f", "j"]
        output: "1"
      - combo: ["j", "f"]
        output: "2"
      - combo: ["f", "space"]
        output: ["ctrl", "s"]

  - name: "gaming"
    active: false
    mappings:
      - combo: ["f", "j"]
        output: "space"
      - combo: ["j", "f"]
        output: "tab"
```

## Loading Configuration

### Programmatic Loading

```cpp
#include "config_manager.h"
#include "key_mapper.h"

ConfigManager config_manager;
KeyMapper key_mapper;

// Load configuration from file
bool success = config_manager.loadFromFile("config/mappings.yaml", key_mapper);

if (success) {
    std::cout << "Configuration loaded successfully" << std::endl;
} else {
    std::cerr << "Failed to load configuration" << std::endl;
}
```

### Runtime Reloading

Future implementations will support runtime configuration reloading:

```cpp
// Watch for configuration file changes
config_manager.watchFile("config/mappings.yaml");

// Reload when file changes
config_manager.reloadOnChange(key_mapper);
```

## Validation and Error Handling

### Configuration Validation

The system validates configurations:

- Checks for duplicate key combinations
- Verifies key names are supported
- Ensures output keys are valid
- Validates layer names and references

### Error Reporting

Errors are reported with descriptive messages:

```
Error: Duplicate key combination found: ["f", "j"]
Error: Unknown key name: "xyz"
Error: Invalid layer name: "invalid_layer"
```

## Best Practices

### Naming Conventions

- Use descriptive names for mappings
- Follow consistent naming patterns
- Include context in layer names

### Performance Considerations

- Limit the number of layers for better performance
- Avoid overly complex key combinations
- Use single-key mappings sparingly

### Organization Tips

- Group related mappings together
- Use comments to explain complex mappings
- Separate global and layer-specific mappings clearly

## Future Extensions

### Planned Features

1. **JSON Configuration Support**
   - Alternative to YAML format
   - Same structure, different syntax

2. **Remote Configuration**
   - Load configurations from URLs
   - Centralized configuration management

3. **Configuration GUI**
   - Visual editor for key mappings
   - Real-time preview of changes

4. **Profile Management**
   - Save and load configuration profiles
   - Import/export configurations