#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

using namespace tff;

class MockWriter : public EventWriter {
public:
    std::vector<Event> events;

    void writeOne(const Event& ev) override { events.push_back(ev); }

    void clear() { events.clear(); }

    std::vector<Event> keyEvents() const {
        std::vector<Event> result;
        for (const auto& ev : events) {
            if (ev.type == EV_KEY) {
                result.push_back(ev);
            }
        }
        return result;
    }
};

static void test_tap_capslock_emits_esc() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;  // 200ms
    engine.setTapHoldKeys({thk});

    // Press CapsLock at t = 10ms
    Event ev_down;
    ev_down.time = TimeVal{0, 10000};
    ev_down.type = EV_KEY;
    ev_down.code = Keys::KEY_CAPSLOCK;
    ev_down.value = KEY_VAL_DOWN;
    bool ok = engine.processEvent(ev_down);
    assert(ok);

    // No events should be emitted yet while waiting for tap/hold resolution
    assert(writer.keyEvents().empty());

    // Release CapsLock at t = 60ms (50ms duration, well under 200ms)
    Event ev_up;
    ev_up.time = TimeVal{0, 60000};
    ev_up.type = EV_KEY;
    ev_up.code = Keys::KEY_CAPSLOCK;
    ev_up.value = KEY_VAL_UP;
    ok = engine.processEvent(ev_up);
    assert(ok);

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 2);
    assert(key_evs[0].code == Keys::KEY_ESC && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_ESC && key_evs[1].value == KEY_VAL_UP);
    std::cout << "test_tap_capslock_emits_esc: PASSED\n";
}

static void test_hold_capslock_emits_super() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;  // 200ms
    engine.setTapHoldKeys({thk});

    // Press CapsLock at t = 10ms
    Event ev_down;
    ev_down.time = TimeVal{0, 10000};
    ev_down.type = EV_KEY;
    ev_down.code = Keys::KEY_CAPSLOCK;
    ev_down.value = KEY_VAL_DOWN;
    bool ok = engine.processEvent(ev_down);
    assert(ok);

    // Trigger timer at t = 220ms (timeout is 10ms + 200ms = 210ms)
    engine.onTimer(TimeVal{0, 220000});

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 1);
    assert(key_evs[0].code == Keys::KEY_LEFTMETA && key_evs[0].value == KEY_VAL_DOWN);

    // Release CapsLock at t = 500ms
    Event ev_up;
    ev_up.time = TimeVal{0, 500000};
    ev_up.type = EV_KEY;
    ev_up.code = Keys::KEY_CAPSLOCK;
    ev_up.value = KEY_VAL_UP;
    ok = engine.processEvent(ev_up);
    assert(ok);

    key_evs = writer.keyEvents();
    assert(key_evs.size() == 2);
    assert(key_evs[1].code == Keys::KEY_LEFTMETA && key_evs[1].value == KEY_VAL_UP);
    std::cout << "test_hold_capslock_emits_super: PASSED\n";
}

static void test_chord_capslock_with_key() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    // CapsLock DOWN at t = 10ms
    Event cl_down;
    cl_down.time = TimeVal{0, 10000};
    cl_down.type = EV_KEY;
    cl_down.code = Keys::KEY_CAPSLOCK;
    cl_down.value = KEY_VAL_DOWN;
    bool ok = engine.processEvent(cl_down);
    assert(ok);

    // KEY_C DOWN at t = 50ms (before 200ms timeout)
    // Intervening key press immediately promotes CapsLock to Super DOWN!
    Event c_down;
    c_down.time = TimeVal{0, 50000};
    c_down.type = EV_KEY;
    c_down.code = Keys::KEY_C;
    c_down.value = KEY_VAL_DOWN;
    ok = engine.processEvent(c_down);
    assert(ok);

    // KEY_C UP at t = 100ms
    Event c_up;
    c_up.time = TimeVal{0, 100000};
    c_up.type = EV_KEY;
    c_up.code = Keys::KEY_C;
    c_up.value = KEY_VAL_UP;
    ok = engine.processEvent(c_up);
    assert(ok);

    // CapsLock UP at t = 150ms
    Event cl_up;
    cl_up.time = TimeVal{0, 150000};
    cl_up.type = EV_KEY;
    cl_up.code = Keys::KEY_CAPSLOCK;
    cl_up.value = KEY_VAL_UP;
    ok = engine.processEvent(cl_up);
    assert(ok);

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 4);
    assert(key_evs[0].code == Keys::KEY_LEFTMETA && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_C && key_evs[1].value == KEY_VAL_DOWN);
    assert(key_evs[2].code == Keys::KEY_C && key_evs[2].value == KEY_VAL_UP);
    assert(key_evs[3].code == Keys::KEY_LEFTMETA && key_evs[3].value == KEY_VAL_UP);
    std::cout << "test_chord_capslock_with_key: PASSED\n";
}

static void test_double_tap() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    // Tap 1: 10ms -> 40ms
    Event ev1;
    ev1.time = TimeVal{0, 10000};
    ev1.type = EV_KEY;
    ev1.code = Keys::KEY_CAPSLOCK;
    ev1.value = KEY_VAL_DOWN;
    engine.processEvent(ev1);

    ev1.time = TimeVal{0, 40000};
    ev1.value = KEY_VAL_UP;
    engine.processEvent(ev1);

    // Tap 2: 70ms -> 100ms
    Event ev2;
    ev2.time = TimeVal{0, 70000};
    ev2.type = EV_KEY;
    ev2.code = Keys::KEY_CAPSLOCK;
    ev2.value = KEY_VAL_DOWN;
    engine.processEvent(ev2);

    ev2.time = TimeVal{0, 100000};
    ev2.value = KEY_VAL_UP;
    engine.processEvent(ev2);

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 4);
    assert(key_evs[0].code == Keys::KEY_ESC && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_ESC && key_evs[1].value == KEY_VAL_UP);
    assert(key_evs[2].code == Keys::KEY_ESC && key_evs[2].value == KEY_VAL_DOWN);
    assert(key_evs[3].code == Keys::KEY_ESC && key_evs[3].value == KEY_VAL_UP);
    std::cout << "test_double_tap: PASSED\n";
}

static void test_multiple_keys_while_held() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    // CapsLock DOWN at 10ms
    Event cl_down{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN};
    engine.processEvent(cl_down);

    // A DOWN at 30ms -> promotes CapsLock to Super DOWN
    Event a_down{TimeVal{0, 30000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN};
    engine.processEvent(a_down);

    // A UP at 60ms
    Event a_up{TimeVal{0, 60000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP};
    engine.processEvent(a_up);

    // B DOWN at 80ms
    Event b_down{TimeVal{0, 80000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN};
    engine.processEvent(b_down);

    // B UP at 110ms
    Event b_up{TimeVal{0, 110000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP};
    engine.processEvent(b_up);

    // CapsLock UP at 150ms -> Super UP
    Event cl_up{TimeVal{0, 150000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP};
    engine.processEvent(cl_up);

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 6);
    assert(key_evs[0].code == Keys::KEY_LEFTMETA && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_A && key_evs[1].value == KEY_VAL_DOWN);
    assert(key_evs[2].code == Keys::KEY_A && key_evs[2].value == KEY_VAL_UP);
    assert(key_evs[3].code == Keys::KEY_B && key_evs[3].value == KEY_VAL_DOWN);
    assert(key_evs[4].code == Keys::KEY_B && key_evs[4].value == KEY_VAL_UP);
    assert(key_evs[5].code == Keys::KEY_LEFTMETA && key_evs[5].value == KEY_VAL_UP);
    std::cout << "test_multiple_keys_while_held: PASSED\n";
}

static void test_reset_releases_held_key() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Event ev_down{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN};
    engine.processEvent(ev_down);

    // Expire timer -> Super DOWN
    engine.onTimer(TimeVal{0, 220000});

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 1);
    assert(key_evs[0].code == Keys::KEY_LEFTMETA && key_evs[0].value == KEY_VAL_DOWN);

    // Reset should emit Super UP to avoid stuck key
    engine.reset();
    key_evs = writer.keyEvents();
    assert(key_evs.size() == 2);
    assert(key_evs[1].code == Keys::KEY_LEFTMETA && key_evs[1].value == KEY_VAL_UP);
    std::cout << "test_reset_releases_held_key: PASSED\n";
}

static void test_parser_multiline_tap_hold() {
    std::string yaml = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super
    timeout_ms: 250
)";

    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    assert(config.tap_hold_keys.size() == 1);
    assert(config.tap_hold_keys[0].key == Keys::KEY_CAPSLOCK);
    assert(config.tap_hold_keys[0].tap_key == Keys::KEY_ESC);
    assert(config.tap_hold_keys[0].hold_key == Keys::KEY_LEFTMETA);
    assert(config.tap_hold_keys[0].timeout_us == 250000LL);
    std::cout << "test_parser_multiline_tap_hold: PASSED\n";
}

static void test_parser_inline_bracket() {
    std::string yaml = R"(
tap_hold:
  capslock: [esc, super, 150]
)";

    Config config;
    std::string err;
    assert(!loadYamlConfig(yaml, config, err));
    assert(err.find("compact inline format for tap_hold") != std::string::npos);
    std::cout << "test_parser_inline_bracket (rejection): PASSED\n";
}

static void test_parser_validation_errors() {
    Config config;
    std::string err;

    // Missing hold key in multi-line definition
    std::string yaml_missing_hold = R"(
tap_hold:
  capslock:
    tap: esc
)";
    assert(!loadYamlConfig(yaml_missing_hold, config, err));
    assert(!err.empty());

    // Invalid key name
    std::string yaml_bad_key = R"(
tap_hold:
  not_a_valid_key_12345:
    tap: esc
    hold: super
)";
    assert(!loadYamlConfig(yaml_bad_key, config, err));
    assert(!err.empty());

    // Invalid timeout number
    std::string yaml_bad_timeout = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super
    timeout_ms: not_a_number
)";
    assert(!loadYamlConfig(yaml_bad_timeout, config, err));
    assert(!err.empty());

    // Non-positive timeout (zero or negative)
    std::string yaml_zero_timeout = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super
    timeout_ms: 0
)";
    assert(!loadYamlConfig(yaml_zero_timeout, config, err));
    assert(!err.empty());

    std::string yaml_neg_timeout = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super
    timeout_ms: -50
)";
    assert(!loadYamlConfig(yaml_neg_timeout, config, err));
    assert(!err.empty());

    // Unknown property typo in multi-line block
    std::string yaml_typo = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super
    tiemout_ms: 200
)";
    assert(!loadYamlConfig(yaml_typo, config, err));
    assert(!err.empty());

    // Conflicting key in both tap_hold and combos
    std::string yaml_conflict = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super
combos:
  capslock h: left
)";
    assert(!loadYamlConfig(yaml_conflict, config, err));
    assert(err.find("cannot be used in both combos and tap_hold") != std::string::npos);

    std::cout << "test_parser_validation_errors: PASSED\n";
}

static void test_reset_timestamp_monotonic() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.tap_key = Keys::KEY_ESC;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Event ev_down{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN};
    engine.processEvent(ev_down);

    // Timer expires at 210000us
    engine.onTimer(TimeVal{0, 210000});
    assert(writer.keyEvents().size() == 1);
    TimeVal down_tv = writer.keyEvents()[0].time;

    // Call reset
    engine.reset();
    assert(writer.keyEvents().size() == 2);
    TimeVal up_tv = writer.keyEvents()[1].time;

    // Verify timestamp monotonicity: up_tv >= down_tv
    assert(up_tv >= down_tv);
    std::cout << "test_reset_timestamp_monotonic: PASSED\n";
}

static void test_parser_combined_config() {
    std::string yaml = R"(
tap_hold:
  capslock:
    tap: esc
    hold: super

combos:
  j f: backspace
  f j: delete
)";

    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    assert(config.tap_hold_keys.size() == 1);
    assert(config.combos.size() == 2);
    assert(config.tap_hold_keys[0].key == Keys::KEY_CAPSLOCK);
    assert(config.combos[0].out_keys[0] == Keys::KEY_BACKSPACE);
    assert(config.combos[1].out_keys[0] == Keys::KEY_DELETE);
    std::cout << "test_parser_combined_config: PASSED\n";
}

int main() {
    std::cout << "Running Tap-vs-Hold tests...\n";
    test_tap_capslock_emits_esc();
    test_hold_capslock_emits_super();
    test_chord_capslock_with_key();
    test_double_tap();
    test_multiple_keys_while_held();
    test_reset_releases_held_key();
    test_reset_timestamp_monotonic();
    test_parser_multiline_tap_hold();
    test_parser_inline_bracket();
    test_parser_validation_errors();
    test_parser_combined_config();
    std::cout << "All 11 Tap-vs-Hold tests passed successfully!\n";
    return 0;
}
