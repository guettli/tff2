#include "tff_cheatsheet.h"
#include "tff_engine.h"
#include "tff_key_codes.h"
#include "tff_parser.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace tff;

namespace {

class TrackingWriter : public EventWriter {
public:
    std::vector<Event> all_events;
    std::vector<Event> key_events;
    std::set<KeyCode> active_keys;
    size_t spurious_ups = 0;
    size_t duplicate_downs = 0;

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

    void clear() {
        all_events.clear();
        key_events.clear();
        active_keys.clear();
        spurious_ups = 0;
        duplicate_downs = 0;
    }

    void assertZeroStuckKeys(const std::string& ctx = "") const {
        if (!active_keys.empty()) {
            std::cerr << "FAILED [" << ctx << "]: Stuck keys remaining: ";
            for (KeyCode k : active_keys) {
                std::cerr << keyCodeToWord(k) << " ";
            }
            std::cerr << std::endl;
            assert(active_keys.empty());
        }
        assert(spurious_ups == 0);
        assert(duplicate_downs == 0);
    }
};

}  // anonymous namespace

// 1. Inactive vs active layer combo trigger
static void test_layer_combo_inactive_vs_active() {
    TrackingWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    std::string yaml =
        "tap_hold:\n"
        "  space: [space, nav, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n"
        "    l: right\n"
        "    h + l: end\n";
    assert(loadYamlConfig(yaml, cfg, err));
    engine.setConfig(cfg);

    // Press H and L in base layer (space NOT held)
    engine.processEvent(Event{TimeVal::fromMicros(10000), EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_L, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(200000));
    engine.processEvent(Event{TimeVal::fromMicros(210000), EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(220000), EV_KEY, Keys::KEY_L, KEY_VAL_UP});
    engine.finish();

    // H + L in base layer should NOT trigger 'end'
    bool found_end = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_END) {
            found_end = true;
        }
    }
    assert(!found_end);
    writer.assertZeroStuckKeys("base layer h+l");

    // Now hold Space to activate 'nav' layer, then press H + L
    writer.clear();
    engine.reset();
    engine.setConfig(cfg);

    // Space down at t=100ms
    engine.processEvent(Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    // H down at t=110ms (promotes Space to hold nav layer)
    engine.processEvent(Event{TimeVal::fromMicros(110000), EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    // L down at t=120ms
    engine.processEvent(Event{TimeVal::fromMicros(120000), EV_KEY, Keys::KEY_L, KEY_VAL_DOWN});
    // Timer / chord detection at t=300ms (> 140ms min_age)
    engine.onTimer(TimeVal::fromMicros(300000));

    // 'end' should now have triggered DOWN!
    found_end = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_END && ev.value == KEY_VAL_DOWN) {
            found_end = true;
        }
    }
    assert(found_end);

    // Release H and L and Space
    engine.processEvent(Event{TimeVal::fromMicros(320000), EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(330000), EV_KEY, Keys::KEY_L, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(340000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    engine.finish();

    writer.assertZeroStuckKeys("active nav h+l");
    std::cout << "  [PASS] Layer combo inactive vs active trigger" << std::endl;
}

// 2. Layer-scoped combo overriding global combo
static void test_layer_combo_overriding_global() {
    TrackingWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    std::string yaml =
        "combos:\n"
        "  d + f: backspace\n"
        "tap_hold:\n"
        "  capslock: [esc, nav, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    d + f: pageup\n";
    assert(loadYamlConfig(yaml, cfg, err));
    engine.setConfig(cfg);

    // Base layer: D + F triggers backspace
    engine.processEvent(Event{TimeVal::fromMicros(10000), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(20000), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(200000));
    engine.processEvent(Event{TimeVal::fromMicros(210000), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(220000), EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    engine.finish();

    bool has_backspace = false;
    bool has_pageup = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_BACKSPACE)
            has_backspace = true;
        if (ev.code == Keys::KEY_PAGEUP)
            has_pageup = true;
    }
    assert(has_backspace);
    assert(!has_pageup);
    writer.assertZeroStuckKeys("base layer d+f");

    // Nav layer: D + F triggers pageup instead of backspace
    writer.clear();
    engine.reset();
    engine.setConfig(cfg);

    engine.processEvent(
        Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(110000), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(120000), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(300000));
    engine.processEvent(Event{TimeVal::fromMicros(310000), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(320000), EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(330000), EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    engine.finish();

    has_backspace = false;
    has_pageup = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_BACKSPACE)
            has_backspace = true;
        if (ev.code == Keys::KEY_PAGEUP)
            has_pageup = true;
    }
    assert(!has_backspace);
    assert(has_pageup);
    writer.assertZeroStuckKeys("nav layer d+f overriding global");
    std::cout << "  [PASS] Layer combo overriding global combo" << std::endl;
}

// 3. Layer stack precedence with nested layers
static void test_nested_layer_combos_priority() {
    TrackingWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    std::string yaml =
        "tap_hold:\n"
        "  space: [space, nav, 150ms]\n"
        "  tab: [tab, num, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    j + k: down\n"
        "  num:\n"
        "    j + k: kp1\n";
    assert(loadYamlConfig(yaml, cfg, err));
    engine.setConfig(cfg);

    // Hold Space (nav), then hold Tab (num on top of nav)
    engine.processEvent(Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(300000));
    assert(engine.isLayerActive("nav"));

    engine.processEvent(Event{TimeVal::fromMicros(310000), EV_KEY, Keys::KEY_TAB, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(500000));
    assert(engine.isLayerActive("num"));

    // Press J + K chord -> top of stack 'num' should win (kp1 instead of down)
    engine.processEvent(Event{TimeVal::fromMicros(510000), EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(520000), EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(700000));

    bool has_kp1 = false;
    bool has_down = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_KP1)
            has_kp1 = true;
        if (ev.code == Keys::KEY_DOWN)
            has_down = true;
    }
    assert(has_kp1);
    assert(!has_down);

    engine.processEvent(Event{TimeVal::fromMicros(710000), EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(720000), EV_KEY, Keys::KEY_K, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(730000), EV_KEY, Keys::KEY_TAB, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(740000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    engine.finish();

    writer.assertZeroStuckKeys("nested layer stack precedence");
    std::cout << "  [PASS] Layer stack precedence with nested layers" << std::endl;
}

// 4. Layer combo with actions (text snippets, mouse, toggle)
static void test_layer_combo_with_actions() {
    TrackingWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    std::string yaml =
        "tap_hold:\n"
        "  space: [space, nav, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    p + r: { text: \"println!\" }\n"
        "    m + l: mouse_left(25)\n"
        "    t + g: toggle_layer(locked)\n"
        "  locked:\n"
        "    a: b\n";
    assert(loadYamlConfig(yaml, cfg, err));
    engine.setConfig(cfg);

    // Hold Space (nav)
    engine.processEvent(Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(300000));

    // Test text snippet inside layer combo (P + R)
    engine.processEvent(Event{TimeVal::fromMicros(310000), EV_KEY, Keys::KEY_P, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(320000), EV_KEY, Keys::KEY_R, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(500000));
    engine.processEvent(Event{TimeVal::fromMicros(510000), EV_KEY, Keys::KEY_P, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(520000), EV_KEY, Keys::KEY_R, KEY_VAL_UP});

    bool found_p = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_P && ev.value == KEY_VAL_DOWN)
            found_p = true;
    }
    assert(found_p);
    writer.assertZeroStuckKeys("layer combo text snippet");

    // Test mouse relative action inside layer combo (M + L)
    writer.clear();
    engine.processEvent(Event{TimeVal::fromMicros(610000), EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(620000), EV_KEY, Keys::KEY_L, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(800000));
    engine.processEvent(Event{TimeVal::fromMicros(810000), EV_KEY, Keys::KEY_M, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(820000), EV_KEY, Keys::KEY_L, KEY_VAL_UP});

    bool found_rel = false;
    for (const auto& ev : writer.all_events) {
        if (ev.type == EV_REL && ev.code == REL_X && ev.value == -25) {
            found_rel = true;
        }
    }
    assert(found_rel);

    // Test toggle layer inside layer combo (T + G)
    assert(!engine.isLayerActive("locked"));
    engine.processEvent(Event{TimeVal::fromMicros(910000), EV_KEY, Keys::KEY_T, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(920000), EV_KEY, Keys::KEY_G, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(1100000));
    engine.processEvent(Event{TimeVal::fromMicros(1110000), EV_KEY, Keys::KEY_T, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(1120000), EV_KEY, Keys::KEY_G, KEY_VAL_UP});
    assert(engine.isLayerActive("locked"));

    engine.processEvent(Event{TimeVal::fromMicros(1200000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    engine.finish();

    writer.assertZeroStuckKeys("layer combo with actions");
    std::cout << "  [PASS] Layer combo with actions (text, mouse, toggle)" << std::endl;
}

// 5. Fallback to single-key layer mapping when chord does not complete
static void test_single_key_timeout_fallback_in_layer() {
    TrackingWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    std::string yaml =
        "tap_hold:\n"
        "  space: [space, nav, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n"
        "    l: right\n"
        "    h + l: end\n";
    assert(loadYamlConfig(yaml, cfg, err));
    engine.setConfig(cfg);
    engine.setFakeActiveTimer(true);

    // Hold Space (nav)
    engine.processEvent(Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(300000));

    // Press H alone (does not press L). H is in combo h+l, so it buffers.
    engine.processEvent(Event{TimeVal::fromMicros(310000), EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    // Release H without pressing L -> chord cancelled, flushes buffered key as layer remap 'left'
    engine.processEvent(Event{TimeVal::fromMicros(570000), EV_KEY, Keys::KEY_H, KEY_VAL_UP});

    bool found_left_down = false;
    bool found_left_up = false;
    for (const auto& ev : writer.key_events) {
        if (ev.code == Keys::KEY_LEFT) {
            if (ev.value == KEY_VAL_DOWN)
                found_left_down = true;
            else if (ev.value == KEY_VAL_UP)
                found_left_up = true;
        }
    }
    assert(found_left_down);
    assert(found_left_up);

    engine.processEvent(Event{TimeVal::fromMicros(600000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    engine.finish();

    writer.assertZeroStuckKeys("single-key layer action chord fallback");
    std::cout << "  [PASS] Single-key layer action chord timeout fallback" << std::endl;
}

// 6. Zero stuck keys when layer deactivates while chord is held
static void test_zero_stuck_keys_on_layer_deactivation_during_chord() {
    TrackingWriter writer;
    TFFEngine engine(&writer);

    Config cfg;
    std::string err;
    std::string yaml =
        "tap_hold:\n"
        "  space: [space, nav, 150ms]\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n"
        "    l: right\n"
        "    h + l: end\n";
    assert(loadYamlConfig(yaml, cfg, err));
    engine.setConfig(cfg);

    // Activate nav layer
    engine.processEvent(Event{TimeVal::fromMicros(100000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(300000));

    // Press H + L (triggers 'end')
    engine.processEvent(Event{TimeVal::fromMicros(310000), EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal::fromMicros(320000), EV_KEY, Keys::KEY_L, KEY_VAL_DOWN});
    engine.onTimer(TimeVal::fromMicros(500000));

    // Release Space while H and L are STILL PHYSICALLY HELD!
    engine.processEvent(Event{TimeVal::fromMicros(520000), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});

    // Release L, then H out-of-order
    engine.processEvent(Event{TimeVal::fromMicros(540000), EV_KEY, Keys::KEY_L, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal::fromMicros(560000), EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    engine.finish();

    writer.assertZeroStuckKeys("chord release after layer deactivation");
    std::cout << "  [PASS] Zero stuck keys on layer deactivation during chord" << std::endl;
}

// 7. YAML parser formats and validation checks
static void test_yaml_parser_layer_combos() {
    Config cfg;
    std::string err;

    // Test valid formats: layer block chord, property format, and compact inline dict
    std::string yaml_valid =
        "layers:\n"
        "  nav:\n"
        "    h + l: end\n"
        "    j + k: pageup\n"
        "combos:\n"
        "  - in: [a, b]\n"
        "    out: home\n"
        "    layer: nav\n"
        "  c + d: { out: enter, layer: nav }\n";
    assert(loadYamlConfig(yaml_valid, cfg, err));
    assert(!cfg.combos.empty());
    for (const auto& c : cfg.combos) {
        assert(c.layer == "nav");
    }

    // Test rejection of unknown layer in combo
    Config bad_cfg;
    std::string bad_yaml =
        "layers:\n"
        "  nav:\n"
        "    h: left\n"
        "combos:\n"
        "  - in: [a, b]\n"
        "    out: home\n"
        "    layer: nonexistent_layer\n";
    assert(!loadYamlConfig(bad_yaml, bad_cfg, err));
    assert(err.find("unknown layer 'nonexistent_layer'") != std::string::npos);

    std::cout << "  [PASS] YAML parser formats and layer validation" << std::endl;
}

// 8. Cheatsheet display for layer combos
static void test_cheatsheet_layer_combos() {
    Config cfg;
    std::string err;
    std::string yaml =
        "layers:\n"
        "  nav:\n"
        "    h + l: end\n";
    assert(loadYamlConfig(yaml, cfg, err));

    CheatsheetOptions opts;
    opts.color = false;
    opts.markdown = true;
    std::string md = Cheatsheet::generate(cfg, opts);
    assert(md.find("(Layer: `nav`)") != std::string::npos);

    opts.markdown = false;
    std::string plain = Cheatsheet::generate(cfg, opts);
    assert(plain.find("(Layer: nav)") != std::string::npos);

    std::cout << "  [PASS] Cheatsheet display for layer combos" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  TFF2 - Layer-Scoped Combos Test Suite           " << std::endl;
    std::cout << "==================================================" << std::endl;

    test_layer_combo_inactive_vs_active();
    test_layer_combo_overriding_global();
    test_nested_layer_combos_priority();
    test_layer_combo_with_actions();
    test_single_key_timeout_fallback_in_layer();
    test_zero_stuck_keys_on_layer_deactivation_during_chord();
    test_yaml_parser_layer_combos();
    test_cheatsheet_layer_combos();

    std::cout << "==================================================" << std::endl;
    std::cout << "  All Layer-Scoped Combos tests passed (100%)     " << std::endl;
    std::cout << "==================================================" << std::endl;
    return 0;
}
