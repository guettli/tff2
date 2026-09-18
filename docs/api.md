# Ten Flying Fingers (TFF) — C++ Core API Reference

The `tff_core` library provides a hardware-independent, dependency-free C++17 API for advanced keyboard remapping, order-independent chording, dual-role tap-hold keys, modal layers, auto-shift, one-shot modifiers, sequential leader keys, and mouse key emulation.

---

## 1. Core Header Files

| Header | Description |
|---|---|
| Header | Description |
|---|---|
| [`include/tff_types.h`](../include/tff_types.h) | Primary data structures (`Config`, `Combo`, `TapHoldKey`, `Layer`, `Event`, `TimeVal`) |
| [`include/tff_engine.h`](../include/tff_engine.h) | Main remapper state machine (`TFFEngine`) and `EventWriter` interface |
| [`include/tff_parser.h`](../include/tff_parser.h) | Compact YAML parser and configuration validator |
| [`include/tff_key_codes.h`](../include/tff_key_codes.h) | Evdev keycode definitions, friendly aliases, and string conversion helpers |
| [`include/tff_cheatsheet.h`](../include/tff_cheatsheet.h) | Cheat sheet generator (ANSI colored terminal tables & Markdown) |
| [`include/tff_monitor.h`](../include/tff_monitor.h) | Interactive live event monitor and chord debugger |
| [`include/tff_udev.h`](../include/tff_udev.h) | Linux udev rule generator and permission checker |
| [`include/linux_platform.h`](../include/linux_platform.h) | Linux evdev, `/dev/uinput`, and inotify event loop driver |
| [`include/rp2040_platform.h`](../include/rp2040_platform.h) | Raspberry Pi RP2040 TinyUSB host/device hardware driver |

---

## 2. Core Data Types (`tff_types.h`)

### 2.1 `TimeVal`
Represents an event timestamp with microsecond resolution:
```cpp
struct TimeVal {
    int64_t sec = 0;   // Seconds
    int64_t usec = 0;  // Microseconds (0 .. 999999)

    static TimeVal fromMicros(int64_t us);
    int64_t toMicros() const;
    bool operator<(const TimeVal& o) const;
    bool operator<=(const TimeVal& o) const;
    bool operator==(const TimeVal& o) const;
};
```

### 2.2 `Event`
Represents an input event conforming to the Linux input subsystem (`struct input_event`):
```cpp
struct Event {
    TimeVal time;       // Timestamp
    uint16_t type = 0;  // EV_KEY, EV_SYN, EV_REL, EV_MSC
    uint16_t code = 0;  // KeyCode (e.g. KEY_A) or RelCode (e.g. REL_X)
    int32_t value = 0;  // KEY_VAL_UP (0), KEY_VAL_DOWN (1), KEY_VAL_REPEAT (2)
};
```

### 2.3 `Combo`
Defines an order-independent chord mapping:
```cpp
struct Combo {
    std::vector<KeyCode> keys;      // Input chord keys (e.g. [KEY_F, KEY_J])
    std::vector<KeyCode> out_keys;  // Output keystrokes (e.g. [KEY_ESC])
    std::string text;               // Output text snippet macro (e.g. "email@example.com")
    std::string toggle_layer;       // Toggle layer on/off when chord triggers
    MouseAction mouse;              // Emitted mouse action (click / movement / scroll)
};
```

### 2.4 `TapHoldKey`
Defines a dual-role key (different behavior on short tap vs hold):
```cpp
struct TapHoldKey {
    KeyCode key = 0;                             // Trigger key (e.g. KEY_CAPSLOCK)
    KeyCode tap_key = 0;                         // Emitted on quick tap (e.g. KEY_ESC)
    KeyCode hold_key = 0;                        // Emitted while held (e.g. KEY_LEFTMETA)
    std::string hold_layer;                      // Layer activated while held (momentary)
    int64_t timeout_us = 200000LL;               // Expiration threshold in microseconds (default 200ms)
    KeyCode tap_one_shot_modifier = 0;           // Arm one-shot modifier on tap
    std::string tap_one_shot_layer;              // Arm one-shot layer on tap
    int64_t tap_one_shot_timeout_us = 1500000LL; // One-shot timeout in us (default 1500ms)
    bool tap_leader = false;                     // Activate leader mode on tap
    std::string tap_toggle_layer;                // Toggle layer on/off on tap
};
```

### 2.5 `Layer` and `LayerAction`
Defines a modal keyboard remapping layer:
```cpp
struct LayerAction {
    std::vector<KeyCode> out_keys;  // Key stroke or modifier chord
    std::string text;               // Text macro expansion
    std::string toggle_layer;       // Nested layer toggle
    MouseAction mouse;              // Mouse action
};

struct Layer {
    std::string name;                                  // Layer identifier (e.g. "nav")
    std::unordered_map<KeyCode, LayerAction> mappings; // Key remappings
};
```

### 2.6 `OneShotKey`
Defines a sticky modifier or layer that remains active for exactly one subsequent keystroke or until timeout:
```cpp
struct OneShotKey {
    KeyCode key = 0;                  // Key to intercept (e.g. KEY_LEFTSHIFT)
    KeyCode modifier = 0;             // Modifier applied to next key
    std::string layer;                // One-Shot Layer applied to next key
    int64_t timeout_us = 1500000LL;   // Timeout in microseconds (default 1500ms)
};
```

### 2.7 `AutoShiftConfig`
Defines long-press capitalization without modifier chords:
```cpp
struct AutoShiftConfig {
    bool enabled = false;             // Enable auto-shift engine
    int64_t timeout_us = 175000LL;    // Hold threshold in microseconds (default 175ms)
    std::vector<KeyCode> keys;        // Keys subject to auto-shift
};
```

### 2.8 `LeaderConfig` and `LeaderSequence`
Defines non-simultaneous vim-style sequential shortcuts:
```cpp
struct LeaderSequence {
    std::vector<KeyCode> keys;      // Sequential key chain (e.g. [KEY_W, KEY_Q])
    std::vector<KeyCode> out_keys;  // Emitted keys
    std::string text;               // Emitted text snippet
    std::string toggle_layer;       // Toggle layer
    MouseAction mouse;              // Emitted mouse action
};

struct LeaderConfig {
    KeyCode key = 0;                        // Dedicated leader key (e.g. KEY_RIGHTALT)
    int64_t timeout_us = 1000000LL;         // Inactivity timeout in microseconds (1000ms)
    std::vector<LeaderSequence> sequences;  // Sequence definitions
};
```

### 2.9 `Config`
Master configuration struct encapsulating an entire keyboard layout:
```cpp
struct Config {
    std::vector<Combo> combos;
    std::vector<TapHoldKey> tap_hold_keys;
    std::vector<Layer> layers;
    std::vector<OneShotKey> one_shot_keys;
    LeaderConfig leader;
    AutoShiftConfig auto_shift;
    MouseConfig mouse;
    Settings settings;
};
```

---

## 3. Remapping Engine API (`tff::TFFEngine`)

### 3.1 Construction & Configuration

```cpp
// Construct engine with an output writer and optional initial combos
explicit TFFEngine(EventWriter* out_dev = nullptr, const std::vector<Combo>& combos = {});

// Apply a complete master configuration (combos, tap-hold, layers, one-shots, leader, auto-shift, mouse)
void setConfig(const Config& config);
Config getConfig() const;

// Individual configuration setters & getters
void setCombos(const std::vector<Combo>& combos);
const std::vector<Combo>& getCombos() const;

void setTapHoldKeys(const std::vector<TapHoldKey>& keys);
const std::vector<TapHoldKey>& getTapHoldKeys() const;

void setLayers(const std::vector<Layer>& layers);
const std::vector<Layer>& getLayers() const;

void setOneShotKeys(const std::vector<OneShotKey>& keys);
const std::vector<OneShotKey>& getOneShotKeys() const;

void setLeaderConfig(const LeaderConfig& config);
const LeaderConfig& getLeaderConfig() const;

void setAutoShiftConfig(const AutoShiftConfig& config);
const AutoShiftConfig& getAutoShiftConfig() const;

void setMouseConfig(const MouseConfig& config);
const MouseConfig& getMouseConfig() const;

void setSettings(const Settings& settings);
const Settings& getSettings() const;
```

### 3.2 Event Processing & Lifecycle

```cpp
// Process a single hardware event.
// Returns true on success, false if KEY_RFKILL or fatal error is encountered.
bool processEvent(const Event& ev);

// Trigger a timer expiration tick at the specified timestamp.
// Evaluates active tap-hold keys, auto-shift holds, leader timeouts, and one-shots.
void onTimer(TimeVal time);

// Query if an active timer is pending, and get its next expiration time.
bool hasActiveTimer() const;
TimeVal getActiveTimerTime() const;

// Cleanly finishes input stream (EOF).
// Flushes all buffers, disengages active one-shots, and emits UP events for all held keys.
void finish();

// Resets internal engine state immediately.
// Safely releases any active held combos or tap-hold keys to prevent stuck keys.
void reset();
```

### 3.3 Buffer Bounds & Memory Management

```cpp
// Hard upper bound on internal event buffer to prevent runaway memory usage
static constexpr size_t MAX_BUFFER_SIZE = 64;

// Inspect the current number of events pending in the buffer
size_t getBufferSize() const;

// Manually evict the oldest buffered event using FIFO ordering
void evictOldestBufferedEvent();
```

### 3.4 Modal Layers & Inspection

```cpp
void activateLayer(const std::string& name);
void deactivateLayer(const std::string& name);
void toggleLayer(const std::string& name);
bool isLayerActive(const std::string& name) const;
const std::vector<std::string>& getActiveLayers() const;

bool isOneShotModifierArmed(KeyCode mod) const;
bool isOneShotLayerArmed(const std::string& layer) const;
bool isLeaderActive() const;
```

### 3.5 Diagnostics & Trace Callbacks

```cpp
// Register a callback for real-time diagnostic event tracing (used by tff monitor)
void setTraceCallback(TraceCallback cb);
bool hasTraceCallback() const;
```

---

## 4. Output Interface (`tff::EventWriter`)

Any platform or test mock implementing event emission must inherit from `EventWriter`:

```cpp
class EventWriter {
public:
    virtual ~EventWriter() = default;
    virtual void writeOne(const Event& ev) = 0;
};
```

### Example Implementation (Mock Test Writer)
```cpp
class MockWriter : public tff::EventWriter {
public:
    std::vector<tff::Event> events;
    void writeOne(const tff::Event& ev) override {
        events.push_back(ev);
    }
};
```

---

## 5. Parser API (`tff_parser.h`)

```cpp
// Load complete configuration from YAML string (combos, tap-hold, layers, one-shots, leader, auto-shift, mouse)
bool loadYamlConfig(const std::string& yaml_str, Config& config, std::string& error_msg);

// Load combos only from YAML string
bool loadYamlCombos(const std::string& yaml_str, std::vector<Combo>& combos, std::string& err_msg);

// Parses duration string like "200ms", "1.5s", "1000us" into microseconds
bool parseDurationMicros(const std::string& str, int64_t& out_us);
```

### 5.1 Keycode Helper API (`tff_key_codes.h`)

```cpp
// Convert an ASCII character into an evdev KeyCode and Shift flag
bool asciiToKeyStroke(char c, KeyCode& code, bool& shift);

// Convert a KeyCode to its canonical name or word representation (e.g. "f", "capslock")
std::string keyCodeToWord(KeyCode code);

// Convert a canonical name or word representation to its evdev KeyCode
bool wordToKeyCode(const std::string& word, KeyCode& out_code, std::string& err_msg);
```
