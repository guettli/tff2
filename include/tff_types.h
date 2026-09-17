#ifndef TFF_TYPES_H
#define TFF_TYPES_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <limits>
#include <unordered_map>

namespace tff {

struct TimeVal {
    int64_t sec = 0;
    int64_t usec = 0;

    int64_t toMicros() const {
        return sec * 1000000LL + usec;
    }

    static TimeVal fromMicros(int64_t us) {
        return TimeVal{us / 1000000LL, us % 1000000LL};
    }

    static TimeVal maxTime() {
        return TimeVal{std::numeric_limits<int64_t>::max() / 2, 999999};
    }

    bool operator<(const TimeVal& o) const {
        if (sec != o.sec) return sec < o.sec;
        return usec < o.usec;
    }

    bool operator>(const TimeVal& o) const {
        return o < *this;
    }

    bool operator==(const TimeVal& o) const {
        return sec == o.sec && usec == o.usec;
    }

    bool operator!=(const TimeVal& o) const {
        return !(*this == o);
    }

    bool operator<=(const TimeVal& o) const {
        return !(o < *this);
    }

    bool operator>=(const TimeVal& o) const {
        return !(*this < o);
    }
};

inline int64_t timeSubMicros(const TimeVal& first, const TimeVal& second) {
    return second.toMicros() - first.toMicros();
}

using KeyCode = uint16_t;

enum KeyValue : int32_t {
    KEY_VAL_UP = 0,
    KEY_VAL_DOWN = 1,
    KEY_VAL_REPEAT = 2
};

enum EventType : uint16_t {
    EV_SYN = 0x00,
    EV_KEY = 0x01,
    EV_REL = 0x02,
    EV_MSC = 0x04
};

enum SynCode : uint16_t {
    SYN_REPORT = 0
};

enum RelCode : uint16_t {
    REL_X      = 0x00,
    REL_Y      = 0x01,
    REL_HWHEEL = 0x06,
    REL_WHEEL  = 0x08
};

enum MscCode : uint16_t {
    MSC_SCAN = 4
};

enum class MouseActionType : uint8_t {
    None = 0,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    WheelUp,
    WheelDown,
    WheelLeft,
    WheelRight,
    BtnLeft,
    BtnRight,
    BtnMiddle,
    BtnSide,
    BtnExtra
};

struct MouseAction {
    MouseActionType type = MouseActionType::None;
    int16_t delta = 0; // 0 = use default move speed or wheel step

    bool isMovement() const {
        return type == MouseActionType::MoveLeft || type == MouseActionType::MoveRight ||
               type == MouseActionType::MoveUp || type == MouseActionType::MoveDown;
    }

    bool isWheel() const {
        return type == MouseActionType::WheelUp || type == MouseActionType::WheelDown ||
               type == MouseActionType::WheelLeft || type == MouseActionType::WheelRight;
    }

    bool isButton() const {
        return type == MouseActionType::BtnLeft || type == MouseActionType::BtnRight ||
               type == MouseActionType::BtnMiddle || type == MouseActionType::BtnSide ||
               type == MouseActionType::BtnExtra;
    }

    bool isRelative() const {
        return isMovement() || isWheel();
    }

    bool operator==(const MouseAction& o) const {
        return type == o.type && delta == o.delta;
    }
};

struct MouseConfig {
    int16_t move_speed = 10;
    int16_t wheel_step = 1;

    bool operator==(const MouseConfig& o) const {
        return move_speed == o.move_speed && wheel_step == o.wheel_step;
    }
};

struct Event {
    TimeVal time;
    uint16_t type = EV_KEY;
    uint16_t code = 0;
    int32_t value = KEY_VAL_UP;

    bool operator==(const Event& o) const {
        return time == o.time && type == o.type && code == o.code && value == o.value;
    }
};

struct Combo {
    std::vector<KeyCode> keys;
    std::vector<KeyCode> out_keys;
    std::string text; // Multi-character text snippet / macro expansion
    std::string toggle_layer; // Layer to toggle on/off when combo triggered
    MouseAction mouse;

    Combo() = default;
    Combo(std::vector<KeyCode> k, std::vector<KeyCode> ok, std::string t = "", std::string tl = "", MouseAction m = {})
        : keys(std::move(k)), out_keys(std::move(ok)), text(std::move(t)), toggle_layer(std::move(tl)), mouse(m) {}

    bool operator==(const Combo& o) const {
        return keys == o.keys && out_keys == o.out_keys && text == o.text &&
               toggle_layer == o.toggle_layer && mouse == o.mouse;
    }
};

struct LayerAction {
    std::vector<KeyCode> out_keys;
    std::string text;
    std::string toggle_layer;
    MouseAction mouse;

    LayerAction() = default;
    explicit LayerAction(std::vector<KeyCode> ok, std::string t = "", std::string tl = "", MouseAction m = {})
        : out_keys(std::move(ok)), text(std::move(t)), toggle_layer(std::move(tl)), mouse(m) {}

    bool operator==(const LayerAction& o) const {
        return out_keys == o.out_keys && text == o.text && toggle_layer == o.toggle_layer && mouse == o.mouse;
    }
};

struct Layer {
    std::string name;
    std::unordered_map<KeyCode, LayerAction> mappings;

    bool operator==(const Layer& o) const {
        return name == o.name && mappings == o.mappings;
    }
};

struct TapHoldKey {
    KeyCode key = 0;               // Key to intercept (e.g. KEY_CAPSLOCK)
    KeyCode tap_key = 0;           // Emitted on short tap (e.g. KEY_ESC)
    KeyCode hold_key = 0;          // Emitted on long press / hold (e.g. KEY_LEFTMETA / Super / Win)
    std::string hold_layer;        // Layer activated while held (e.g. "nav")
    int64_t timeout_us = 200000LL; // Overlap timeout in microseconds (default 200ms)
    KeyCode tap_one_shot_modifier = 0; // If set, short tap arms one-shot modifier (OSM)
    std::string tap_one_shot_layer;     // If set, short tap arms one-shot layer (OSL)
    int64_t tap_one_shot_timeout_us = 1500000LL; // One-shot expiration timeout in us (default 1500ms)
    bool tap_leader = false;                     // If true, short tap activates sequential Leader mode
    std::string tap_toggle_layer;                // If set, short tap toggles layer on/off

    bool operator==(const TapHoldKey& o) const {
        return key == o.key && tap_key == o.tap_key && hold_key == o.hold_key &&
               hold_layer == o.hold_layer && timeout_us == o.timeout_us &&
               tap_one_shot_modifier == o.tap_one_shot_modifier &&
               tap_one_shot_layer == o.tap_one_shot_layer &&
               tap_one_shot_timeout_us == o.tap_one_shot_timeout_us &&
               tap_leader == o.tap_leader &&
               tap_toggle_layer == o.tap_toggle_layer;
    }
};

struct OneShotKey {
    KeyCode key = 0;               // Key to intercept (e.g. KEY_LEFTSHIFT)
    KeyCode modifier = 0;          // Modifier applied to next key (defaults to key if key is modifier)
    std::string layer;             // One-Shot Layer applied to next key (OSL)
    int64_t timeout_us = 1500000LL; // Expiration timeout in microseconds (default 1500ms)

    bool operator==(const OneShotKey& o) const {
        return key == o.key && modifier == o.modifier &&
               layer == o.layer && timeout_us == o.timeout_us;
    }
};

struct LeaderSequence {
    std::vector<KeyCode> keys;      // Sequential keys (e.g. [KEY_W, KEY_Q])
    std::vector<KeyCode> out_keys;  // Output key combination
    std::string text;               // Output text snippet (e.g. ":wq\n")
    std::string toggle_layer;       // Layer to toggle on/off when sequence triggered
    MouseAction mouse;

    LeaderSequence() = default;
    LeaderSequence(std::vector<KeyCode> k, std::vector<KeyCode> ok, std::string t = "", std::string tl = "", MouseAction m = {})
        : keys(std::move(k)), out_keys(std::move(ok)), text(std::move(t)), toggle_layer(std::move(tl)), mouse(m) {}

    bool operator==(const LeaderSequence& o) const {
        return keys == o.keys && out_keys == o.out_keys && text == o.text &&
               toggle_layer == o.toggle_layer && mouse == o.mouse;
    }
};

struct LeaderConfig {
    KeyCode key = 0;                        // Leader trigger key (e.g. KEY_CAPSLOCK)
    int64_t timeout_us = 1000000LL;         // Inactivity timeout in us (default 1000ms)
    std::vector<LeaderSequence> sequences;  // Defined sequential shortcuts

    bool operator==(const LeaderConfig& o) const {
        return key == o.key && timeout_us == o.timeout_us && sequences == o.sequences;
    }
};

struct AutoShiftConfig {
    bool enabled = false;
    int64_t timeout_us = 175000LL; // 175 ms default
    std::vector<KeyCode> keys;

    bool operator==(const AutoShiftConfig& o) const {
        return enabled == o.enabled && timeout_us == o.timeout_us && keys == o.keys;
    }
};

struct Settings {
    int64_t combo_timeout_ms = 40;       // Overlap detection window in ms (default 40ms)
    int64_t tap_hold_timeout_ms = 200;   // Default timeout for dual-role keys in ms (default 200ms)
    bool exclusive_grab = true;          // Exclusive device grab (default true)
    bool hotplug = true;                 // Inotify device hotplugging (default true)

    bool operator==(const Settings& o) const {
        return combo_timeout_ms == o.combo_timeout_ms &&
               tap_hold_timeout_ms == o.tap_hold_timeout_ms &&
               exclusive_grab == o.exclusive_grab &&
               hotplug == o.hotplug;
    }

    bool operator!=(const Settings& o) const {
        return !(*this == o);
    }
};

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

class EventWriter {
public:
    virtual ~EventWriter() = default;
    virtual void writeOne(const Event& ev) = 0;
};

class EventReader {
public:
    virtual ~EventReader() = default;
    virtual bool readOne(Event& ev) = 0;
};

} // namespace tff

#endif // TFF_TYPES_H
