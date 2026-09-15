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
        if (ev.code == Keys::KEY_A && ev.value == KEY_VAL_DOWN) saw_a_down = true;
        if (ev.code == Keys::KEY_A && ev.value == KEY_VAL_UP) saw_a_up = true;
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
        if (ev.code == Keys::KEY_H && ev.value == KEY_VAL_DOWN) saw_h_down = true;
        if (ev.code == Keys::KEY_H && ev.value == KEY_VAL_UP) saw_h_up = true;
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
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFT && writer.keyEvents()[0].value == KEY_VAL_DOWN);

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
    assert(writer.keyEvents()[0].code == Keys::KEY_LEFT && writer.keyEvents()[0].value == KEY_VAL_DOWN);

    // Release H -> Left Up
    engine.processEvent(Event{TimeVal{0, 240000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 2);
    assert(writer.keyEvents()[1].code == Keys::KEY_LEFT && writer.keyEvents()[1].value == KEY_VAL_UP);

    // Release Space -> deactivates nav
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(engine.getActiveLayers().empty());

    std::cout << "test_timeout_hold_activation: PASSED\n";
}

static void test_layer_stack_management_api() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav; nav.name = "nav";
    Layer numpad; numpad.name = "numpad";
    Layer fn; fn.name = "fn";
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
  capslock: [esc, super]
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
    std::cout << "All 12 Modal Layers unit tests passed successfully!\n";
    return 0;
}
