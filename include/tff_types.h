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
    EV_MSC = 0x04
};

enum SynCode : uint16_t {
    SYN_REPORT = 0
};

enum MscCode : uint16_t {
    MSC_SCAN = 4
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

    Combo() = default;
    Combo(std::vector<KeyCode> k, std::vector<KeyCode> ok, std::string t = "")
        : keys(std::move(k)), out_keys(std::move(ok)), text(std::move(t)) {}

    bool operator==(const Combo& o) const {
        return keys == o.keys && out_keys == o.out_keys && text == o.text;
    }
};

struct LayerAction {
    std::vector<KeyCode> out_keys;
    std::string text;

    LayerAction() = default;
    explicit LayerAction(std::vector<KeyCode> ok, std::string t = "")
        : out_keys(std::move(ok)), text(std::move(t)) {}

    bool operator==(const LayerAction& o) const {
        return out_keys == o.out_keys && text == o.text;
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

    bool operator==(const TapHoldKey& o) const {
        return key == o.key && tap_key == o.tap_key && hold_key == o.hold_key &&
               hold_layer == o.hold_layer && timeout_us == o.timeout_us &&
               tap_one_shot_modifier == o.tap_one_shot_modifier &&
               tap_one_shot_layer == o.tap_one_shot_layer &&
               tap_one_shot_timeout_us == o.tap_one_shot_timeout_us &&
               tap_leader == o.tap_leader;
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

    LeaderSequence() = default;
    LeaderSequence(std::vector<KeyCode> k, std::vector<KeyCode> ok, std::string t = "")
        : keys(std::move(k)), out_keys(std::move(ok)), text(std::move(t)) {}

    bool operator==(const LeaderSequence& o) const {
        return keys == o.keys && out_keys == o.out_keys && text == o.text;
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

struct Config {
    std::vector<Combo> combos;
    std::vector<TapHoldKey> tap_hold_keys;
    std::vector<Layer> layers;
    std::vector<OneShotKey> one_shot_keys;
    LeaderConfig leader;
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
