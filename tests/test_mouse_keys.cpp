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

    std::vector<Event> relEvents() const {
        std::vector<Event> result;
        for (const auto& ev : events) {
            if (ev.type == EV_REL) {
                result.push_back(ev);
            }
        }
        return result;
    }
};

} // anonymous namespace

static void test_mouse_action_parsing() {
    MouseAction act;
    std::string err;

    // Standard directions
    assert(parseMouseAction("mouse_left", act, err));
    assert(act.type == MouseActionType::MoveLeft && act.delta == 0);
    assert(parseMouseAction("mouse_right", act, err));
    assert(act.type == MouseActionType::MoveRight && act.delta == 0);
    assert(parseMouseAction("mouse_up", act, err));
    assert(act.type == MouseActionType::MoveUp && act.delta == 0);
    assert(parseMouseAction("mouse_down", act, err));
    assert(act.type == MouseActionType::MoveDown && act.delta == 0);

    // Aliases
    assert(parseMouseAction("cursor_left", act, err));
    assert(act.type == MouseActionType::MoveLeft);
    assert(parseMouseAction("ms_right", act, err));
    assert(act.type == MouseActionType::MoveRight);
    assert(parseMouseAction("mouse_move_up", act, err));
    assert(act.type == MouseActionType::MoveUp);

    // Movements with custom deltas
    assert(parseMouseAction("mouse_left(25)", act, err));
    assert(act.type == MouseActionType::MoveLeft && act.delta == 25);
    assert(parseMouseAction("cursor_up(50)", act, err));
    assert(act.type == MouseActionType::MoveUp && act.delta == 50);

    // Wheel actions
    assert(parseMouseAction("wheel_up", act, err));
    assert(act.type == MouseActionType::WheelUp && act.delta == 0);
    assert(parseMouseAction("mouse_wheel_down", act, err));
    assert(act.type == MouseActionType::WheelDown && act.delta == 0);
    assert(parseMouseAction("scroll_left", act, err));
    assert(act.type == MouseActionType::WheelLeft && act.delta == 0);
    assert(parseMouseAction("scroll_right", act, err));
    assert(act.type == MouseActionType::WheelRight && act.delta == 0);
    assert(parseMouseAction("wheel_up(3)", act, err));
    assert(act.type == MouseActionType::WheelUp && act.delta == 3);

    // Mouse buttons
    assert(parseMouseAction("mouse_btn_left", act, err));
    assert(act.type == MouseActionType::BtnLeft && act.isButton());
    assert(parseMouseAction("btn_right", act, err));
    assert(act.type == MouseActionType::BtnRight && act.isButton());
    assert(parseMouseAction("mouse_middle_click", act, err));
    assert(act.type == MouseActionType::BtnMiddle && act.isButton());
    assert(parseMouseAction("btn_side", act, err));
    assert(act.type == MouseActionType::BtnSide && act.isButton());
    assert(parseMouseAction("mouse_btn_extra", act, err));
    assert(act.type == MouseActionType::BtnExtra && act.isButton());

    // Rejection of invalid deltas
    assert(!parseMouseAction("mouse_left(0)", act, err));
    assert(!parseMouseAction("mouse_left(-5)", act, err));
    assert(!parseMouseAction("mouse_left(1001)", act, err));
    assert(!parseMouseAction("mouse_left(abc)", act, err));

    // Rejection of uppercase
    assert(!parseMouseAction("Mouse_Left", act, err));
    assert(err.find("only lower case") != std::string::npos);

    // Non-mouse words should return false without error
    err.clear();
    assert(!parseMouseAction("enter", act, err));
    assert(err.empty());
    assert(!parseMouseAction("\"hello world\"", act, err));
    assert(err.empty());

    // mouseActionToWord
    assert(mouseActionToWord(MouseAction{MouseActionType::MoveLeft, 0}) == "mouse_left");
    assert(mouseActionToWord(MouseAction{MouseActionType::MoveLeft, 20}) == "mouse_left(20)");
    assert(mouseActionToWord(MouseAction{MouseActionType::WheelUp, 0}) == "mouse_wheel_up");
    assert(mouseActionToWord(MouseAction{MouseActionType::WheelUp, 4}) == "mouse_wheel_up(4)");
    assert(mouseActionToWord(MouseAction{MouseActionType::BtnLeft, 0}) == "mouse_btn_left");

    std::cout << "test_mouse_action_parsing: PASSED\n";
}

static void test_mouse_movement_emission() {
    MockWriter writer;
    TFFEngine engine(&writer);

    MouseConfig mc;
    mc.move_speed = 12;
    engine.setMouseConfig(mc);

    // 1. Emit default move left: REL_X -12
    engine.emitMouseAction(MouseAction{MouseActionType::MoveLeft, 0}, TimeVal{1, 0});
    auto rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_X);
    assert(rels[0].value == -12);
    assert(writer.events.size() == 2 && writer.events[1].type == EV_SYN);

    writer.clear();

    // 2. Emit move right with custom delta: REL_X +25
    engine.emitMouseAction(MouseAction{MouseActionType::MoveRight, 25}, TimeVal{1, 50});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_X);
    assert(rels[0].value == 25);

    writer.clear();

    // 3. Emit move up: REL_Y -12
    engine.emitMouseAction(MouseAction{MouseActionType::MoveUp, 0}, TimeVal{1, 100});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_Y);
    assert(rels[0].value == -12);

    writer.clear();

    // 4. Emit move down: REL_Y +12
    engine.emitMouseAction(MouseAction{MouseActionType::MoveDown, 0}, TimeVal{1, 150});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_Y);
    assert(rels[0].value == 12);

    std::cout << "test_mouse_movement_emission: PASSED\n";
}

static void test_mouse_wheel_emission() {
    MockWriter writer;
    TFFEngine engine(&writer);

    MouseConfig mc;
    mc.wheel_step = 2;
    engine.setMouseConfig(mc);

    // 1. Wheel up: REL_WHEEL +2
    engine.emitMouseAction(MouseAction{MouseActionType::WheelUp, 0}, TimeVal{1, 0});
    auto rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_WHEEL);
    assert(rels[0].value == 2);

    writer.clear();

    // 2. Wheel down: REL_WHEEL -2
    engine.emitMouseAction(MouseAction{MouseActionType::WheelDown, 0}, TimeVal{1, 50});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_WHEEL);
    assert(rels[0].value == -2);

    writer.clear();

    // 3. Horizontal wheel left: REL_HWHEEL -2
    engine.emitMouseAction(MouseAction{MouseActionType::WheelLeft, 0}, TimeVal{1, 100});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_HWHEEL);
    assert(rels[0].value == -2);

    writer.clear();

    // 4. Horizontal wheel right: REL_HWHEEL +2
    engine.emitMouseAction(MouseAction{MouseActionType::WheelRight, 0}, TimeVal{1, 150});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_HWHEEL);
    assert(rels[0].value == 2);

    writer.clear();

    // 5. Wheel with custom delta override: delta=5
    engine.emitMouseAction(MouseAction{MouseActionType::WheelDown, 5}, TimeVal{1, 200});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_WHEEL);
    assert(rels[0].value == -5);

    std::cout << "test_mouse_wheel_emission: PASSED\n";
}

static void test_mouse_button_press_and_release() {
    MockWriter writer;
    TFFEngine engine(&writer);

    // Layer mapping key F to BTN_LEFT
    Layer mouse_layer;
    mouse_layer.name = "mouse";
    LayerAction act;
    act.out_keys = {Keys::BTN_LEFT};
    mouse_layer.mappings[Keys::KEY_F] = act;
    engine.setLayers({mouse_layer});
    engine.activateLayer("mouse");

    // Key F pressed DOWN -> emits BTN_LEFT DOWN
    engine.processEvent(Event{TimeVal{1, 0}, EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    auto keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::BTN_LEFT);
    assert(keys[0].value == KEY_VAL_DOWN);

    writer.clear();

    // Key F released UP -> emits BTN_LEFT UP
    engine.processEvent(Event{TimeVal{1, 100000}, EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::BTN_LEFT);
    assert(keys[0].value == KEY_VAL_UP);

    std::cout << "test_mouse_button_press_and_release: PASSED\n";
}

static void test_modal_layer_home_row_mouse_keys() {
    MockWriter writer;
    TFFEngine engine(&writer);

    // Tap-Hold: Space -> tap: space, hold: layer "mouse"
    TapHoldKey th;
    th.key = Keys::KEY_SPACE;
    th.tap_key = Keys::KEY_SPACE;
    th.hold_layer = "mouse";
    th.timeout_us = 200000LL;
    engine.setTapHoldKeys({th});

    // Mouse layer with home-row keys:
    // h -> mouse_left, j -> mouse_down, k -> mouse_up, l -> mouse_right
    // u -> wheel_up, d -> wheel_down
    // f -> btn_left, s -> btn_right
    Layer mouse_layer;
    mouse_layer.name = "mouse";
    {
        LayerAction a;
        a.mouse = MouseAction{MouseActionType::MoveLeft, 0};
        mouse_layer.mappings[Keys::KEY_H] = a;
    }
    {
        LayerAction a;
        a.mouse = MouseAction{MouseActionType::MoveDown, 0};
        mouse_layer.mappings[Keys::KEY_J] = a;
    }
    {
        LayerAction a;
        a.mouse = MouseAction{MouseActionType::MoveUp, 0};
        mouse_layer.mappings[Keys::KEY_K] = a;
    }
    {
        LayerAction a;
        a.mouse = MouseAction{MouseActionType::MoveRight, 0};
        mouse_layer.mappings[Keys::KEY_L] = a;
    }
    {
        LayerAction a;
        a.mouse = MouseAction{MouseActionType::WheelUp, 0};
        mouse_layer.mappings[Keys::KEY_U] = a;
    }
    {
        LayerAction a;
        a.mouse = MouseAction{MouseActionType::WheelDown, 0};
        mouse_layer.mappings[Keys::KEY_D] = a;
    }
    {
        LayerAction a;
        a.out_keys = {Keys::BTN_LEFT};
        mouse_layer.mappings[Keys::KEY_F] = a;
    }
    {
        LayerAction a;
        a.out_keys = {Keys::BTN_RIGHT};
        mouse_layer.mappings[Keys::KEY_S] = a;
    }
    engine.setLayers({mouse_layer});

    MouseConfig mc;
    mc.move_speed = 10;
    mc.wheel_step = 1;
    engine.setMouseConfig(mc);

    // 1. Hold Space to activate mouse layer (fast chord with H)
    engine.processEvent(Event{TimeVal{1, 0}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    assert(engine.getActiveLayers().empty()); // buffered

    // Press H -> permissive hold activates mouse layer, H emits MoveLeft
    engine.processEvent(Event{TimeVal{1, 50000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    assert(!engine.getActiveLayers().empty());
    assert(engine.getActiveLayers().back() == "mouse");

    auto rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_X);
    assert(rels[0].value == -10);

    writer.clear();

    // Release H -> clean swallow, no release event emitted for relative mouse move
    engine.processEvent(Event{TimeVal{1, 80000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    assert(writer.events.empty());

    // 2. Press J -> MoveDown (+10 on REL_Y)
    engine.processEvent(Event{TimeVal{1, 90000}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_Y);
    assert(rels[0].value == 10);
    writer.clear();
    engine.processEvent(Event{TimeVal{1, 100000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    assert(writer.events.empty());

    // 3. Wheel up on U
    engine.processEvent(Event{TimeVal{1, 110000}, EV_KEY, Keys::KEY_U, KEY_VAL_DOWN});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_WHEEL);
    assert(rels[0].value == 1);
    writer.clear();
    engine.processEvent(Event{TimeVal{1, 120000}, EV_KEY, Keys::KEY_U, KEY_VAL_UP});
    assert(writer.events.empty());

    // 4. Left click on F
    engine.processEvent(Event{TimeVal{1, 130000}, EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    auto keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::BTN_LEFT);
    assert(keys[0].value == KEY_VAL_DOWN);
    writer.clear();

    engine.processEvent(Event{TimeVal{1, 150000}, EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::BTN_LEFT);
    assert(keys[0].value == KEY_VAL_UP);
    writer.clear();

    // 5. Release Space -> layer deactivated
    engine.processEvent(Event{TimeVal{1, 200000}, EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    assert(engine.getActiveLayers().empty());
    assert(writer.events.empty());

    // 6. Normal typing outside mouse layer: H emits standard KEY_H
    engine.processEvent(Event{TimeVal{1, 250000}, EV_KEY, Keys::KEY_H, KEY_VAL_DOWN});
    keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::KEY_H);
    assert(keys[0].value == KEY_VAL_DOWN);
    writer.clear();
    engine.processEvent(Event{TimeVal{1, 270000}, EV_KEY, Keys::KEY_H, KEY_VAL_UP});
    keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::KEY_H);
    assert(keys[0].value == KEY_VAL_UP);

    std::cout << "test_modal_layer_home_row_mouse_keys: PASSED\n";
}

static void test_key_repeat_continuous_movement() {
    MockWriter writer;
    TFFEngine engine(&writer);

    Layer mouse_layer;
    mouse_layer.name = "mouse";
    LayerAction act;
    act.mouse = MouseAction{MouseActionType::MoveRight, 15};
    mouse_layer.mappings[Keys::KEY_L] = act;
    engine.setLayers({mouse_layer});
    engine.activateLayer("mouse");

    // Key L DOWN: first step
    engine.processEvent(Event{TimeVal{1, 0}, EV_KEY, Keys::KEY_L, KEY_VAL_DOWN});
    auto rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_X && rels[0].value == 15);
    writer.clear();

    // Key L REPEAT: subsequent steps
    engine.processEvent(Event{TimeVal{1, 20000}, EV_KEY, Keys::KEY_L, KEY_VAL_REPEAT});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_X && rels[0].value == 15);
    writer.clear();

    engine.processEvent(Event{TimeVal{1, 40000}, EV_KEY, Keys::KEY_L, KEY_VAL_REPEAT});
    rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_X && rels[0].value == 15);
    writer.clear();

    // Key L UP: release cleanly
    engine.processEvent(Event{TimeVal{1, 60000}, EV_KEY, Keys::KEY_L, KEY_VAL_UP});
    assert(writer.events.empty());

    std::cout << "test_key_repeat_continuous_movement: PASSED\n";
}

static void test_mouse_combos() {
    MockWriter writer;
    TFFEngine engine(&writer);
    engine.setFakeActiveTimer(true);

    // Combo: d + f -> btn_left
    Combo c1;
    c1.keys = {Keys::KEY_D, Keys::KEY_F};
    c1.mouse = MouseAction{MouseActionType::BtnLeft, 0};

    // Combo: j + k -> mouse_up(30)
    Combo c2;
    c2.keys = {Keys::KEY_J, Keys::KEY_K};
    c2.mouse = MouseAction{MouseActionType::MoveUp, 30};

    engine.setCombos({c1, c2});

    // 1. Trigger D+F combo: press D, then F, hold past min_age_us_ (140ms)
    engine.processEvent(Event{TimeVal{1, 0}, EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{1, 20000}, EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{1, 180000}, EV_KEY, Keys::KEY_F, KEY_VAL_REPEAT});
    auto keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::BTN_LEFT);
    assert(keys[0].value == KEY_VAL_DOWN);
    writer.clear();

    // Release D then F -> emits BTN_LEFT UP and cleanly swallows
    engine.processEvent(Event{TimeVal{1, 200000}, EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    keys = writer.keyEvents();
    assert(keys.size() == 1);
    assert(keys[0].code == Keys::BTN_LEFT);
    assert(keys[0].value == KEY_VAL_UP);
    writer.clear();

    engine.processEvent(Event{TimeVal{1, 220000}, EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    assert(writer.events.empty());

    // 2. Trigger J+K combo -> relative mouse move
    engine.processEvent(Event{TimeVal{2, 0}, EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{2, 20000}, EV_KEY, Keys::KEY_K, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{2, 180000}, EV_KEY, Keys::KEY_K, KEY_VAL_REPEAT});
    auto rels = writer.relEvents();
    assert(rels.size() == 1);
    assert(rels[0].code == RelCodes::REL_Y);
    assert(rels[0].value == -30);
    writer.clear();

    engine.processEvent(Event{TimeVal{2, 200000}, EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{2, 220000}, EV_KEY, Keys::KEY_K, KEY_VAL_UP});
    assert(writer.events.empty());

    std::cout << "test_mouse_combos: PASSED\n";
}

static void test_mouse_leader_sequence() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lc;
    lc.key = Keys::KEY_SEMICOLON;
    lc.timeout_us = 1000000LL;

    LeaderSequence seq;
    seq.keys = {Keys::KEY_M, Keys::KEY_L};
    seq.mouse = MouseAction{MouseActionType::BtnLeft, 0};
    lc.sequences.push_back(seq);

    engine.setLeaderConfig(lc);

    // Tap semicolon (leader trigger)
    engine.processEvent(Event{TimeVal{1, 0}, EV_KEY, Keys::KEY_SEMICOLON, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{1, 20000}, EV_KEY, Keys::KEY_SEMICOLON, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Press M
    engine.processEvent(Event{TimeVal{1, 50000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{1, 70000}, EV_KEY, Keys::KEY_M, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Press L -> matches sequence!
    engine.processEvent(Event{TimeVal{1, 90000}, EV_KEY, Keys::KEY_L, KEY_VAL_DOWN});
    auto keys = writer.keyEvents();
    assert(keys.size() == 2);
    assert(keys[0].code == Keys::BTN_LEFT && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::BTN_LEFT && keys[1].value == KEY_VAL_UP);

    engine.processEvent(Event{TimeVal{1, 110000}, EV_KEY, Keys::KEY_L, KEY_VAL_UP});
    assert(!engine.isLeaderActive());

    std::cout << "test_mouse_leader_sequence: PASSED\n";
}

static void test_yaml_parser_mouse_keys() {
    std::string yaml = R"(
mouse:
  speed: 15
  wheel_step: 3

layers:
  mouse:
    h: mouse_left
    j: mouse_down(25)
    k: mouse_up
    l: mouse_right
    u: wheel_up
    d: wheel_down(2)
    f: mouse_btn_left
    s: mouse_btn_right

combos:
  d + f: mouse_btn_left
  j + k: mouse_up(30)
  - keys: [f, g]
    action: mouse_down
)";

    Config config;
    std::string err;
    bool ok = loadYamlConfig(yaml, config, err);
    if (!ok) {
        std::cerr << "Parser error: " << err << "\n";
    }
    assert(ok);

    // Mouse section
    assert(config.mouse.move_speed == 15);
    assert(config.mouse.wheel_step == 3);

    // Layer mappings
    assert(config.layers.size() == 1);
    const auto& lyr = config.layers[0];
    assert(lyr.name == "mouse");
    assert(lyr.mappings.at(Keys::KEY_H).mouse.type == MouseActionType::MoveLeft);
    assert(lyr.mappings.at(Keys::KEY_H).mouse.delta == 0);
    assert(lyr.mappings.at(Keys::KEY_J).mouse.type == MouseActionType::MoveDown);
    assert(lyr.mappings.at(Keys::KEY_J).mouse.delta == 25);
    assert(lyr.mappings.at(Keys::KEY_U).mouse.type == MouseActionType::WheelUp);
    assert(lyr.mappings.at(Keys::KEY_D).mouse.delta == 2);
    assert(lyr.mappings.at(Keys::KEY_F).out_keys.size() == 1 && lyr.mappings.at(Keys::KEY_F).out_keys[0] == Keys::BTN_LEFT);
    assert(lyr.mappings.at(Keys::KEY_S).out_keys.size() == 1 && lyr.mappings.at(Keys::KEY_S).out_keys[0] == Keys::BTN_RIGHT);

    // Combos
    // d+f symmetric -> 2 combos
    // j+k symmetric -> 2 combos
    // f g classic -> 1 combo
    assert(config.combos.size() == 5);
    assert(config.combos[0].mouse.type == MouseActionType::BtnLeft);
    assert(config.combos[2].mouse.type == MouseActionType::MoveUp && config.combos[2].mouse.delta == 30);
    assert(config.combos[4].mouse.type == MouseActionType::MoveDown);

    std::cout << "test_yaml_parser_mouse_keys: PASSED\n";
}

static void test_yaml_validation_errors() {
    Config config;
    std::string err;

    // 1. Invalid mouse speed: 0
    std::string bad_speed1 = "mouse:\n  speed: 0\ncombos:\n  j k: esc\n";
    assert(!loadYamlConfig(bad_speed1, config, err));
    assert(err.find("mouse speed must be between 1 and 1000") != std::string::npos);

    // 2. Invalid mouse speed: 1001
    std::string bad_speed2 = "mouse:\n  speed: 1001\ncombos:\n  j k: esc\n";
    assert(!loadYamlConfig(bad_speed2, config, err));
    assert(err.find("mouse speed must be between 1 and 1000") != std::string::npos);

    // 3. Invalid wheel step: 0
    std::string bad_wheel = "mouse:\n  wheel_step: 0\ncombos:\n  j k: esc\n";
    assert(!loadYamlConfig(bad_wheel, config, err));
    assert(err.find("wheel_step must be between 1 and 100") != std::string::npos);

    // 4. Unknown field in mouse section
    std::string unknown_field = "mouse:\n  invalid_prop: 10\ncombos:\n  j k: esc\n";
    assert(!loadYamlConfig(unknown_field, config, err));
    assert(err.find("unknown field 'invalid_prop'") != std::string::npos);

    std::cout << "test_yaml_validation_errors: PASSED\n";
}

static void test_cheatsheet_mouse_keys() {
    Config config;
    config.mouse.move_speed = 20;
    config.mouse.wheel_step = 2;

    Combo c;
    c.keys = {Keys::KEY_D, Keys::KEY_F};
    c.mouse = MouseAction{MouseActionType::BtnLeft, 0};
    config.combos.push_back(c);

    Layer mouse_layer;
    mouse_layer.name = "mouse";
    LayerAction act;
    act.mouse = MouseAction{MouseActionType::MoveLeft, 25};
    mouse_layer.mappings[Keys::KEY_H] = act;
    config.layers.push_back(mouse_layer);

    CheatsheetOptions opts;
    opts.markdown = true;
    std::string md = Cheatsheet::generate(config, opts);
    assert(md.find("mouse_btn_left") != std::string::npos);
    assert(md.find("mouse_left(25)") != std::string::npos);
    assert(md.find("Mouse Action") != std::string::npos);

    opts.markdown = false;
    opts.color = false;
    std::string plain = Cheatsheet::generate(config, opts);
    assert(plain.find("mouse_btn_left") != std::string::npos);
    assert(plain.find("mouse_left(25)") != std::string::npos);
    assert(plain.find("Mouse Action") != std::string::npos);

    std::cout << "test_cheatsheet_mouse_keys: PASSED\n";
}

int main() {
    test_mouse_action_parsing();
    test_mouse_movement_emission();
    test_mouse_wheel_emission();
    test_mouse_button_press_and_release();
    test_modal_layer_home_row_mouse_keys();
    test_key_repeat_continuous_movement();
    test_mouse_combos();
    test_mouse_leader_sequence();
    test_yaml_parser_mouse_keys();
    test_yaml_validation_errors();
    test_cheatsheet_mouse_keys();

    std::cout << "All mouse keys tests passed successfully!\n";
    return 0;
}
