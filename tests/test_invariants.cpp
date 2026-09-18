#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <random>
#include <algorithm>

using namespace tff;

namespace {

// TrackingEventWriter verifies the Zero-Stuck-Key invariant in real-time.
// It tracks every key code that is pressed down and ensures:
// 1. Every DOWN key is eventually released with an UP event.
// 2. No UP event is emitted for a key that was not down (spurious UP).
// 3. No double-DOWN events are emitted for a key already down without an intervening UP.
// 4. Upon test completion, active_keys_ must be strictly empty (zero stuck keys).
class TrackingEventWriter : public EventWriter {
public:
    std::vector<Event> all_events;
    std::vector<Event> key_events;
    std::set<KeyCode> active_keys;
    int spurious_ups = 0;
    int duplicate_downs = 0;

    void writeOne(const Event& ev) override {
        all_events.push_back(ev);
        if (ev.type == EV_KEY) {
            key_events.push_back(ev);
            if (ev.value == KEY_VAL_DOWN) {
                if (active_keys.find(ev.code) != active_keys.end()) {
                    duplicate_downs++;
                }
                active_keys.insert(ev.code);
            } else if (ev.value == KEY_VAL_UP) {
                if (active_keys.find(ev.code) == active_keys.end()) {
                    spurious_ups++;
                }
                active_keys.erase(ev.code);
            }
        }
    }

    bool hasStuckKeys() const { return !active_keys.empty(); }

    const std::set<KeyCode>& getStuckKeys() const { return active_keys; }

    void assertZeroStuckKeys(const std::string& context = "") const {
        if (!active_keys.empty()) {
            std::cerr << "FAILED: Stuck keys detected in " << context << ": ";
            for (KeyCode k : active_keys) {
                std::cerr << keyCodeToWord(k) << "(" << k << ") ";
            }
            std::cerr << std::endl;
            assert(active_keys.empty());
        }
        if (spurious_ups > 0) {
            std::cerr << "FAILED: Spurious UP events detected in " << context << ": "
                      << spurious_ups << std::endl;
            assert(spurious_ups == 0);
        }
        if (duplicate_downs > 0) {
            std::cerr << "FAILED: Duplicate DOWN events detected in " << context << ": "
                      << duplicate_downs << std::endl;
            assert(duplicate_downs == 0);
        }
    }

    void clear() {
        all_events.clear();
        key_events.clear();
        active_keys.clear();
        spurious_ups = 0;
        duplicate_downs = 0;
    }
};

}  // anonymous namespace

// 1. Single-key invariant: every press has a matching release, zero stuck keys
static void test_invariant_single_keys() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    std::vector<KeyCode> test_keys = {Keys::KEY_A,     Keys::KEY_B,         Keys::KEY_SPACE,
                                      Keys::KEY_ENTER, Keys::KEY_LEFTSHIFT, Keys::KEY_LEFTCTRL,
                                      Keys::KEY_ESC};

    int64_t t_us = 1000;
    for (KeyCode k : test_keys) {
        engine.processEvent(Event{TimeVal::fromMicros(t_us), EV_KEY, k, KEY_VAL_DOWN});
        t_us += 50000;
        engine.processEvent(Event{TimeVal::fromMicros(t_us), EV_KEY, k, KEY_VAL_UP});
        t_us += 50000;
    }

    writer.assertZeroStuckKeys("test_invariant_single_keys");
    assert(writer.key_events.size() == test_keys.size() * 2);
    std::cout << "  [PASS] Single-key invariant" << std::endl;
}

// 2. Chords & combos across all permutations of press and release order
static void test_invariant_combos_all_permutations() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    // Symmetric 3-key combo: d + f + j -> Tab
    Config cfg;
    std::string err;
    bool ok = loadYamlConfig(
        "combos:\n"
        "  d + f + j: tab\n"
        "  f + j: esc\n",
        cfg, err);
    assert(ok);
    engine.setConfig(cfg);

    std::vector<KeyCode> keys = {Keys::KEY_D, Keys::KEY_F, Keys::KEY_J};
    std::vector<size_t> p = {0, 1, 2};

    // Test all 6 arrival permutations with all 6 release permutations
    do {
        std::vector<size_t> r_p = {0, 1, 2};
        do {
            writer.clear();
            engine.reset();
            int64_t t = 1000;

            // Press all 3 keys within combo window
            for (size_t idx : p) {
                engine.processEvent(Event{TimeVal::fromMicros(t), EV_KEY, keys[idx], KEY_VAL_DOWN});
                t += 10000;
            }

            // Release all 3 keys
            for (size_t idx : r_p) {
                t += 50000;
                engine.processEvent(Event{TimeVal::fromMicros(t), EV_KEY, keys[idx], KEY_VAL_UP});
            }

            writer.assertZeroStuckKeys("test_invariant_combos_all_permutations");
        } while (std::next_permutation(r_p.begin(), r_p.end()));
    } while (std::next_permutation(p.begin(), p.end()));

    std::cout << "  [PASS] Multi-key combo permutations invariant" << std::endl;
}

// 3. Tap-Hold keys across all paths (tap, hold, fast chording, out-of-order release)
static void test_invariant_tap_hold_all_paths() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000;  // 200ms
    engine.setTapHoldKeys({thk});

    // Path 1: Tap (release before timeout)
    {
        writer.clear();
        engine.reset();
        engine.processEvent(
            Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
        engine.processEvent(
            Event{TimeVal::fromMicros(50000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
        writer.assertZeroStuckKeys("tap-hold tap");
    }

    // Path 2: Hold (timeout expires, then release)
    {
        writer.clear();
        engine.reset();
        engine.processEvent(
            Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
        engine.onTimer(TimeVal::fromMicros(250000));
        assert(writer.hasStuckKeys());  // Meta should be down
        engine.processEvent(
            Event{TimeVal::fromMicros(300000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
        writer.assertZeroStuckKeys("tap-hold hold release");
    }

    // Path 3: Fast chording (CapsLock down, A down, A up, CapsLock up)
    {
        writer.clear();
        engine.reset();
        engine.processEvent(
            Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
        engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
        engine.processEvent(Event{TimeVal::fromMicros(40000), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
        engine.processEvent(
            Event{TimeVal::fromMicros(60000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
        writer.assertZeroStuckKeys("tap-hold fast chording");
    }

    // Path 4: Key release out of order (CapsLock down, A down, CapsLock up, A up)
    {
        writer.clear();
        engine.reset();
        engine.processEvent(
            Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
        engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
        engine.processEvent(
            Event{TimeVal::fromMicros(40000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
        engine.processEvent(Event{TimeVal::fromMicros(60000), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
        writer.assertZeroStuckKeys("tap-hold out-of-order release");
    }

    std::cout << "  [PASS] Tap-hold invariant" << std::endl;
}

// 4. Modal layers race conditions: layer deactivated before remapped key is released
static void test_invariant_modal_layers_race_conditions() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    bool ok = loadYamlConfig(
        "tap_hold:\n"
        "  space:\n"
        "    tap: space\n"
        "    layer: nav\n"
        "    timeout: 200ms\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n"
        "    j: down\n"
        "    k: up\n"
        "    l: right\n",
        cfg, err);
    assert(ok);
    engine.setConfig(cfg);

    // Race condition: Press Space (activates nav layer on chord), Press 'j' (Down arrow down),
    // Release Space (deactivates nav layer) BEFORE releasing 'j'!
    // When 'j' is finally released, the engine must release Down arrow, NOT leave it stuck down!
    engine.processEvent(Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    assert(writer.hasStuckKeys());  // Down arrow should be down
    assert(writer.active_keys.count(Keys::KEY_DOWN) == 1);

    // Release Space (layer deactivates)
    engine.processEvent(Event{TimeVal::fromMicros(50000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});

    // Release J after layer has already deactivated
    engine.processEvent(Event{TimeVal::fromMicros(80000), EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    writer.assertZeroStuckKeys("test_invariant_modal_layers_race_conditions");

    std::cout << "  [PASS] Modal layer race conditions invariant" << std::endl;
}

// 5. One-Shot Modifiers and Layers: armed, consumed, timed out, reset
static void test_invariant_one_shot() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    OneShotKey osk;
    osk.key = Keys::KEY_LEFTSHIFT;
    osk.modifier = Keys::KEY_LEFTSHIFT;
    osk.timeout_us = 200000;
    engine.setOneShotKeys({osk});

    // Case 1: Arm, then press key -> modifier released with key
    engine.processEvent(
        Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(30000), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    writer.assertZeroStuckKeys("oneshot armed");

    engine.processEvent(Event{TimeVal::fromMicros(60000), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(90000), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    writer.assertZeroStuckKeys("oneshot consumed");

    // Case 2: Arm, then timeout expires without keypress
    writer.clear();
    engine.processEvent(
        Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    engine.processEvent(
        Event{TimeVal::fromMicros(120000), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    engine.onTimer(TimeVal::fromMicros(400000));
    writer.assertZeroStuckKeys("oneshot timeout");

    std::cout << "  [PASS] One-shot modifiers invariant" << std::endl;
}

// 6. Auto-Shift invariant: tap unshifted, hold shifted, interrupted, reset
static void test_invariant_auto_shift() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000;
    asc.keys = {Keys::KEY_A, Keys::KEY_B};
    engine.setAutoShiftConfig(asc);

    // Hold 'a' until auto-shift triggers uppercase
    engine.processEvent(Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(200000));
    assert(writer.active_keys.count(Keys::KEY_LEFTSHIFT) == 1);
    assert(writer.active_keys.count(Keys::KEY_A) == 1);

    // Release 'a' -> both Shift and 'a' must be released
    engine.processEvent(Event{TimeVal::fromMicros(250000), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    writer.assertZeroStuckKeys("auto-shift hold release");

    // Interrupted by another key before timeout
    writer.clear();
    engine.processEvent(Event{TimeVal::fromMicros(300000), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(350000), EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(370000), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(390000), EV_KEY, Keys::KEY_B, KEY_VAL_UP});
    writer.assertZeroStuckKeys("auto-shift interrupted");

    std::cout << "  [PASS] Auto-shift invariant" << std::endl;
}

// 7. Leader Key invariant: trigger, cancel, replay, reset
static void test_invariant_leader_key() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lc;
    lc.key = Keys::KEY_LEFTCTRL;
    LeaderSequence seq;
    seq.keys = {Keys::KEY_W, Keys::KEY_Q};
    seq.text = ":wq\n";
    lc.sequences.push_back(seq);
    engine.setLeaderConfig(lc);

    // Match macro
    engine.processEvent(Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(40000), EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(60000), EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(80000), EV_KEY, Keys::KEY_Q, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_Q, KEY_VAL_UP});
    writer.assertZeroStuckKeys("leader match");

    // Mismatch -> replay raw events and cancel
    writer.clear();
    engine.processEvent(
        Event{TimeVal::fromMicros(200000), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(220000), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(240000), EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});
    engine.processEvent(
        Event{TimeVal::fromMicros(260000), EV_KEY, Keys::KEY_X, KEY_VAL_DOWN});  // mismatch
    engine.processEvent(Event{TimeVal::fromMicros(280000), EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(300000), EV_KEY, Keys::KEY_X, KEY_VAL_UP});
    writer.assertZeroStuckKeys("leader mismatch replay");

    std::cout << "  [PASS] Leader key invariant" << std::endl;
}

// 8. Reset and Finish invariants: ensure all active down keys are released upon reset/finish
static void test_invariant_reset_and_finish() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    bool ok = loadYamlConfig(
        "combos:\n"
        "  f + j: ctrl+c\n"
        "tap_hold:\n"
        "  capslock: [esc, super, 200ms]\n"
        "  space: [space, nav, 200ms]\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n",
        cfg, err);
    assert(ok);
    engine.setConfig(cfg);

    // Scenario A: Combo written down -> call reset()
    engine.processEvent(Event{TimeVal::fromMicros(1000), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(10000), EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(200000));
    // Combo ctrl+c is down
    assert(writer.hasStuckKeys());
    engine.reset();
    writer.assertZeroStuckKeys("reset() with active combo");

    // Scenario B: Layer held -> call finish()
    writer.clear();
    engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(30000), EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    assert(writer.hasStuckKeys());
    engine.finish();
    writer.assertZeroStuckKeys("finish() with active layer keys");

    // Scenario C: Tap-hold key held -> call finish()
    writer.clear();
    engine.processEvent(
        Event{TimeVal::fromMicros(40000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(300000));
    assert(writer.hasStuckKeys());  // Super is held
    engine.finish();
    writer.assertZeroStuckKeys("finish() with active tap-hold");

    std::cout << "  [PASS] Reset and finish clean release invariant" << std::endl;
}

// 9. Bounded buffer invariant: ensure buf_ never grows beyond MAX_BUFFER_SIZE
static void test_invariant_bounded_buffer() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    // Verify calling evictOldestBufferedEvent on an empty engine is safe
    engine.evictOldestBufferedEvent();
    assert(engine.getBufferSize() == 0);

    // Define overlapping candidate combos so that as each oldest key is evicted,
    // the remaining 64 buffered keys continue to match a valid candidate prefix
    // (ComboNotFinished), directly exercising evictOldestBufferedEvent().
    const size_t TOTAL_KEYS = 80;
    const size_t OVERFLOW_COUNT = TOTAL_KEYS - TFFEngine::MAX_BUFFER_SIZE;
    std::vector<Combo> combos;
    for (size_t start = 1; start <= OVERFLOW_COUNT + 1; ++start) {
        Combo c;
        for (size_t k = start; k < start + TFFEngine::MAX_BUFFER_SIZE + 5; ++k) {
            c.keys.push_back(static_cast<KeyCode>(k));
        }
        c.out_keys = {Keys::KEY_Z};
        combos.push_back(c);
    }
    engine.setCombos(combos);

    int64_t t = 1000;
    // Feed the first MAX_BUFFER_SIZE (64) keys -> all accumulate in buf_
    for (size_t i = 0; i < TFFEngine::MAX_BUFFER_SIZE; ++i) {
        KeyCode code = static_cast<KeyCode>(i + 1);
        engine.processEvent(Event{TimeVal::fromMicros(t), EV_KEY, code, KEY_VAL_DOWN});
        assert(engine.getBufferSize() == i + 1);
        t += 1000;
    }
    assert(engine.getBufferSize() == TFFEngine::MAX_BUFFER_SIZE);
    assert(writer.key_events.empty());  // No evictions yet

    // Feed remaining keys (65 through 80) -> each triggers evictOldestBufferedEvent()
    for (size_t i = TFFEngine::MAX_BUFFER_SIZE; i < TOTAL_KEYS; ++i) {
        KeyCode code = static_cast<KeyCode>(i + 1);
        engine.processEvent(Event{TimeVal::fromMicros(t), EV_KEY, code, KEY_VAL_DOWN});
        assert(engine.getBufferSize() == TFFEngine::MAX_BUFFER_SIZE);
        t += 1000;
    }

    // Exactly OVERFLOW_COUNT (16) oldest keys must have been evicted in strict FIFO order
    assert(writer.key_events.size() == OVERFLOW_COUNT);
    for (size_t i = 0; i < OVERFLOW_COUNT; ++i) {
        assert(writer.key_events[i].code == static_cast<KeyCode>(i + 1));
        assert(writer.key_events[i].value == KEY_VAL_DOWN);
    }

    // Directly test manual evictOldestBufferedEvent invocation
    engine.evictOldestBufferedEvent();
    assert(engine.getBufferSize() == TFFEngine::MAX_BUFFER_SIZE - 1);
    assert(writer.key_events.size() == OVERFLOW_COUNT + 1);
    assert(writer.key_events.back().code == static_cast<KeyCode>(OVERFLOW_COUNT + 1));

    // Release all 80 keys while stream is active
    for (size_t i = 1; i <= TOTAL_KEYS; ++i) {
        engine.processEvent(
            Event{TimeVal::fromMicros(t), EV_KEY, static_cast<KeyCode>(i), KEY_VAL_UP});
        t += 1000;
    }
    engine.finish();
    writer.assertZeroStuckKeys("bounded buffer eviction");

    std::cout << "  [PASS] Bounded buffer invariant (<= " << TFFEngine::MAX_BUFFER_SIZE
              << " events, FIFO eviction verified)" << std::endl;
}

// 10. Randomized property-based fuzz test across all features (5,000 cycles)
static void test_invariant_randomized_fuzzing() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    bool ok = loadYamlConfig(
        "combos:\n"
        "  d + f: backspace\n"
        "  j + k: enter\n"
        "  f + j: esc\n"
        "  c + v: tab\n"
        "tap_hold:\n"
        "  capslock: [esc, super, 150ms]\n"
        "  space: [space, nav, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n"
        "    j: down\n"
        "    k: up\n"
        "    l: right\n",
        cfg, err);
    assert(ok);
    engine.setConfig(cfg);

    std::mt19937 rng(1337);
    std::vector<KeyCode> pool = {Keys::KEY_D, Keys::KEY_F, Keys::KEY_J,        Keys::KEY_K,
                                 Keys::KEY_C, Keys::KEY_V, Keys::KEY_CAPSLOCK, Keys::KEY_SPACE,
                                 Keys::KEY_H, Keys::KEY_L, Keys::KEY_A,        Keys::KEY_B};

    std::set<KeyCode> physical_down;
    int64_t curr_time_us = 1000000;

    for (int cycle = 0; cycle < 5000; ++cycle) {
        int action_type = rng() % 10;
        curr_time_us += (rng() % 50000) + 1000;  // 1ms to 51ms forward

        if (action_type < 5) {
            // Press a random key that is not currently down
            std::vector<KeyCode> available;
            for (KeyCode k : pool) {
                if (physical_down.find(k) == physical_down.end()) {
                    available.push_back(k);
                }
            }
            if (!available.empty()) {
                KeyCode chosen = available[rng() % available.size()];
                physical_down.insert(chosen);
                engine.processEvent(
                    Event{TimeVal::fromMicros(curr_time_us), EV_KEY, chosen, KEY_VAL_DOWN});
            }
        } else if (action_type < 8) {
            // Release a random key that is currently down
            if (!physical_down.empty()) {
                auto it = physical_down.begin();
                std::advance(it, rng() % physical_down.size());
                KeyCode chosen = *it;
                physical_down.erase(it);
                engine.processEvent(
                    Event{TimeVal::fromMicros(curr_time_us), EV_KEY, chosen, KEY_VAL_UP});
            }
        } else {
            // Timer tick
            engine.onTimer(TimeVal::fromMicros(curr_time_us));
        }

        // Buffer size must strictly respect bound at every single step
        assert(engine.getBufferSize() <= TFFEngine::MAX_BUFFER_SIZE);
    }

    // End of session: release all physically held keys in random order
    std::vector<KeyCode> remaining(physical_down.begin(), physical_down.end());
    std::shuffle(remaining.begin(), remaining.end(), rng);
    for (KeyCode k : remaining) {
        curr_time_us += 10000;
        engine.processEvent(Event{TimeVal::fromMicros(curr_time_us), EV_KEY, k, KEY_VAL_UP});
    }

    // Flush any pending timers
    curr_time_us += 500000;
    engine.onTimer(TimeVal::fromMicros(curr_time_us));
    engine.finish();

    // The core invariant: after all physical keys are released and stream finished, ZERO STUCK
    // KEYS!
    writer.assertZeroStuckKeys("randomized fuzz test");
    std::cout << "  [PASS] Randomized property-based fuzzing (5,000 cycles, 0 stuck keys)"
              << std::endl;
}

// 11. Extreme timestamps & corrupted event resilience
static void test_invariant_corrupted_and_extreme_inputs() {
    TrackingEventWriter writer;
    TFFEngine engine(&writer);

    // Negative timestamps (time going backward)
    engine.processEvent(Event{TimeVal::fromMicros(-50000), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(0), EV_KEY, Keys::KEY_A, KEY_VAL_UP});

    // Extreme int64 timestamps
    engine.processEvent(
        Event{TimeVal::fromMicros(0x7fffffffffffLL), EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});
    engine.processEvent(
        Event{TimeVal::fromMicros(0x7fffffffffffLL + 1000), EV_KEY, Keys::KEY_B, KEY_VAL_UP});

    // Out-of-range key codes (> 1000)
    engine.processEvent(Event{TimeVal::fromMicros(1000), EV_KEY, 9999, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(2000), EV_KEY, 9999, KEY_VAL_UP});

    // Back-to-back duplicate DOWN events (switch bounce)
    engine.processEvent(Event{TimeVal::fromMicros(3000), EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(3010), EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(4000), EV_KEY, Keys::KEY_C, KEY_VAL_UP});

    // Back-to-back duplicate UP events (key pressed down, then released with contact bounce)
    engine.processEvent(Event{TimeVal::fromMicros(5000), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(5010), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(5020), EV_KEY, Keys::KEY_D, KEY_VAL_UP});

    engine.finish();
    writer.assertZeroStuckKeys("extreme inputs");

    std::cout << "  [PASS] Extreme and corrupted input resilience" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  TFF2 - Invariant & Resilience Test Suite        " << std::endl;
    std::cout << "==================================================" << std::endl;

    test_invariant_single_keys();
    test_invariant_combos_all_permutations();
    test_invariant_tap_hold_all_paths();
    test_invariant_modal_layers_race_conditions();
    test_invariant_one_shot();
    test_invariant_auto_shift();
    test_invariant_leader_key();
    test_invariant_reset_and_finish();
    test_invariant_bounded_buffer();
    test_invariant_randomized_fuzzing();
    test_invariant_corrupted_and_extreme_inputs();

    std::cout << "==================================================" << std::endl;
    std::cout << "  All invariant & resilience tests passed (100%)  " << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
