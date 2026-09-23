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

static void test_standalone_osm_shift() {
    MockWriter writer;
    TFFEngine engine(&writer);

    OneShotKey osk;
    osk.key = Keys::KEY_LEFTSHIFT;
    osk.modifier = Keys::KEY_LEFTSHIFT;
    osk.timeout_us = 1500000LL;
    engine.setOneShotKeys({osk});

    // 1. Tap LeftShift (down at 10ms, up at 50ms)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    // LeftShift is not emitted yet; it is armed as one-shot modifier
    assert(writer.keyEvents().empty());
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));

    // 2. Press Key A down at 100ms
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 2);
    // LeftShift down emitted first, then Key A down
    assert(keys1[0].code == Keys::KEY_LEFTSHIFT && keys1[0].value == KEY_VAL_DOWN);
    assert(keys1[1].code == Keys::KEY_A && keys1[1].value == KEY_VAL_DOWN);

    // 3. Release Key A up at 150ms
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    auto keys2 = writer.keyEvents();
    assert(keys2.size() == 4);
    // Key A up emitted first, then LeftShift up disengaged
    assert(keys2[2].code == Keys::KEY_A && keys2[2].value == KEY_VAL_UP);
    assert(keys2[3].code == Keys::KEY_LEFTSHIFT && keys2[3].value == KEY_VAL_UP);
    assert(!engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));

    // 4. Press Key B down and up -> normal, unshifted
    writer.clear();
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP});
    auto keys3 = writer.keyEvents();
    assert(keys3.size() == 2);
    assert(keys3[0].code == Keys::KEY_B && keys3[0].value == KEY_VAL_DOWN);
    assert(keys3[1].code == Keys::KEY_B && keys3[1].value == KEY_VAL_UP);

    std::cout << "test_standalone_osm_shift passed\n";
}

static void test_chained_osm() {
    MockWriter writer;
    TFFEngine engine(&writer);

    OneShotKey osk_ctrl;
    osk_ctrl.key = Keys::KEY_LEFTCTRL;
    osk_ctrl.modifier = Keys::KEY_LEFTCTRL;
    osk_ctrl.timeout_us = 1500000LL;

    OneShotKey osk_shift;
    osk_shift.key = Keys::KEY_LEFTSHIFT;
    osk_shift.modifier = Keys::KEY_LEFTSHIFT;
    osk_shift.timeout_us = 1500000LL;

    engine.setOneShotKeys({osk_ctrl, osk_shift});

    // Tap LeftCtrl
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_UP});
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTCTRL));

    // Tap LeftShift
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTCTRL));

    // Press Key T down -> emits LeftCtrl DOWN, LeftShift DOWN, Key T DOWN
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_T, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 3);
    assert(keys1[0].code == Keys::KEY_LEFTCTRL && keys1[0].value == KEY_VAL_DOWN);
    assert(keys1[1].code == Keys::KEY_LEFTSHIFT && keys1[1].value == KEY_VAL_DOWN);
    assert(keys1[2].code == Keys::KEY_T && keys1[2].value == KEY_VAL_DOWN);

    // Release Key T up -> emits Key T UP, LeftCtrl UP, LeftShift UP
    engine.processEvent(Event{TimeVal{0, 160000}, EV_KEY, Keys::KEY_T, KEY_VAL_UP});
    auto keys2 = writer.keyEvents();
    assert(keys2.size() == 6);
    assert(keys2[3].code == Keys::KEY_T && keys2[3].value == KEY_VAL_UP);
    assert(keys2[4].code == Keys::KEY_LEFTSHIFT && keys2[4].value == KEY_VAL_UP);
    assert(keys2[5].code == Keys::KEY_LEFTCTRL && keys2[5].value == KEY_VAL_UP);

    assert(!engine.isOneShotModifierArmed(Keys::KEY_LEFTCTRL));
    assert(!engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));

    std::cout << "test_chained_osm passed\n";
}

static void test_osm_timeout_expiration() {
    MockWriter writer;
    TFFEngine engine(&writer);

    OneShotKey osk;
    osk.key = Keys::KEY_LEFTSHIFT;
    osk.modifier = Keys::KEY_LEFTSHIFT;
    osk.timeout_us = 1500000LL;  // 1.5s
    engine.setOneShotKeys({osk});

    // Tap LeftShift at t=100ms
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 140000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));
    assert(engine.hasActiveTimer());

    // Timeout occurs at 100ms + 1500ms = 1600ms. Fire timer at 1650ms.
    engine.onTimer(TimeVal{1, 650000});
    assert(!engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));
    assert(writer.keyEvents().empty());

    // Press Key A at t=2000ms -> regular A, no shift
    engine.processEvent(Event{TimeVal{2, 0}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{2, 50000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    auto keys = writer.keyEvents();
    assert(keys.size() == 2);
    assert(keys[0].code == Keys::KEY_A && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::KEY_A && keys[1].value == KEY_VAL_UP);

    std::cout << "test_osm_timeout_expiration passed\n";
}

static void test_osm_held_normal_modifier() {
    MockWriter writer;
    TFFEngine engine(&writer);

    OneShotKey osk;
    osk.key = Keys::KEY_LEFTSHIFT;
    osk.modifier = Keys::KEY_LEFTSHIFT;
    osk.timeout_us = 1500000LL;
    engine.setOneShotKeys({osk});

    // Press and hold LeftShift down at t=10ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());

    // Fast chord: press Key A down at t=30ms while LeftShift is still held
    // LeftShift promotes to hold immediately and emits LeftShift DOWN, then Key A DOWN
    engine.processEvent(Event{TimeVal{0, 30000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 2);
    assert(keys1[0].code == Keys::KEY_LEFTSHIFT && keys1[0].value == KEY_VAL_DOWN);
    assert(keys1[1].code == Keys::KEY_A && keys1[1].value == KEY_VAL_DOWN);

    // Release Key A up
    engine.processEvent(Event{TimeVal{0, 60000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    auto keys2 = writer.keyEvents();
    assert(keys2.size() == 3);
    assert(keys2[2].code == Keys::KEY_A && keys2[2].value == KEY_VAL_UP);

    // Release LeftShift up
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    auto keys3 = writer.keyEvents();
    assert(keys3.size() == 4);
    assert(keys3[3].code == Keys::KEY_LEFTSHIFT && keys3[3].value == KEY_VAL_UP);

    std::cout << "test_osm_held_normal_modifier passed\n";
}

static void test_standalone_osl_nav() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    nav.mappings[Keys::KEY_J] = LayerAction({Keys::KEY_DOWN});
    nav.mappings[Keys::KEY_K] = LayerAction({Keys::KEY_UP});
    nav.mappings[Keys::KEY_L] = LayerAction({Keys::KEY_RIGHT});
    engine.setLayers({nav});

    OneShotKey osk;
    osk.key = Keys::KEY_SPACE;
    osk.layer = "nav";
    osk.timeout_us = 1500000LL;
    engine.setOneShotKeys({osk});

    // Tap Space at t=10ms, up at 40ms
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(writer.keyEvents().empty());
    assert(engine.isOneShotLayerArmed("nav"));

    // Press Key K down at t=80ms -> mapped in "nav" to KEY_UP!
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 1);
    assert(keys1[0].code == Keys::KEY_UP && keys1[0].value == KEY_VAL_DOWN);

    // Release Key K up at t=120ms -> KEY_UP released and "nav" layer disengages
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_K, KEY_VAL_UP});
    auto keys2 = writer.keyEvents();
    assert(keys2.size() == 2);
    assert(keys2[1].code == Keys::KEY_UP && keys2[1].value == KEY_VAL_UP);
    assert(!engine.isOneShotLayerArmed("nav"));
    assert(!engine.isLayerActive("nav"));

    // Subsequent press of Key K -> normal Key K
    writer.clear();
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 180000}, EV_KEY, Keys::KEY_K, KEY_VAL_UP});
    auto keys3 = writer.keyEvents();
    assert(keys3.size() == 2);
    assert(keys3[0].code == Keys::KEY_K && keys3[0].value == KEY_VAL_DOWN);
    assert(keys3[1].code == Keys::KEY_K && keys3[1].value == KEY_VAL_UP);

    std::cout << "test_standalone_osl_nav passed\n";
}

static void test_osl_timeout_expiration() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_K] = LayerAction({Keys::KEY_UP});
    engine.setLayers({nav});

    OneShotKey osk;
    osk.key = Keys::KEY_SPACE;
    osk.layer = "nav";
    osk.timeout_us = 1500000LL;
    engine.setOneShotKeys({osk});

    // Tap Space at t=100ms
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 140000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(engine.isOneShotLayerArmed("nav"));

    // Fire timer at 1650ms
    engine.onTimer(TimeVal{1, 650000});
    assert(!engine.isOneShotLayerArmed("nav"));
    assert(!engine.isLayerActive("nav"));

    // Press Key K -> regular K, not UP
    engine.processEvent(Event{TimeVal{2, 0}, EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{2, 50000}, EV_KEY, Keys::KEY_K, KEY_VAL_UP});
    auto keys = writer.keyEvents();
    assert(keys.size() == 2);
    assert(keys[0].code == Keys::KEY_K && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::KEY_K && keys[1].value == KEY_VAL_UP);

    std::cout << "test_osl_timeout_expiration passed\n";
}

static void test_tap_hold_with_one_shot_actions() {
    MockWriter writer;
    TFFEngine engine(&writer);

    // Tap-hold CapsLock: tap = osm(shift), hold = super, timeout = 200ms
    TapHoldKey thk;
    thk.key = Keys::KEY_CAPSLOCK;
    thk.hold_key = Keys::KEY_LEFTMETA;
    thk.tap_one_shot_modifier = Keys::KEY_LEFTSHIFT;
    thk.tap_one_shot_timeout_us = 1500000LL;
    thk.timeout_us = 200000LL;
    engine.setTapHoldKeys({thk});

    // 1. Quick tap of CapsLock (down at 10ms, up at 50ms < 200ms)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().empty());
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));

    // Press Key B -> emits LeftShift DOWN, Key B DOWN
    engine.processEvent(Event{TimeVal{0, 90000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 2);
    assert(keys1[0].code == Keys::KEY_LEFTSHIFT && keys1[0].value == KEY_VAL_DOWN);
    assert(keys1[1].code == Keys::KEY_B && keys1[1].value == KEY_VAL_DOWN);

    // Release Key B -> Key B UP, LeftShift UP
    engine.processEvent(Event{TimeVal{0, 120000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP});
    auto keys2 = writer.keyEvents();
    assert(keys2.size() == 4);
    assert(keys2[2].code == Keys::KEY_B && keys2[2].value == KEY_VAL_UP);
    assert(keys2[3].code == Keys::KEY_LEFTSHIFT && keys2[3].value == KEY_VAL_UP);

    // 2. Sustained hold of CapsLock (> 200ms)
    writer.clear();
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    // Fire timer at 200ms + 200ms = 400ms
    engine.onTimer(TimeVal{0, 410000});
    auto keys3 = writer.keyEvents();
    assert(keys3.size() == 1);
    assert(keys3[0].code == Keys::KEY_LEFTMETA && keys3[0].value == KEY_VAL_DOWN);

    engine.processEvent(Event{TimeVal{0, 500000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    auto keys4 = writer.keyEvents();
    assert(keys4.size() == 2);
    assert(keys4[1].code == Keys::KEY_LEFTMETA && keys4[1].value == KEY_VAL_UP);

    std::cout << "test_tap_hold_with_one_shot_actions passed\n";
}

static void test_yaml_parser_one_shot() {
    std::string yaml_dict =
        "one_shot:\n"
        "  leftshift: 1200\n"
        "  space: [nav, 2000]\n"
        "layers:\n"
        "  nav:\n"
        "    h: left\n";

    Config cfg;
    std::string err;
    bool ok = loadYamlConfig(yaml_dict, cfg, err);
    assert(ok);
    assert(cfg.one_shot_keys.size() == 2);
    assert(cfg.one_shot_keys[0].key == Keys::KEY_LEFTSHIFT);
    assert(cfg.one_shot_keys[0].modifier == Keys::KEY_LEFTSHIFT);
    assert(cfg.one_shot_keys[0].timeout_us == 1200000LL);
    assert(cfg.one_shot_keys[1].key == Keys::KEY_SPACE);
    assert(cfg.one_shot_keys[1].layer == "nav");
    assert(cfg.one_shot_keys[1].timeout_us == 2000000LL);

    std::string yaml_list =
        "one_shot:\n"
        "  - key: capslock\n"
        "    modifier: shift\n"
        "    timeout_ms: 1000\n";

    Config cfg2;
    ok = loadYamlConfig(yaml_list, cfg2, err);
    assert(ok);
    assert(cfg2.one_shot_keys.size() == 1);
    assert(cfg2.one_shot_keys[0].key == Keys::KEY_CAPSLOCK);
    assert(cfg2.one_shot_keys[0].modifier == Keys::KEY_LEFTSHIFT);
    assert(cfg2.one_shot_keys[0].timeout_us == 1000000LL);

    // Tap-Hold with inline osm(...) and osl(...)
    std::string yaml_tap_hold =
        "layers:\n"
        "  nav:\n"
        "    k: up\n"
        "tap_hold:\n"
        "  capslock:\n"
        "    tap: osm(shift)\n"
        "    hold: super\n"
        "    timeout_ms: 200\n"
        "  space:\n"
        "    tap: osl(nav)\n"
        "    hold: alt\n"
        "    timeout_ms: 250\n";

    Config cfg3;
    ok = loadYamlConfig(yaml_tap_hold, cfg3, err);
    assert(ok);
    assert(cfg3.tap_hold_keys.size() == 2);
    assert(cfg3.tap_hold_keys[0].key == Keys::KEY_CAPSLOCK);
    assert(cfg3.tap_hold_keys[0].tap_one_shot_modifier == Keys::KEY_LEFTSHIFT);
    assert(cfg3.tap_hold_keys[0].hold_key == Keys::KEY_LEFTMETA);
    assert(cfg3.tap_hold_keys[1].key == Keys::KEY_SPACE);
    assert(cfg3.tap_hold_keys[1].tap_one_shot_layer == "nav");
    assert(cfg3.tap_hold_keys[1].hold_key == Keys::KEY_LEFTALT);

    // Validation error: invalid layer in osl
    std::string yaml_bad_layer =
        "one_shot:\n"
        "  space: [unknown_layer, 1500]\n";
    Config cfg_bad;
    ok = loadYamlConfig(yaml_bad_layer, cfg_bad, err);
    assert(!ok);
    assert(err.find("unknown layer 'unknown_layer' referenced in one_shot") != std::string::npos);

    std::cout << "test_yaml_parser_one_shot passed\n";
}

static void test_rollover_safety() {
    MockWriter writer;
    TFFEngine engine(&writer);

    OneShotKey osk;
    osk.key = Keys::KEY_LEFTSHIFT;
    osk.modifier = Keys::KEY_LEFTSHIFT;
    osk.timeout_us = 1500000LL;
    engine.setOneShotKeys({osk});

    // Tap LeftShift
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    assert(engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));

    // Rollover typing:
    // Key A down at 100ms (Shift down, A down)
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    // Key B down at 110ms before Key A is released
    engine.processEvent(Event{TimeVal{0, 110000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});
    // Key A up at 140ms -> Key A up, Shift UP!
    engine.processEvent(Event{TimeVal{0, 140000}, EV_KEY, Keys::KEY_A, KEY_VAL_UP});
    // Key B up at 160ms
    engine.processEvent(Event{TimeVal{0, 160000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP});

    auto keys = writer.keyEvents();
    assert(keys.size() == 6);
    // 0: Shift DOWN
    assert(keys[0].code == Keys::KEY_LEFTSHIFT && keys[0].value == KEY_VAL_DOWN);
    // 1: A DOWN
    assert(keys[1].code == Keys::KEY_A && keys[1].value == KEY_VAL_DOWN);
    // 2: B DOWN
    assert(keys[2].code == Keys::KEY_B && keys[2].value == KEY_VAL_DOWN);
    // 3: A UP
    assert(keys[3].code == Keys::KEY_A && keys[3].value == KEY_VAL_UP);
    // 4: Shift UP (disengaged upon release of trigger key A!)
    assert(keys[4].code == Keys::KEY_LEFTSHIFT && keys[4].value == KEY_VAL_UP);
    // 5: B UP
    assert(keys[5].code == Keys::KEY_B && keys[5].value == KEY_VAL_UP);

    std::cout << "test_rollover_safety passed\n";
}

static void test_modifier_key_with_one_shot_layer() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_K] = LayerAction({Keys::KEY_UP});
    engine.setLayers({nav});

    // LeftShift is physically a modifier key, but configured as One-Shot Layer "nav"!
    OneShotKey osk;
    osk.key = Keys::KEY_LEFTSHIFT;
    osk.layer = "nav";
    osk.modifier = 0;
    osk.timeout_us = 1500000LL;
    engine.setOneShotKeys({osk});

    // Tap LeftShift
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 40000}, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP});
    assert(engine.isOneShotLayerArmed("nav"));
    assert(!engine.isOneShotModifierArmed(Keys::KEY_LEFTSHIFT));

    // Press Key K -> emits KEY_UP, not Shift + K
    engine.processEvent(Event{TimeVal{0, 80000}, EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    auto keys1 = writer.keyEvents();
    assert(keys1.size() == 1);
    assert(keys1[0].code == Keys::KEY_UP && keys1[0].value == KEY_VAL_DOWN);

    std::cout << "test_modifier_key_with_one_shot_layer passed\n";
}

int main() {
    test_standalone_osm_shift();
    test_chained_osm();
    test_osm_timeout_expiration();
    test_osm_held_normal_modifier();
    test_standalone_osl_nav();
    test_osl_timeout_expiration();
    test_tap_hold_with_one_shot_actions();
    test_yaml_parser_one_shot();
    test_rollover_safety();
    test_modifier_key_with_one_shot_layer();

    std::cout << "All One-Shot tests passed successfully!\n";
    return 0;
}
