#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include "tff_cheatsheet.h"
#include "rp2040_platform.h"
#include <cassert>
#include <iostream>
#include <vector>
#include <string>

using namespace tff;

namespace {

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

}  // anonymous namespace

// 1. Single tap fast-commit: when only tap and hold are defined (no multi-tap),
// key release should immediately commit tap without waiting for timeout.
static void test_tap_dance_single_tap_fast_commit() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.timeout_us = 200000LL;  // 200ms
    engine.setTapDances({td});

    // Press down at 10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // Release at 60ms (< 200ms)
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    // Fast commit: ESC DOWN and ESC UP immediately emitted
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_ESC && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_ESC && evs[1].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_single_tap_fast_commit: PASSED\n";
}

// 2. Single tap with multi-tap defined: when double_tap is configured,
// release waits for timeout before committing single tap.
static void test_tap_dance_single_tap_with_multitap_defined() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.double_tap.out_keys = {Keys::KEY_CAPSLOCK};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.timeout_us = 200000LL;  // 200ms
    engine.setTapDances({td});

    // Press down at 10ms, release at 50ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    // Not yet committed, waiting to see if 2nd tap occurs
    assert(writer.keyEvents().empty());
    assert(engine.hasActiveTimer());

    // Timer expires at 250ms (50ms + 200ms)
    engine.onTimer(TimeVal{0, 250000});

    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_ESC && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_ESC && evs[1].value == KEY_VAL_UP);
    assert(!engine.hasActiveTimer());

    std::cout << "test_tap_dance_single_tap_with_multitap_defined: PASSED\n";
}

// 3. Double tap: two fast taps within timeout triggers double_tap action
static void test_tap_dance_double_tap() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.double_tap.out_keys = {Keys::KEY_CAPSLOCK};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // 1st tap: down at 10ms, up at 40ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().empty());

    // 2nd tap: down at 90ms (< 240ms), up at 130ms
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    engine.processEvent(Event{TimeVal{0, 130000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    // Since no triple_tap or double_hold is configured, release immediately commits double tap
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_CAPSLOCK && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_CAPSLOCK && evs[1].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_double_tap: PASSED\n";
}

// 4. Triple tap: three fast taps triggers triple_tap action
static void test_tap_dance_triple_tap() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_A;
    td.tap.out_keys = {Keys::KEY_1};
    td.double_tap.out_keys = {Keys::KEY_2};
    td.triple_tap.out_keys = {Keys::KEY_3};
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // 1st tap: down at 10ms, up at 30ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});

    // 2nd tap: down at 60ms, up at 80ms
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});

    // 3rd tap: down at 110ms, up at 130ms
    engine.processEvent(Event{TimeVal{0, 110000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 130000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});

    // Reached max tap count (3) -> immediately commits 3rd tap
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_3 && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_3 && evs[1].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_triple_tap: PASSED\n";
}

// 5. Single hold: holding beyond timeout fires hold action
static void test_tap_dance_single_hold() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.timeout_us = 200000LL;  // 200ms
    engine.setTapDances({td});

    // Press down at 10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // Timer expires at 210ms (10ms + 200ms)
    engine.onTimer(TimeVal{0, 210000});

    // Hold action (LEFTCTRL) emitted DOWN
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTCTRL &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // Release at 300ms -> LEFTCTRL emitted UP
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_LEFTCTRL &&
           writer.keyEvents()[1].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_single_hold: PASSED\n";
}

// 6. Double tap hold: tap once, tap second time and hold past timeout fires double_hold
static void test_tap_dance_double_tap_hold() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.double_tap.out_keys = {Keys::KEY_CAPSLOCK};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.double_hold.out_keys = {Keys::KEY_RIGHTCTRL};
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // 1st tap: down at 10ms, up at 40ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    // 2nd tap down at 80ms and held
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // Timer expires at 280ms (80ms + 200ms)
    engine.onTimer(TimeVal{0, 280000});

    // RIGHTCTRL emitted DOWN
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_RIGHTCTRL &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // Release at 350ms
    engine.processEvent(Event{TimeVal{0, 350000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_RIGHTCTRL &&
           writer.keyEvents()[1].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_double_tap_hold: PASSED\n";
}

// 7. Permissive hold rollover: Key A held, Key B pressed before timeout ->
// promotes A to hold and then processes B
static void test_tap_dance_rollover_while_held() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // Caplock down at 10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // Key B down at 50ms (< 200ms)
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});

    // LEFTCTRL promoted DOWN, then B emitted DOWN
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_LEFTCTRL && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_B && evs[1].value == KEY_VAL_DOWN);

    // Release B at 80ms
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 3);
    assert(writer.keyEvents()[2].code == Keys::KEY_B && writer.keyEvents()[2].value == KEY_VAL_UP);

    // Release Capslock at 120ms
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);
    assert(writer.keyEvents()[3].code == Keys::KEY_LEFTCTRL &&
           writer.keyEvents()[3].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_rollover_while_held: PASSED\n";
}

// 8. Rollover after tap: Key A tapped and released (waiting for multi-tap),
// then Key B pressed -> immediately commits A's tap count, then processes B.
static void test_tap_dance_rollover_after_tap() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.double_tap.out_keys = {Keys::KEY_CAPSLOCK};
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // Tap Capslock: down at 10ms, up at 40ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().empty());

    // Press Key B at 70ms (< 240ms)
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});

    // ESC committed DOWN and UP, then B emitted DOWN
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 3);
    assert(evs[0].code == Keys::KEY_ESC && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_ESC && evs[1].value == KEY_VAL_UP);
    assert(evs[2].code == Keys::KEY_B && evs[2].value == KEY_VAL_DOWN);

    std::cout << "test_tap_dance_rollover_after_tap: PASSED\n";
}

// 9. Momentary layer activation on hold and double_hold
static void test_tap_dance_momentary_layer() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});

    TapDance td;
    td.key = Keys::KEY_SPACE;
    td.tap.out_keys = {Keys::KEY_SPACE};
    td.hold.layer = "nav";
    td.timeout_us = 200000LL;

    Config cfg;
    cfg.layers = {nav_layer};
    cfg.tap_dances = {td};
    engine.setConfig(cfg);

    // Press space at 10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});

    // Hold timeout expires at 210ms -> activates 'nav' layer
    engine.onTimer(TimeVal{0, 210000});
    assert(engine.isLayerActive("nav"));

    // Press H at 230ms -> remapped to LEFT
    engine.processEvent(Event{TimeVal{0, 230000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFT &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // Release H at 260ms
    engine.processEvent(Event{TimeVal{0, 260000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_LEFT &&
           writer.keyEvents()[1].value == KEY_VAL_UP);

    // Release space at 300ms -> deactivates 'nav' layer
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(!engine.isLayerActive("nav"));

    std::cout << "test_tap_dance_momentary_layer: PASSED\n";
}

// 10. Toggle layer on double tap
static void test_tap_dance_toggle_layer() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer num_layer;
    num_layer.name = "numpad";
    num_layer.mappings[Keys::KEY_M] = LayerAction({Keys::KEY_0});

    TapDance td;
    td.key = Keys::KEY_N;
    td.tap.out_keys = {Keys::KEY_N};
    td.double_tap.toggle_layer = "numpad";
    td.timeout_us = 200000LL;

    Config cfg;
    cfg.layers = {num_layer};
    cfg.tap_dances = {td};
    engine.setConfig(cfg);

    // Double tap N: down 10, up 30, down 60, up 80
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_N, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_N, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_N, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_N, KEY_VAL_UP});

    // Numpad layer should now be active!
    assert(engine.isLayerActive("numpad"));

    // Press M -> emitted as 0
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_0 &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});

    // Double tap N again -> deactivates numpad layer
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_N, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 170000}, EV_KEY, Keys::KEY_N, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_N, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 220000}, EV_KEY, Keys::KEY_N, KEY_VAL_UP});

    assert(!engine.isLayerActive("numpad"));

    std::cout << "test_tap_dance_toggle_layer: PASSED\n";
}

// 11. Text snippets and mouse actions in tap dance
static void test_tap_dance_text_and_mouse() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_G;
    td.tap.text = "git";
    td.double_tap.out_keys = {Keys::BTN_LEFT};
    td.double_tap.mouse.type = MouseActionType::BtnLeft;
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // 1st tap down at 10, up at 40
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_G, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_G, KEY_VAL_UP});

    // Timeout expires -> emits 'g', 'i', 't'
    engine.onTimer(TimeVal{0, 250000});
    const auto& evs = writer.keyEvents();
    assert(evs.size() == 6);  // 3 keys * (down + up)
    assert(evs[0].code == Keys::KEY_G && evs[0].value == KEY_VAL_DOWN);
    assert(evs[2].code == Keys::KEY_I && evs[2].value == KEY_VAL_DOWN);
    assert(evs[4].code == Keys::KEY_T && evs[4].value == KEY_VAL_DOWN);

    writer.clear();

    // Double tap -> emits BTN_LEFT
    engine.processEvent(Event{TimeVal{1, 10000}, EV_KEY, Keys::KEY_G, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{1, 30000}, EV_KEY, Keys::KEY_G, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{1, 60000}, EV_KEY, Keys::KEY_G, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{1, 80000}, EV_KEY, Keys::KEY_G, KEY_VAL_UP});

    const auto& btn_evs = writer.keyEvents();
    assert(btn_evs.size() == 2);
    assert(btn_evs[0].code == Keys::BTN_LEFT && btn_evs[0].value == KEY_VAL_DOWN);
    assert(btn_evs[1].code == Keys::BTN_LEFT && btn_evs[1].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_text_and_mouse: PASSED\n";
}

// 12. YAML parser formats
static void test_tap_dance_yaml_parser() {
    std::string yaml = R"(
tap_dance:
  capslock:
    tap: esc
    double_tap: capslock
    hold: lctrl
    double_hold: rctrl
    triple_tap: text "DONE"
    timeout_ms: 220
  - key: semicolon
    tap: semicolon
    hold: layer(nav)
  tab: { tap: tab, double_tap: toggle_layer(nav) }
  grave: [grave, esc, 180]

layers:
  nav:
    h: left
)";

    Config config;
    std::string err;
    bool ok = loadYamlConfig(yaml, config, err);
    if (!ok) {
        std::cerr << "Parser error: " << err << "\n";
    }
    assert(ok);
    assert(config.tap_dances.size() == 4);

    // capslock
    assert(config.tap_dances[0].key == Keys::KEY_CAPSLOCK);
    assert(config.tap_dances[0].tap.out_keys == std::vector<KeyCode>{Keys::KEY_ESC});
    assert(config.tap_dances[0].double_tap.out_keys == std::vector<KeyCode>{Keys::KEY_CAPSLOCK});
    assert(config.tap_dances[0].hold.out_keys == std::vector<KeyCode>{Keys::KEY_LEFTCTRL});
    assert(config.tap_dances[0].double_hold.out_keys == std::vector<KeyCode>{Keys::KEY_RIGHTCTRL});
    assert(config.tap_dances[0].triple_tap.text == "DONE");
    assert(config.tap_dances[0].timeout_us == 220000LL);

    // semicolon
    assert(config.tap_dances[1].key == Keys::KEY_SEMICOLON);
    assert(config.tap_dances[1].tap.out_keys == std::vector<KeyCode>{Keys::KEY_SEMICOLON});
    assert(config.tap_dances[1].hold.layer == "nav");

    // tab
    assert(config.tap_dances[2].key == Keys::KEY_TAB);
    assert(config.tap_dances[2].tap.out_keys == std::vector<KeyCode>{Keys::KEY_TAB});
    assert(config.tap_dances[2].double_tap.toggle_layer == "nav");

    // grave
    assert(config.tap_dances[3].key == Keys::KEY_GRAVE);
    assert(config.tap_dances[3].tap.out_keys == std::vector<KeyCode>{Keys::KEY_GRAVE});
    assert(config.tap_dances[3].double_tap.out_keys == std::vector<KeyCode>{Keys::KEY_ESC});
    assert(config.tap_dances[3].timeout_us == 180000LL);

    std::cout << "test_tap_dance_yaml_parser: PASSED\n";
}

// 13. YAML parser validation errors
static void test_tap_dance_validation_errors() {
    Config config;
    std::string err;

    // Duplicate tap dance key
    std::string dup_yaml = R"(
tap_dance:
  capslock:
    tap: esc
  capslock:
    hold: lctrl
)";
    assert(!loadYamlConfig(dup_yaml, config, err));
    assert(err.find("duplicate tap_dance key") != std::string::npos);

    // Conflict with tap_hold
    std::string conflict_th = R"(
tap_hold:
  capslock:
    tap: esc
    hold: lctrl
tap_dance:
  capslock:
    tap: esc
)";
    assert(!loadYamlConfig(conflict_th, config, err));
    assert(err.find("cannot be used in both") != std::string::npos);

    // Unknown layer
    std::string unk_layer = R"(
tap_dance:
  capslock:
    tap: esc
    hold: layer(nonexistent)
)";
    assert(!loadYamlConfig(unk_layer, config, err));
    assert(err.find("unknown layer") != std::string::npos);

    std::cout << "test_tap_dance_validation_errors: PASSED\n";
}

// 14. Cheatsheet display
static void test_tap_dance_cheatsheet() {
    std::string yaml = R"(
tap_dance:
  capslock:
    tap: esc
    double_tap: capslock
    hold: lctrl
    double_hold: rctrl
    timeout_ms: 200
)";

    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));

    CheatsheetOptions md_opts;
    md_opts.markdown = true;
    std::string md = Cheatsheet::generate(config, md_opts);
    assert(md.find("## Tap Dance Keys (Multi-Tap & Tap-Hold)") != std::string::npos);
    assert(md.find("| `capslock` | `esc` | `capslock` | `ctrl` | `rightctrl` | - | 200ms |") !=
           std::string::npos);

    CheatsheetOptions term_opts;
    term_opts.markdown = false;
    term_opts.color = false;
    std::string term = Cheatsheet::generate(config, term_opts);
    assert(term.find("[ Tap Dance Keys (Multi-Tap & Tap-Hold) ]") != std::string::npos);
    assert(term.find("capslock") != std::string::npos);
    assert(term.find("esc") != std::string::npos);

    std::cout << "test_tap_dance_cheatsheet: PASSED\n";
}

// 15. RP2040 integration
static void test_tap_dance_rp2040_integration() {
    RP2040Platform platform;
    std::string yaml = R"(
tap_dance:
  capslock:
    tap: esc
    hold: lctrl
    timeout_ms: 200
)";
    bool loaded = platform.loadConfiguration(yaml);
    assert(loaded);

    // Tap capslock (USB HID 0x39 = KEY_CAPSLOCK): press at 10ms, release at 50ms
    platform.setTimestamp(10);
    platform.processHostKeyEvent(0x39, true);  // capslock down

    platform.setTimestamp(50);
    platform.processHostKeyEvent(0x39, false);  // capslock up

    // Fast commit on release: ESC emitted
    const auto& emitted = platform.getEmittedKeys();
    assert(!emitted.empty());
    assert(emitted.back() == tff::Keys::KEY_ESC);

    std::cout << "test_tap_dance_rp2040_integration: PASSED\n";
}

// 16. Compact list with tokens ending in 's' (e.g. minus, scrolllock, 200)
static void test_tap_dance_compact_list_keys_ending_in_s() {
    std::string yaml = R"(
tap_dance:
  tab: [tab, minus, 200]
  grave: [grave, scrolllock, 180ms]
)";
    Config cfg;
    std::string err;
    bool ok = loadYamlConfig(yaml, cfg, err);
    if (!ok) {
        std::cerr << "Parser error: " << err << "\n";
    }
    assert(ok);
    assert(cfg.tap_dances.size() == 2);
    assert(cfg.tap_dances[0].double_tap.out_keys == std::vector<KeyCode>{Keys::KEY_MINUS});
    assert(cfg.tap_dances[0].timeout_us == 200000LL);
    assert(cfg.tap_dances[1].double_tap.out_keys == std::vector<KeyCode>{Keys::KEY_SCROLLLOCK});
    assert(cfg.tap_dances[1].timeout_us == 180000LL);
    std::cout << "test_tap_dance_compact_list_keys_ending_in_s: PASSED\n";
}

// 17. Rollover between two distinct Tap Dance keys
static void test_tap_dance_two_key_rollover() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td1;
    td1.key = Keys::KEY_A;
    td1.tap.out_keys = {Keys::KEY_A};
    td1.double_tap.out_keys = {Keys::KEY_ESC};
    td1.timeout_us = 200000LL;

    TapDance td2;
    td2.key = Keys::KEY_B;
    td2.tap.out_keys = {Keys::KEY_B};
    td2.double_tap.out_keys = {Keys::KEY_TAB};
    td2.timeout_us = 200000LL;

    engine.setTapDances({td1, td2});

    // Tap A (down at 10ms, up at 30ms) -> WAITING_FOR_NEXT_TAP
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});

    // Quickly press B DOWN at 50ms (before A's timeout at 230ms)
    // A should commit its single tap (KEY_A), and B should start its tap dance!
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});

    // Release B UP at 70ms
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP});

    // Advance time past B's timeout
    engine.onTimer(TimeVal{0, 300000});

    const auto& evs = writer.keyEvents();
    // Expected: A down, A up, B down, B up
    assert(evs.size() == 4);
    assert(evs[0].code == Keys::KEY_A && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_A && evs[1].value == KEY_VAL_UP);
    assert(evs[2].code == Keys::KEY_B && evs[2].value == KEY_VAL_DOWN);
    assert(evs[3].code == Keys::KEY_B && evs[3].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_two_key_rollover: PASSED\n";
}

// 18. Interruption during 3rd tap in WAITING_FOR_RELEASE
static void test_tap_dance_triple_tap_interruption() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_A;
    td.tap.out_keys = {Keys::KEY_A};
    td.double_tap.out_keys = {Keys::KEY_B};
    td.triple_tap.out_keys = {Keys::KEY_C};
    td.timeout_us = 200000LL;
    engine.setTapDances({td});

    // Tap 1: 10ms - 20ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    // Tap 2: 40ms - 50ms
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    // Tap 3 down: 70ms (WAITING_FOR_RELEASE, tap_count = 3)
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});

    // Interrupted by KEY_SPACE down at 80ms while A is physically down!
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});

    // Release A at 90ms, release SPACE at 100ms
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});

    const auto& evs = writer.keyEvents();
    // Expected: KEY_C down, KEY_C up (triple tap committed), then KEY_SPACE down, KEY_SPACE up
    assert(evs.size() == 4);
    assert(evs[0].code == Keys::KEY_C && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_C && evs[1].value == KEY_VAL_UP);
    assert(evs[2].code == Keys::KEY_SPACE && evs[2].value == KEY_VAL_DOWN);
    assert(evs[3].code == Keys::KEY_SPACE && evs[3].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_triple_tap_interruption: PASSED\n";
}

// 19. AutoShift suppression while holding a Tap Dance modifier
static void test_tap_dance_autoshift_modifier_held() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapDance td;
    td.key = Keys::KEY_CAPSLOCK;
    td.tap.out_keys = {Keys::KEY_ESC};
    td.hold.out_keys = {Keys::KEY_LEFTCTRL};
    td.timeout_us = 150000LL;
    engine.setTapDances({td});

    AutoShiftConfig as_cfg;
    as_cfg.enabled = true;
    as_cfg.timeout_us = 150000LL;
    as_cfg.keys = {Keys::KEY_C};
    engine.setAutoShiftConfig(as_cfg);

    // Press CapsLock DOWN -> promote to hold (timeout at 160ms)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 200000});

    // LEFTCTRL should now be held down
    assert(engine.isModifierActive());

    // While CapsLock is held, type 'c' (Auto-Shift enabled key)
    // Because a modifier (LEFTCTRL) is active via tap dance, AutoShift must NOT shift!
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_C, KEY_VAL_UP});

    // Release CapsLock
    engine.processEvent(Event{TimeVal{0, 350000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    const auto& evs = writer.keyEvents();
    // Verify no LEFTSHIFT was emitted:
    for (const auto& ev : evs) {
        assert(ev.code != Keys::KEY_LEFTSHIFT);
    }
    // Verify LEFTCTRL was down during 'c'
    assert(evs[0].code == Keys::KEY_LEFTCTRL && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_C && evs[1].value == KEY_VAL_DOWN);
    assert(evs[2].code == Keys::KEY_C && evs[2].value == KEY_VAL_UP);
    assert(evs[3].code == Keys::KEY_LEFTCTRL && evs[3].value == KEY_VAL_UP);

    std::cout << "test_tap_dance_autoshift_modifier_held: PASSED\n";
}

int main() {
    std::cout << "Running Tap Dance unit test suite...\n";

    test_tap_dance_single_tap_fast_commit();
    test_tap_dance_single_tap_with_multitap_defined();
    test_tap_dance_double_tap();
    test_tap_dance_triple_tap();
    test_tap_dance_single_hold();
    test_tap_dance_double_tap_hold();
    test_tap_dance_rollover_while_held();
    test_tap_dance_rollover_after_tap();
    test_tap_dance_momentary_layer();
    test_tap_dance_toggle_layer();
    test_tap_dance_text_and_mouse();
    test_tap_dance_yaml_parser();
    test_tap_dance_validation_errors();
    test_tap_dance_cheatsheet();
    test_tap_dance_rp2040_integration();
    test_tap_dance_compact_list_keys_ending_in_s();
    test_tap_dance_two_key_rollover();
    test_tap_dance_triple_tap_interruption();
    test_tap_dance_autoshift_modifier_held();

    std::cout << "\nAll Tap Dance tests PASSED successfully!\n";
    return 0;
}
