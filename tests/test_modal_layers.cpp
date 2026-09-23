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

static void test_momentary_layer_activation() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    nav_layer.mappings[Keys::KEY_J] = LayerAction({Keys::KEY_DOWN});
    nav_layer.mappings[Keys::KEY_K] = LayerAction({Keys::KEY_UP});
    nav_layer.mappings[Keys::KEY_L] = LayerAction({Keys::KEY_RIGHT});
    engine.setLayers({nav_layer});

    // Space down at t=10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // H down at t=30ms -> immediately promotes Space to hold (activates nav) and emits Left Down
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 1);
    assert(keys1[0].code == Keys::KEY_LEFT && keys1[0].value == KEY_VAL_DOWN);

    // H up at t=50ms -> emits Left Up
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    auto keys2 = writer.keyEvents();
    assert(keys2.size() == 2);
    assert(keys2[1].code == Keys::KEY_LEFT && keys2[1].value == KEY_VAL_UP);

    // Space up at t=70ms -> deactivates nav layer, no Space key emitted
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    auto keys3 = writer.keyEvents();
    assert(keys3.size() == 2);
    assert(engine.getActiveLayers().empty());

    std::cout << "test_momentary_layer_activation: PASSED\n";
}

static void test_key_passthrough_in_active_layer() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    engine.setLayers({nav_layer});

    // Space down at t=10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});

    // 'A' down at t=30ms (unmapped in nav) -> promotes Space to hold, 'A' passes through
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    // Let buffer flush 'A'
    engine.onTimer(TimeVal{0, 200000});

    // 'A' up at t=210ms
    engine.processEvent(Event{TimeVal{0, 210000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    engine.onTimer(TimeVal{0, 400000});

    // Space up at t=450ms
    engine.processEvent(Event{TimeVal{0, 450000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});

    auto key_evs = writer.keyEvents();
    bool saw_a_down = false;
    bool saw_a_up = false;
    for (const auto& ev : key_evs) {
        if (ev.code == Keys::KEY_A && ev.value == KEY_VAL_DOWN)
            saw_a_down = true;
        if (ev.code == Keys::KEY_A && ev.value == KEY_VAL_UP)
            saw_a_up = true;
    }
    assert(saw_a_down && saw_a_up);

    std::cout << "test_key_passthrough_in_active_layer: PASSED\n";
}

static void test_layer_deactivation_on_release() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    engine.setLayers({nav_layer});

    // 1. Space + H -> Left
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});

    writer.clear();
    assert(engine.getActiveLayers().empty());

    // 2. H alone after Space release -> regular H
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 260000});
    engine.processEvent(Event{TimeVal{0, 270000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    engine.onTimer(TimeVal{0, 430000});

    auto key_evs = writer.keyEvents();
    bool saw_h_down = false;
    bool saw_h_up = false;
    for (const auto& ev : key_evs) {
        if (ev.code == Keys::KEY_H && ev.value == KEY_VAL_DOWN)
            saw_h_down = true;
        if (ev.code == Keys::KEY_H && ev.value == KEY_VAL_UP)
            saw_h_up = true;
        assert(ev.code != Keys::KEY_LEFT);
    }
    assert(saw_h_down && saw_h_up);

    std::cout << "test_layer_deactivation_on_release: PASSED\n";
}

static void test_key_release_after_layer_deactivation() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    engine.setLayers({nav_layer});

    // Space down at t=10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});

    // H down at t=30ms (remapped to Left Down)
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFT &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // Space up at t=50ms (layer deactivates, but H is still physically held down!)
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(engine.getActiveLayers().empty());

    // H up at t=70ms -> must emit Left UP, not H UP! Prevents stuck keys.
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 2);
    assert(key_evs[1].code == Keys::KEY_LEFT && key_evs[1].value == KEY_VAL_UP);

    std::cout << "test_key_release_after_layer_deactivation: PASSED\n";
}

static void test_multi_key_chord_in_layer() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_W] = LayerAction({Keys::KEY_LEFTCTRL, Keys::KEY_RIGHT});
    engine.setLayers({nav_layer});

    // Space down, W down
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});

    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 2);
    assert(key_evs[0].code == Keys::KEY_LEFTCTRL && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_RIGHT && key_evs[1].value == KEY_VAL_DOWN);

    // W up -> releases in reverse order (Right Up, then LeftCtrl Up)
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    key_evs = writer.keyEvents();
    assert(key_evs.size() == 4);
    assert(key_evs[2].code == Keys::KEY_RIGHT && key_evs[2].value == KEY_VAL_UP);
    assert(key_evs[3].code == Keys::KEY_LEFTCTRL && key_evs[3].value == KEY_VAL_UP);

    // Space up
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);

    std::cout << "test_multi_key_chord_in_layer: PASSED\n";
}

static void test_text_snippet_in_layer() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav_layer;
    nav_layer.name = "nav";
    nav_layer.mappings[Keys::KEY_C] = LayerAction({}, "hi!");
    engine.setLayers({nav_layer});

    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_C, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});

    auto key_evs = writer.keyEvents();
    // 'h' (down+up), 'i' (down+up), '!' (shift down, 1 down, 1 up, shift up) = 2 + 2 + 4 = 8 events
    assert(key_evs.size() == 8);
    assert(key_evs[0].code == Keys::KEY_H && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_H && key_evs[1].value == KEY_VAL_UP);
    assert(key_evs[2].code == Keys::KEY_I && key_evs[2].value == KEY_VAL_DOWN);
    assert(key_evs[3].code == Keys::KEY_I && key_evs[3].value == KEY_VAL_UP);
    assert(key_evs[4].code == Keys::KEY_LEFTSHIFT && key_evs[4].value == KEY_VAL_DOWN);
    assert(key_evs[5].code == Keys::KEY_1 && key_evs[5].value == KEY_VAL_DOWN);
    assert(key_evs[6].code == Keys::KEY_1 && key_evs[6].value == KEY_VAL_UP);
    assert(key_evs[7].code == Keys::KEY_LEFTSHIFT && key_evs[7].value == KEY_VAL_UP);

    std::cout << "test_text_snippet_in_layer: PASSED\n";
}

static void test_nested_layers() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    nav.mappings[Keys::KEY_M] = LayerAction({Keys::KEY_DOWN});

    Layer numpad;
    numpad.name = "numpad";
    numpad.mappings[Keys::KEY_M] = LayerAction({Keys::KEY_KP0});
    numpad.mappings[Keys::KEY_J] = LayerAction({Keys::KEY_KP1});

    engine.setLayers({nav, numpad});

    // 1. Activate nav
    engine.activateLayer("nav");
    assert(engine.getActiveLayers().size() == 1);

    // M -> Down (from nav)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});
    auto evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_DOWN && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_DOWN && evs[1].value == KEY_VAL_UP);

    // 2. Activate numpad on top of nav
    writer.clear();
    engine.activateLayer("numpad");
    assert(engine.getActiveLayers().size() == 2);

    // M -> KP0 (numpad shadows nav)
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});
    evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_KP0 && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_KP0 && evs[1].value == KEY_VAL_UP);

    // H -> Left (numpad doesn't map H, falls through to nav)
    writer.clear();
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_LEFT && evs[0].value == KEY_VAL_DOWN);
    assert(evs[1].code == Keys::KEY_LEFT && evs[1].value == KEY_VAL_UP);

    // 3. Deactivate numpad
    engine.deactivateLayer("numpad");
    assert(engine.getActiveLayers().size() == 1);
    assert(engine.getActiveLayers()[0] == "nav");

    // M -> Down again (from nav)
    writer.clear();
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});
    evs = writer.keyEvents();
    assert(evs.size() == 2);
    assert(evs[0].code == Keys::KEY_DOWN && evs[0].value == KEY_VAL_DOWN);

    std::cout << "test_nested_layers: PASSED\n";
}

static void test_tap_vs_hold_for_layer_key() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    engine.setLayers({nav});

    // Tap Space (down at 10ms, up at 60ms; duration 50ms < 200ms)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    auto key_evs = writer.keyEvents();
    assert(key_evs.size() == 2);
    assert(key_evs[0].code == Keys::KEY_SPACE && key_evs[0].value == KEY_VAL_DOWN);
    assert(key_evs[1].code == Keys::KEY_SPACE && key_evs[1].value == KEY_VAL_UP);
    assert(engine.getActiveLayers().empty());

    std::cout << "test_tap_vs_hold_for_layer_key: PASSED\n";
}

static void test_timeout_hold_activation() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey thk;
    thk.key = Keys::KEY_SPACE;
    thk.tap_key = Keys::KEY_SPACE;
    thk.hold_layer = "nav";
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    engine.setLayers({nav});

    // Space down at 10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    assert(engine.getActiveLayers().empty());

    // Timer expires at 220ms (10ms + 200ms timeout)
    engine.onTimer(TimeVal{0, 220000});
    assert(!engine.getActiveLayers().empty());
    assert(engine.getActiveLayers()[0] == "nav");

    // Press H -> Left Down
    engine.processEvent(Event{TimeVal{0, 230000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFT &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // Release H -> Left Up
    engine.processEvent(Event{TimeVal{0, 240000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_LEFT &&
           writer.keyEvents()[1].value == KEY_VAL_UP);

    // Release Space -> deactivates nav
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(engine.getActiveLayers().empty());

    std::cout << "test_timeout_hold_activation: PASSED\n";
}

static void test_layer_stack_management_api() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav;
    nav.name = "nav";
    Layer numpad;
    numpad.name = "numpad";
    Layer fn;
    fn.name = "fn";
    engine.setLayers({nav, numpad, fn});

    assert(engine.getActiveLayers().empty());

    engine.activateLayer("nav");
    assert(engine.getActiveLayers().size() == 1 && engine.getActiveLayers().back() == "nav");

    engine.activateLayer("numpad");
    assert(engine.getActiveLayers().size() == 2 && engine.getActiveLayers().back() == "numpad");

    // Toggle existing layer -> removes it
    engine.toggleLayer("numpad");
    assert(engine.getActiveLayers().size() == 1 && engine.getActiveLayers().back() == "nav");

    // Toggle non-existing layer -> adds it
    engine.toggleLayer("fn");
    assert(engine.getActiveLayers().size() == 2 && engine.getActiveLayers().back() == "fn");

    // Reset clears everything
    engine.reset();
    assert(engine.getActiveLayers().empty());

    std::cout << "test_layer_stack_management_api: PASSED\n";
}

static void test_yaml_parser_layers_and_tap_hold() {
    std::string yaml = R"(
layers:
  nav:
    h: left
    j: down
    k: up
    l: right
    w: ctrl+right
    c: "println!();"
  numpad:
    m: kp0
    j: kp1
    k: kp2
    l: kp3

tap_hold:
  space:
    tap: space
    layer: nav
    timeout_ms: 180
  capslock:
    tap: esc
    hold: super
  - key: rightalt
    tap: rightalt
    layer: numpad

combos:
  j k: esc
)";

    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));

    // Verify layers
    assert(config.layers.size() == 2);
    assert(config.layers[0].name == "nav");
    assert(config.layers[0].mappings.count(Keys::KEY_H) == 1);
    assert(config.layers[0].mappings[Keys::KEY_H].out_keys.size() == 1);
    assert(config.layers[0].mappings[Keys::KEY_H].out_keys[0] == Keys::KEY_LEFT);

    // Verify chord mapping
    assert(config.layers[0].mappings.count(Keys::KEY_W) == 1);
    assert(config.layers[0].mappings[Keys::KEY_W].out_keys.size() == 2);
    assert(config.layers[0].mappings[Keys::KEY_W].out_keys[0] == Keys::KEY_LEFTCTRL);
    assert(config.layers[0].mappings[Keys::KEY_W].out_keys[1] == Keys::KEY_RIGHT);

    // Verify text snippet mapping
    assert(config.layers[0].mappings.count(Keys::KEY_C) == 1);
    assert(config.layers[0].mappings[Keys::KEY_C].text == "println!();");

    // Verify numpad layer
    assert(config.layers[1].name == "numpad");
    assert(config.layers[1].mappings.count(Keys::KEY_M) == 1);
    assert(config.layers[1].mappings[Keys::KEY_M].out_keys[0] == Keys::KEY_KP0);

    // Verify tap_hold keys
    assert(config.tap_hold_keys.size() == 3);

    // Space -> layer nav
    assert(config.tap_hold_keys[0].key == Keys::KEY_SPACE);
    assert(config.tap_hold_keys[0].tap_key == Keys::KEY_SPACE);
    assert(config.tap_hold_keys[0].hold_layer == "nav");
    assert(config.tap_hold_keys[0].timeout_us == 180000LL);

    // CapsLock -> super
    assert(config.tap_hold_keys[1].key == Keys::KEY_CAPSLOCK);
    assert(config.tap_hold_keys[1].tap_key == Keys::KEY_ESC);
    assert(config.tap_hold_keys[1].hold_key == Keys::KEY_LEFTMETA);

    // RightAlt -> layer numpad
    assert(config.tap_hold_keys[2].key == Keys::KEY_RIGHTALT);
    assert(config.tap_hold_keys[2].tap_key == Keys::KEY_RIGHTALT);
    assert(config.tap_hold_keys[2].hold_layer == "numpad");

    // Verify combos
    assert(config.combos.size() == 1);

    std::cout << "test_yaml_parser_layers_and_tap_hold: PASSED\n";
}

static void test_yaml_parser_validation_errors() {
    Config config;
    std::string err;

    // 1. Unknown layer referenced in tap_hold
    std::string bad_layer_ref = R"(
layers:
  nav:
    h: left
tap_hold:
  space:
    tap: space
    layer: unknown_layer
)";
    assert(!loadYamlConfig(bad_layer_ref, config, err));
    assert(err.find("unknown layer 'unknown_layer'") != std::string::npos);

    // 2. Duplicate layer name
    std::string dup_layer = R"(
layers:
  nav:
    h: left
  nav:
    j: down
)";
    assert(!loadYamlConfig(dup_layer, config, err));
    assert(err.find("duplicate layer definition") != std::string::npos);

    // 3. Duplicate key inside layer
    std::string dup_key = R"(
layers:
  nav:
    h: left
    h: right
)";
    assert(!loadYamlConfig(dup_key, config, err));
    assert(err.find("duplicate mapping for key 'h'") != std::string::npos);

    // 4. Invalid key in layer mapping
    std::string bad_key = R"(
layers:
  nav:
    nonexistentkey: left
)";
    assert(!loadYamlConfig(bad_key, config, err));
    assert(err.find("unknown key") != std::string::npos);

    // 5. Empty mapping
    std::string empty_map = R"(
layers:
  nav:
    h:
)";
    assert(!loadYamlConfig(empty_map, config, err));
    assert(err.find("empty mapping") != std::string::npos);

    std::cout << "test_yaml_parser_validation_errors: PASSED\n";
}

static void test_toggle_layer_combo_and_locked_typing() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
combos:
  f + space: toggle_layer(numpad)

layers:
  numpad:
    m: "1"
    comma: "2"
    dot: "3"
    j: "4"
    k: "5"
    l: "6"
    u: "7"
    i: "8"
    o: "9"
    space: "0"
    esc: toggle_layer(numpad)
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    assert(!engine.isLayerActive("numpad"));

    // 1. Press f + space (combo to toggle numpad on)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_F, KEY_VAL_UP});

    // Numpad layer must now be locked ON!
    assert(engine.isLayerActive("numpad"));
    assert(writer.events.empty());  // Both keys swallowed cleanly on release

    // 2. Type while locked in numpad without holding any key down!
    // Press 'm' -> emits text "1"
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});

    auto k1 = writer.keyEvents();
    assert(k1.size() == 2);
    assert(k1[0].code == Keys::KEY_1 && k1[0].value == KEY_VAL_DOWN);
    assert(k1[1].code == Keys::KEY_1 && k1[1].value == KEY_VAL_UP);
    writer.clear();

    // Press 'j' -> emits text "4"
    engine.processEvent(Event{TimeVal{0, 130000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    auto k2 = writer.keyEvents();
    assert(k2.size() == 2);
    assert(k2[0].code == Keys::KEY_4 && k2[0].value == KEY_VAL_DOWN);
    assert(k2[1].code == Keys::KEY_4 && k2[1].value == KEY_VAL_UP);
    writer.clear();

    // Press 'space' -> emits text "0"
    engine.processEvent(Event{TimeVal{0, 160000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 180000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    auto k3 = writer.keyEvents();
    assert(k3.size() == 2);
    assert(k3[0].code == Keys::KEY_0 && k3[0].value == KEY_VAL_DOWN);
    assert(k3[1].code == Keys::KEY_0 && k3[1].value == KEY_VAL_UP);
    writer.clear();

    // 3. Exit numpad by pressing 'esc' (mapped to toggle_layer(numpad))
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_ESC, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 220000}, EV_KEY, Keys::KEY_ESC, KEY_VAL_UP});

    // Layer is toggled off!
    assert(!engine.isLayerActive("numpad"));
    // 'esc' was swallowed, not leaked to output!
    assert(writer.events.empty());

    // 4. Type 'm' after deactivation -> emits regular 'm'
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 460000});
    engine.processEvent(Event{TimeVal{0, 470000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});
    engine.onTimer(TimeVal{0, 630000});

    auto k4 = writer.keyEvents();
    bool saw_m_down = false;
    bool saw_m_up = false;
    for (const auto& ev : k4) {
        if (ev.code == Keys::KEY_M && ev.value == KEY_VAL_DOWN)
            saw_m_down = true;
        if (ev.code == Keys::KEY_M && ev.value == KEY_VAL_UP)
            saw_m_up = true;
        assert(ev.code != Keys::KEY_1);
    }
    assert(saw_m_down && saw_m_up);

    std::cout << "test_toggle_layer_combo_and_locked_typing: PASSED\n";
}

static void test_toggle_layer_key_release_safety() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer numpad;
    numpad.name = "numpad";
    numpad.mappings[Keys::KEY_K] = LayerAction({Keys::KEY_UP});      // 'k' maps to UP arrow
    numpad.mappings[Keys::KEY_ESC] = LayerAction({}, "", "numpad");  // 'esc' toggles numpad
    engine.setLayers({numpad});

    // Toggle on
    engine.toggleLayer("numpad");
    assert(engine.isLayerActive("numpad"));

    // Press and HOLD 'k' at t=10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_UP &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);
    writer.clear();

    // While 'k' is physically held down, toggle layer OFF via 'esc'
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_ESC, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_ESC, KEY_VAL_UP});
    assert(!engine.isLayerActive("numpad"));
    assert(writer.keyEvents().empty());  // 'esc' swallowed cleanly

    // Now release 'k' at t=60ms
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_K, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_UP && writer.keyEvents()[0].value == KEY_VAL_UP);

    std::cout << "test_toggle_layer_key_release_safety: PASSED\n";
}

static void test_toggle_layer_tap_hold() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
tap_hold:
  capslock:
    tap: toggle_layer(numpad)
    hold: super
    timeout_ms: 200

layers:
  numpad:
    j: "4"
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    // 1. Short tap on CapsLock (<200ms) -> toggles numpad ON
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    assert(engine.isLayerActive("numpad"));
    assert(writer.keyEvents().empty());  // No raw capslock output

    // 2. Type 'j' in numpad -> "4"
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_4);
    writer.clear();

    // 3. Short tap on CapsLock (<200ms) -> toggles numpad OFF
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});

    assert(!engine.isLayerActive("numpad"));
    assert(writer.keyEvents().empty());

    // 4. Long hold on CapsLock (>200ms) -> emits Super
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 510000});  // Hold timeout triggers
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFTMETA &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    engine.processEvent(Event{TimeVal{0, 550000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_LEFTMETA &&
           writer.keyEvents()[1].value == KEY_VAL_UP);

    std::cout << "test_toggle_layer_tap_hold: PASSED\n";
}

static void test_toggle_layer_leader_sequence() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
leader:
  key: capslock
  timeout_ms: 1000
  sequences:
    n p: toggle_layer(numpad)

layers:
  numpad:
    j: "4"
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    // Tap CapsLock (dedicated leader)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 20000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Press 'n', then 'p'
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_N, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_N, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_P, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 110000}, EV_KEY, Keys::KEY_P, KEY_VAL_UP});

    assert(!engine.isLeaderActive());
    assert(engine.isLayerActive("numpad"));
    assert(writer.keyEvents().empty());  // Sequence swallowed, no stray keys

    // Press 'j' -> emits "4"
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 220000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_4);

    std::cout << "test_toggle_layer_leader_sequence: PASSED\n";
}

static void test_toggle_layer_nested_with_momentary_layers() {
    MockWriter writer;
    TFFEngine engine(&writer);

    const std::string yaml = R"(
tap_hold:
  space:
    tap: space
    layer: nav
    timeout_ms: 200
  capslock:
    tap: toggle_layer(numpad)
    hold: super
    timeout_ms: 200

layers:
  nav:
    h: left
    j: down
  numpad:
    j: "4"
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    engine.setConfig(config);

    // Toggle on numpad via capslock tap
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLayerActive("numpad"));

    // Type 'j' in numpad -> "4"
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 70000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_4);
    writer.clear();

    // Now momentarily hold Space (nav layer) while numpad is locked on
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    // Fast chord 'j' -> nav layer has 'j: down', taking LIFO precedence over numpad!
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    assert(writer.keyEvents().size() == 1);
    assert(writer.keyEvents()[0].code == Keys::KEY_DOWN &&
           writer.keyEvents()[0].value == KEY_VAL_DOWN);

    engine.processEvent(Event{TimeVal{0, 140000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_DOWN &&
           writer.keyEvents()[1].value == KEY_VAL_UP);
    writer.clear();

    // Release Space -> nav layer deactivates, numpad remains active!
    engine.processEvent(Event{TimeVal{0, 160000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(engine.isLayerActive("numpad"));
    assert(!engine.isLayerActive("nav"));

    // Type 'j' again -> "4" from numpad
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 220000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[0].code == Keys::KEY_4);

    std::cout << "test_toggle_layer_nested_with_momentary_layers: PASSED\n";
}

static void test_toggle_layer_validation_errors() {
    Config config;
    std::string err;

    // 1. Unknown layer in combo toggle_layer
    const std::string bad_combo = R"(
combos:
  f + space: toggle_layer(nonexistent)
layers:
  numpad:
    j: "4"
)";
    assert(!loadYamlConfig(bad_combo, config, err));
    assert(err.find("unknown layer 'nonexistent' referenced in combo toggle_layer") !=
           std::string::npos);

    // 2. Unknown layer in layer mapping
    const std::string bad_map = R"(
layers:
  numpad:
    esc: toggle_layer(nonexistent)
)";
    assert(!loadYamlConfig(bad_map, config, err));
    assert(err.find("unknown layer 'nonexistent' referenced in layer 'numpad' toggle_layer") !=
           std::string::npos);

    // 3. Unknown layer in tap_hold
    const std::string bad_th = R"(
tap_hold:
  capslock:
    tap: toggle_layer(nonexistent)
    hold: super
    timeout_ms: 200
layers:
  numpad:
    j: "4"
)";
    assert(!loadYamlConfig(bad_th, config, err));
    assert(err.find("unknown layer 'nonexistent' referenced in tap_hold toggle_layer") !=
           std::string::npos);

    // 4. Unknown layer in leader
    const std::string bad_leader = R"(
leader:
  sequences:
    n p: toggle_layer(nonexistent)
layers:
  numpad:
    j: "4"
)";
    assert(!loadYamlConfig(bad_leader, config, err));
    assert(err.find("unknown layer 'nonexistent' referenced in leader toggle_layer") !=
           std::string::npos);

    // 5. Empty layer name in toggle_layer()
    const std::string empty_tl = R"(
combos:
  f + space: toggle_layer()
layers:
  numpad:
    j: "4"
)";
    assert(!loadYamlConfig(empty_tl, config, err));
    assert(err.find("empty layer name in toggle_layer()") != std::string::npos);

    std::cout << "test_toggle_layer_validation_errors: PASSED\n";
}

int main() {
    std::cout << "Running Modal Layers unit tests...\n";
    test_momentary_layer_activation();
    test_key_passthrough_in_active_layer();
    test_layer_deactivation_on_release();
    test_key_release_after_layer_deactivation();
    test_multi_key_chord_in_layer();
    test_text_snippet_in_layer();
    test_nested_layers();
    test_tap_vs_hold_for_layer_key();
    test_timeout_hold_activation();
    test_layer_stack_management_api();
    test_yaml_parser_layers_and_tap_hold();
    test_yaml_parser_validation_errors();
    test_toggle_layer_combo_and_locked_typing();
    test_toggle_layer_key_release_safety();
    test_toggle_layer_tap_hold();
    test_toggle_layer_leader_sequence();
    test_toggle_layer_nested_with_momentary_layers();
    test_toggle_layer_validation_errors();
    std::cout << "All 18 Modal Layers unit tests passed successfully!\n";
    return 0;
}
