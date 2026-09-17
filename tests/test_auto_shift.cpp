#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include "tff_cheatsheet.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

using namespace tff;

namespace {

class MockWriter : public EventWriter {
public:
    std::vector<Event> events;

    void writeOne(const Event& ev) override {
        events.push_back(ev);
    }

    void clear() {
        events.clear();
    }

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

} // anonymous namespace

static void test_auto_shift_tap_lowercase() {
    MockWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000LL; // 175 ms
    asc.keys = {Keys::KEY_A, Keys::KEY_B};
    engine.setAutoShiftConfig(asc);

    // 1. Tap 'a': down at 10ms, up at 80ms (< 175ms)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    // Key-down should be buffered, no events emitted yet
    assert(writer.keyEvents().empty());

    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    // On release before timeout: emits 'a' DOWN and 'a' UP
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_A && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_A && evs[1].value == KEY_VAL_UP);

    std::cout << "test_auto_shift_tap_lowercase: PASSED\n";
}

static void test_auto_shift_hold_uppercase() {
    MockWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000LL; // 175 ms
    asc.keys = {Keys::KEY_A};
    engine.setAutoShiftConfig(asc);

    // 1. Press 'a' down at 10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // 2. Timer fires at 185ms (10ms + 175ms = 185ms)
    engine.onTimer(TimeVal{0, 185000});

    // Shift + 'a' should now be emitted DOWN
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_LEFTSHIFT && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_A && evs[1].value == KEY_VAL_DOWN);

    // 3. Release 'a' at 250ms
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[2].code == Keys::KEY_A && writer.keyEvents()[2].value == KEY_VAL_UP);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[3].value == KEY_VAL_UP);

    std::cout << "test_auto_shift_hold_uppercase: PASSED\n";
}

static void test_auto_shift_fast_typing_rolls() {
    MockWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000LL;
    asc.keys = {Keys::KEY_T, Keys::KEY_H, Keys::KEY_E};
    engine.setAutoShiftConfig(asc);

    // Typing "the" with fast finger rollover:
    // 't' down at 0ms
    engine.processEvent(Event{TimeVal{0, 0}, EV_KEY, Keys::KEY_T, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // 'h' down at 40ms (before 't' timeout or release -> roll!)
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    // 't' must be committed as unshifted DOWN!
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_T && writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // 't' up at 60ms
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_T, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_T && writer.keyEvents()[1].value == KEY_VAL_UP);

    // 'e' down at 80ms (before 'h' timeout or release -> roll!)
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_E, KEY_VAL_DOWN});
    // 'h' committed as unshifted DOWN!
    assert(writer.keyEvents().size() == 3);
    assert(writer.keyEvents()[2].code == Keys::KEY_H && writer.keyEvents()[2].value == KEY_VAL_DOWN);

    // 'h' up at 100ms
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[3].code == Keys::KEY_H && writer.keyEvents()[3].value == KEY_VAL_UP);

    // 'e' up at 130ms (< 80ms + 175ms)
    engine.processEvent(Event{TimeVal{0, 130000}, EV_KEY, Keys::KEY_E, KEY_VAL_UP});
    // 'e' committed as tap DOWN and UP!
    assert(writer.keyEvents().size() == 6);
    assert(writer.keyEvents()[4].code == Keys::KEY_E && writer.keyEvents()[4].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[5].code == Keys::KEY_E && writer.keyEvents()[5].value == KEY_VAL_UP);

    // Verify no LeftShift was ever emitted
    for (const auto& ev : writer.keyEvents()) {
        assert(ev.code != Keys::KEY_LEFTSHIFT);
    }

    std::cout << "test_auto_shift_fast_typing_rolls: PASSED\n";
}

static void test_auto_shift_roll_then_hold() {
    MockWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000LL;
    asc.keys = {Keys::KEY_C, Keys::KEY_A};
    engine.setAutoShiftConfig(asc);

    // 1. 'c' down at 0ms
    engine.processEvent(Event{TimeVal{0, 0}, EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // 2. 'a' down at 40ms (commit 'c' as unshifted)
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_C && writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // 3. 'c' up at 60ms
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_C, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_C && writer.keyEvents()[1].value == KEY_VAL_UP);

    // 4. 'a' is held past timeout: 40ms + 175ms = 215ms
    engine.onTimer(TimeVal{0, 215000});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[2].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[2].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[3].code == Keys::KEY_A && writer.keyEvents()[3].value == KEY_VAL_DOWN);

    // 5. 'a' up at 300ms
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 6);
    assert(writer.keyEvents()[4].code == Keys::KEY_A && writer.keyEvents()[4].value == KEY_VAL_UP);
    assert(writer.keyEvents()[5].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[5].value == KEY_VAL_UP);

    std::cout << "test_auto_shift_roll_then_hold: PASSED\n";
}

static void test_auto_shift_punctuation_symbols() {
    MockWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000LL;
    asc.keys = {Keys::KEY_COMMA, Keys::KEY_DOT, Keys::KEY_SLASH};
    engine.setAutoShiftConfig(asc);

    // Tap comma (<175ms) -> emits comma
    engine.processEvent(Event{TimeVal{0, 0}, EV_KEY, Keys::KEY_COMMA, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_COMMA, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_COMMA && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_COMMA && writer.keyEvents()[1].value == KEY_VAL_UP);
    writer.clear();

    // Hold comma (>175ms) -> emits Shift + comma ('<')
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_COMMA, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 275000}); // 100 + 175
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_COMMA, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_COMMA && writer.keyEvents()[1].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[2].code == Keys::KEY_COMMA && writer.keyEvents()[2].value == KEY_VAL_UP);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[3].value == KEY_VAL_UP);
    writer.clear();

    // Hold dot (>175ms) -> emits Shift + dot ('>')
    engine.processEvent(Event{TimeVal{0, 400000}, EV_KEY, Keys::KEY_DOT, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 575000});
    engine.processEvent(Event{TimeVal{0, 600000}, EV_KEY, Keys::KEY_DOT, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_DOT && writer.keyEvents()[1].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[2].code == Keys::KEY_DOT && writer.keyEvents()[2].value == KEY_VAL_UP);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[3].value == KEY_VAL_UP);

    std::cout << "test_auto_shift_punctuation_symbols: PASSED\n";
}

static void test_auto_shift_modifier_coexistence() {
    MockWriter writer;
    TFFEngine engine(&writer);

    AutoShiftConfig asc;
    asc.enabled = true;
    asc.timeout_us = 175000LL;
    asc.keys = {Keys::KEY_C};
    engine.setAutoShiftConfig(asc);

    // 1. Press LeftCtrl down
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTCTRL && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(engine.isModifierActive());

    // 2. Press 'c' down while Ctrl is held -> Auto-Shift MUST be bypassed immediately!
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_C && writer.keyEvents()[1].value == KEY_VAL_DOWN);

    // 3. Release 'c'
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_C, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 3);
    assert(writer.keyEvents()[2].code == Keys::KEY_C && writer.keyEvents()[2].value == KEY_VAL_UP);

    // 4. Release LeftCtrl
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTCTRL && writer.keyEvents()[3].value == KEY_VAL_UP);
    assert(!engine.isModifierActive());

    std::cout << "test_auto_shift_modifier_coexistence: PASSED\n";
}

static void test_auto_shift_tap_hold_and_oneshot_modifier_coexistence() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
tap_hold:
  capslock: [esc, leftctrl, 200]
one_shot:
  leftalt: 1000
auto_shift:
  enabled: true
  timeout_ms: 175
  keys: letters
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    // 1. Dual-role Tap-Hold modifier coexistence:
    // Hold CapsLock down, then press 'c'. Chording promotes CapsLock to LeftCtrl DOWN,
    // and 'c' MUST emit DOWN immediately without auto-shift buffering/delay!
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    // Fast chord: 'c' goes down
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTCTRL && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_C && writer.keyEvents()[1].value == KEY_VAL_DOWN);
    assert(engine.isModifierActive());

    // Release 'c'
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_C, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 3);
    assert(writer.keyEvents()[2].code == Keys::KEY_C && writer.keyEvents()[2].value == KEY_VAL_UP);

    // Release CapsLock
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTCTRL && writer.keyEvents()[3].value == KEY_VAL_UP);
    assert(!engine.isModifierActive());
    writer.clear();

    // 2. One-Shot modifier coexistence:
    // Tap LeftAlt -> arms OSM LeftAlt.
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_LEFTALT, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_LEFTALT, KEY_VAL_UP});
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTALT));
    assert(engine.isModifierActive());

    // Now press 't' (which is in auto_shift letters) -> OSM Alt activates and auto-shift is bypassed immediately!
    engine.processEvent(Event{TimeVal{0, 140000}, EV_KEY, Keys::KEY_T, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTALT && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_T && writer.keyEvents()[1].value == KEY_VAL_DOWN);

    // Release 't' -> both T and Alt release
    engine.processEvent(Event{TimeVal{0, 160000}, EV_KEY, Keys::KEY_T, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[2].code == Keys::KEY_T && writer.keyEvents()[2].value == KEY_VAL_UP);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTALT && writer.keyEvents()[3].value == KEY_VAL_UP);
    assert(!engine.isModifierActive());

    std::cout << "test_auto_shift_tap_hold_and_oneshot_modifier_coexistence: PASSED\n";
}

static void test_auto_shift_with_combos() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
combos:
  d + f: esc
auto_shift:
  enabled: true
  timeout_ms: 175
  keys: letters
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    // 1. Press d and f together within combo window -> triggers Esc combo!
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_ESC && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_ESC && writer.keyEvents()[1].value == KEY_VAL_UP);
    writer.clear();

    // 2. Press 'a' alone (not in combo) and hold past timeout -> capital 'A'
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 275000}); // 100 + 175
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTSHIFT);
    assert(writer.keyEvents()[1].code == Keys::KEY_A);

    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[2].code == Keys::KEY_A && writer.keyEvents()[2].value == KEY_VAL_UP);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTSHIFT && writer.keyEvents()[3].value == KEY_VAL_UP);

    std::cout << "test_auto_shift_with_combos: PASSED\n";
}

static void test_auto_shift_with_modal_layers() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
tap_hold:
  space: [space, nav, 200]
layers:
  nav:
    j: down
auto_shift:
  enabled: true
  timeout_ms: 175
  keys: letters
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    // 1. Hold Space (activates nav layer via chord with 'j')
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    // Chord 'j' -> remapped to 'down' arrow immediately
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_DOWN && writer.keyEvents()[0].value == KEY_VAL_DOWN);

    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_DOWN && writer.keyEvents()[1].value == KEY_VAL_UP);

    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    writer.clear();

    // 2. Now tap 'j' normally -> lowercase 'j'
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_J && writer.keyEvents()[0].value == KEY_VAL_DOWN);
    assert(writer.keyEvents()[1].code == Keys::KEY_J && writer.keyEvents()[1].value == KEY_VAL_UP);

    std::cout << "test_auto_shift_with_modal_layers: PASSED\n";
}

static void test_auto_shift_yaml_parsing_and_presets() {
    Config config;
    std::string err;

    // 1. Default presets (omitted keys -> letters)
    const std::string y1 = R"(
auto_shift:
  enabled: true
  timeout_ms: 180
)";
    assert(loadYamlConfig(y1, config, err));
    assert(config.auto_shift.enabled);
    assert(config.auto_shift.timeout_us == 180000LL);
    assert(config.auto_shift.keys.size() == 26); // 26 alphabet letters

    // 2. Numbers preset
    const std::string y2 = R"(
auto_shift:
  keys: numbers
)";
    assert(loadYamlConfig(y2, config, err));
    assert(config.auto_shift.enabled);
    assert(config.auto_shift.keys.size() == 10); // 0-9

    // 3. Symbols preset
    const std::string y3 = R"(
auto_shift:
  keys: symbols
)";
    assert(loadYamlConfig(y3, config, err));
    assert(config.auto_shift.keys.size() == 11);

    // 4. All preset
    const std::string y4 = R"(
auto_shift:
  keys: all
)";
    assert(loadYamlConfig(y4, config, err));
    assert(config.auto_shift.keys.size() == 26 + 10 + 11);

    // 5. Explicit key list
    const std::string y5 = R"(
auto_shift:
  keys: [a, b, c, comma, period]
)";
    assert(loadYamlConfig(y5, config, err));
    assert(config.auto_shift.keys.size() == 5);

    // 6. Explicit enabled: false
    const std::string y6 = R"(
auto_shift:
  enabled: false
  keys: letters
)";
    assert(loadYamlConfig(y6, config, err));
    assert(!config.auto_shift.enabled);

    std::cout << "test_auto_shift_yaml_parsing_and_presets: PASSED\n";
}

static void test_auto_shift_yaml_validation_errors() {
    Config config;
    std::string err;

    // 1. Negative timeout
    const std::string y1 = R"(
auto_shift:
  timeout_ms: -10
)";
    assert(!loadYamlConfig(y1, config, err));
    assert(err.find("auto_shift timeout_ms must be positive") != std::string::npos);

    // 2. Zero timeout
    const std::string y2 = R"(
auto_shift:
  timeout_ms: 0
)";
    assert(!loadYamlConfig(y2, config, err));
    assert(err.find("auto_shift timeout_ms must be positive") != std::string::npos);

    // 3. Unknown key in keys list
    const std::string y3 = R"(
auto_shift:
  keys: [a, b, nonexistent_key_xyz]
)";
    assert(!loadYamlConfig(y3, config, err));
    assert(err.find("unknown key or preset in auto_shift keys") != std::string::npos);

    // 4. Invalid enabled boolean
    const std::string y4 = R"(
auto_shift:
  enabled: maybe
)";
    assert(!loadYamlConfig(y4, config, err));
    assert(err.find("invalid boolean for auto_shift enabled") != std::string::npos);

    std::cout << "test_auto_shift_yaml_validation_errors: PASSED\n";
}

static void test_auto_shift_cheatsheet() {
    Config config;
    std::string err;
    const std::string yaml = R"(
auto_shift:
  enabled: true
  timeout_ms: 175
  keys: letters
)";
    assert(loadYamlConfig(yaml, config, err));

    CheatsheetOptions opts_md;
    opts_md.markdown = true;
    std::string md = Cheatsheet::generate(config, opts_md);
    assert(md.find("## Auto-Shift (Long-Press Capitalization)") != std::string::npos);
    assert(md.find("175 ms") != std::string::npos);
    assert(md.find("26 keys active") != std::string::npos);

    CheatsheetOptions opts_term;
    opts_term.markdown = false;
    opts_term.color = false;
    std::string term = Cheatsheet::generate(config, opts_term);
    assert(term.find("Auto-Shift (Long-Press Capitalization)") != std::string::npos);
    assert(term.find("175ms") != std::string::npos);
    assert(term.find("Active Keys: 26") != std::string::npos);

    std::cout << "test_auto_shift_cheatsheet: PASSED\n";
}

int main() {
    std::cout << "Running Auto-Shift unit tests...\n";
    test_auto_shift_tap_lowercase();
    test_auto_shift_hold_uppercase();
    test_auto_shift_fast_typing_rolls();
    test_auto_shift_roll_then_hold();
    test_auto_shift_punctuation_symbols();
    test_auto_shift_modifier_coexistence();
    test_auto_shift_tap_hold_and_oneshot_modifier_coexistence();
    test_auto_shift_with_combos();
    test_auto_shift_with_modal_layers();
    test_auto_shift_yaml_parsing_and_presets();
    test_auto_shift_yaml_validation_errors();
    test_auto_shift_cheatsheet();
    std::cout << "All 12 Auto-Shift unit tests passed successfully!\n";
    return 0;
}
