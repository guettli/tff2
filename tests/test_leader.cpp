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

static void test_sequential_text_snippet() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lcfg;
    lcfg.key = Keys::KEY_CAPSLOCK;
    lcfg.timeout_us = 1000000LL;
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_W, Keys::KEY_Q}, {}, ":wq\n"));
    engine.setLeaderConfig(lcfg);

    // 1. Tap CapsLock (down at 10ms, up at 50ms)
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(writer.keyEvents().empty());
    assert(engine.isLeaderActive());

    // 2. Tap W (down at 100ms, up at 150ms) - valid prefix
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});
    assert(writer.keyEvents().empty());
    assert(engine.getLeaderBuffer().size() == 1);
    assert(engine.getLeaderBuffer()[0] == Keys::KEY_W);

    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    assert(writer.keyEvents().empty());

    // 3. Tap Q (down at 200ms, up at 250ms) - exact match!
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_Q, KEY_VAL_DOWN});
    assert(!engine.isLeaderActive());
    auto keys = writer.keyEvents();
    assert(!keys.empty());

    // Verify text sequence ":wq\n" emitted:
    // ':' = Shift + Semicolon
    assert(keys[0].code == Keys::KEY_LEFTSHIFT && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::KEY_SEMICOLON && keys[1].value == KEY_VAL_DOWN);
    assert(keys[2].code == Keys::KEY_SEMICOLON && keys[2].value == KEY_VAL_UP);
    assert(keys[3].code == Keys::KEY_LEFTSHIFT && keys[3].value == KEY_VAL_UP);
    // 'w'
    assert(keys[4].code == Keys::KEY_W && keys[4].value == KEY_VAL_DOWN);
    assert(keys[5].code == Keys::KEY_W && keys[5].value == KEY_VAL_UP);
    // 'q'
    assert(keys[6].code == Keys::KEY_Q && keys[6].value == KEY_VAL_DOWN);
    assert(keys[7].code == Keys::KEY_Q && keys[7].value == KEY_VAL_UP);
    // '\n' = Enter
    assert(keys[8].code == Keys::KEY_ENTER && keys[8].value == KEY_VAL_DOWN);
    assert(keys[9].code == Keys::KEY_ENTER && keys[9].value == KEY_VAL_UP);

    // 4. Release Q - should be swallowed (already handled via leader_pending_releases_)
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_Q, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 10);

    std::cout << "test_sequential_text_snippet: PASSED\n";
}

static void test_sequential_out_keys_chord() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lcfg;
    lcfg.key = Keys::KEY_CAPSLOCK;
    lcfg.timeout_us = 1000000LL;
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_B}, {Keys::KEY_LEFTCTRL, Keys::KEY_B}, ""));
    engine.setLeaderConfig(lcfg);

    // Tap CapsLock
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Tap B
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_B, KEY_VAL_DOWN});
    assert(!engine.isLeaderActive());
    auto keys = writer.keyEvents();
    assert(keys.size() == 4);
    assert(keys[0].code == Keys::KEY_LEFTCTRL && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::KEY_B && keys[1].value == KEY_VAL_DOWN);
    assert(keys[2].code == Keys::KEY_B && keys[2].value == KEY_VAL_UP);
    assert(keys[3].code == Keys::KEY_LEFTCTRL && keys[3].value == KEY_VAL_UP);

    // Release B
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_B, KEY_VAL_UP});
    assert(writer.keyEvents().size() == 4);

    std::cout << "test_sequential_out_keys_chord: PASSED\n";
}

static void test_multi_step_sequence() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lcfg;
    lcfg.key = Keys::KEY_CAPSLOCK;
    lcfg.timeout_us = 500000LL; // 500ms
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_G, Keys::KEY_S}, {}, "git status\n"));
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_G, Keys::KEY_C, Keys::KEY_M}, {}, "git commit -m \"\"\n"));
    engine.setLeaderConfig(lcfg);

    // Tap CapsLock
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Tap G
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_G, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_G, KEY_VAL_UP});
    assert(engine.isLeaderActive());
    assert(engine.getLeaderBuffer().size() == 1);

    // Tap C
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_C, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_C, KEY_VAL_UP});
    assert(engine.isLeaderActive());
    assert(engine.getLeaderBuffer().size() == 2);

    // Tap M -> triggers git commit -m ""\n
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_M, KEY_VAL_DOWN});
    assert(!engine.isLeaderActive());
    assert(!writer.keyEvents().empty());

    std::cout << "test_multi_step_sequence: PASSED\n";
}

static void test_inactivity_timeout_and_replay() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lcfg;
    lcfg.key = Keys::KEY_CAPSLOCK;
    lcfg.timeout_us = 500000LL; // 500ms
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_W, Keys::KEY_Q}, {}, ":wq\n"));
    engine.setLeaderConfig(lcfg);

    // Tap CapsLock
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Tap W at 100ms
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    assert(writer.keyEvents().empty());

    // Timer expires at 700ms (> 150ms + 500ms)
    assert(engine.hasActiveTimer());
    TimeVal next_timer = engine.getActiveTimerTime();
    assert(next_timer.toMicros() == 650000LL);

    engine.onTimer(TimeVal{0, 700000});
    assert(!engine.isLeaderActive());

    // Verify that W down and W up were replayed to writer!
    auto keys = writer.keyEvents();
    assert(keys.size() == 2);
    assert(keys[0].code == Keys::KEY_W && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::KEY_W && keys[1].value == KEY_VAL_UP);

    std::cout << "test_inactivity_timeout_and_replay: PASSED\n";
}

static void test_mismatch_cancellation_and_replay() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lcfg;
    lcfg.key = Keys::KEY_CAPSLOCK;
    lcfg.timeout_us = 1000000LL;
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_W, Keys::KEY_Q}, {}, ":wq\n"));
    engine.setLeaderConfig(lcfg);

    // Tap CapsLock
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Tap W
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    assert(writer.keyEvents().empty());

    // Press X (mismatch!) at 200ms
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_X, KEY_VAL_DOWN});
    assert(!engine.isLeaderActive());

    // Replayed W (down, up), then X down should be processed
    auto keys = writer.keyEvents();
    assert(keys.size() >= 2);
    assert(keys[0].code == Keys::KEY_W && keys[0].value == KEY_VAL_DOWN);
    assert(keys[1].code == Keys::KEY_W && keys[1].value == KEY_VAL_UP);

    // Finish X up
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_X, KEY_VAL_UP});
    engine.finish();

    // Verify all keys (W down, W up, X down, X up) are emitted
    auto all_keys = writer.keyEvents();
    bool found_x_down = false;
    bool found_x_up = false;
    for (const auto& k : all_keys) {
        if (k.code == Keys::KEY_X && k.value == KEY_VAL_DOWN) found_x_down = true;
        if (k.code == Keys::KEY_X && k.value == KEY_VAL_UP) found_x_up = true;
    }
    assert(found_x_down && found_x_up);

    std::cout << "test_mismatch_cancellation_and_replay: PASSED\n";
}

static void test_leader_double_tap_cancellation() {
    MockWriter writer;
    TFFEngine engine(&writer);

    LeaderConfig lcfg;
    lcfg.key = Keys::KEY_CAPSLOCK;
    lcfg.timeout_us = 1000000LL;
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_W, Keys::KEY_Q}, {}, ":wq\n"));
    engine.setLeaderConfig(lcfg);

    // Tap CapsLock once
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Tap CapsLock second time -> cancels leader mode
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(!engine.isLeaderActive());
    assert(writer.keyEvents().empty());

    std::cout << "test_leader_double_tap_cancellation: PASSED\n";
}

static void test_dual_role_tap_leader() {
    MockWriter writer;
    TFFEngine engine(&writer);

    TapHoldKey th;
    th.key = Keys::KEY_CAPSLOCK;
    th.tap_leader = true;
    th.hold_key = Keys::KEY_LEFTMETA;
    th.timeout_us = 200000LL; // 200ms
    engine.setTapHoldKeys({th});

    LeaderConfig lcfg;
    lcfg.timeout_us = 1000000LL;
    lcfg.sequences.push_back(LeaderSequence({Keys::KEY_W, Keys::KEY_Q}, {}, ":wq\n"));
    engine.setLeaderConfig(lcfg);

    // 1. Short tap on CapsLock (<200ms) -> activates leader mode!
    engine.processEvent(Event{TimeVal{0, 10000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    assert(!engine.isLeaderActive());
    engine.processEvent(Event{TimeVal{0, 50000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    assert(engine.isLeaderActive());

    // Type 'w q'
    engine.processEvent(Event{TimeVal{0, 100000}, EV_KEY, Keys::KEY_W, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 150000}, EV_KEY, Keys::KEY_W, KEY_VAL_UP});
    engine.processEvent(Event{TimeVal{0, 200000}, EV_KEY, Keys::KEY_Q, KEY_VAL_DOWN});
    engine.processEvent(Event{TimeVal{0, 250000}, EV_KEY, Keys::KEY_Q, KEY_VAL_UP});
    assert(!engine.isLeaderActive());
    assert(!writer.keyEvents().empty());

    writer.clear();

    // 2. Long hold on CapsLock (>200ms) -> emits Super, leader NOT activated!
    engine.processEvent(Event{TimeVal{0, 300000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_DOWN});
    engine.onTimer(TimeVal{0, 550000}); // expire after 200ms
    auto hold_keys = writer.keyEvents();
    assert(hold_keys.size() == 1);
    assert(hold_keys[0].code == Keys::KEY_LEFTMETA && hold_keys[0].value == KEY_VAL_DOWN);
    assert(!engine.isLeaderActive());

    // Release CapsLock
    engine.processEvent(Event{TimeVal{0, 600000}, EV_KEY, Keys::KEY_CAPSLOCK, KEY_VAL_UP});
    hold_keys = writer.keyEvents();
    assert(hold_keys.size() == 2);
    assert(hold_keys[1].code == Keys::KEY_LEFTMETA && hold_keys[1].value == KEY_VAL_UP);
    assert(!engine.isLeaderActive());

    std::cout << "test_dual_role_tap_leader: PASSED\n";
}

static void test_yaml_parsing_and_validation() {
    // 1. Valid dictionary format
    const std::string valid_dict_yaml = R"(
leader:
  key: capslock
  timeout_ms: 800
  sequences:
    "w q": ":wq\n"
    "g s": "git status\n"
    "b": ctrl+b
)";
    Config cfg1;
    std::string err;
    assert(loadYamlConfig(valid_dict_yaml, cfg1, err));
    assert(cfg1.leader.key == Keys::KEY_CAPSLOCK);
    assert(cfg1.leader.timeout_us == 800000LL);
    assert(cfg1.leader.sequences.size() == 3);
    assert(cfg1.leader.sequences[0].keys == std::vector<KeyCode>({Keys::KEY_W, Keys::KEY_Q}));
    assert(cfg1.leader.sequences[0].text == ":wq\n");
    assert(cfg1.leader.sequences[2].keys == std::vector<KeyCode>({Keys::KEY_B}));
    assert(cfg1.leader.sequences[2].out_keys == std::vector<KeyCode>({Keys::KEY_LEFTCTRL, Keys::KEY_B}));

    // 2. Valid list format and compact tap_hold
    const std::string valid_list_yaml = R"(
tap_hold:
  capslock: [leader, super, 200]

leader:
  timeout_ms: 1200
  sequences:
    - keys: [w, q]
      text: ":wq\n"
    - keys: "g s"
      out: ctrl+s
)";
    Config cfg2;
    err.clear();
    assert(loadYamlConfig(valid_list_yaml, cfg2, err));
    assert(cfg2.tap_hold_keys.size() == 1);
    assert(cfg2.tap_hold_keys[0].tap_leader);
    assert(cfg2.leader.key == Keys::KEY_CAPSLOCK); // Auto-bound from tap_leader!
    assert(cfg2.leader.timeout_us == 1200000LL);
    assert(cfg2.leader.sequences.size() == 2);

    // 3. Validation error: duplicate sequence
    const std::string dup_yaml = R"(
leader:
  key: capslock
  sequences:
    "w q": ":wq\n"
    "w q": ":write-quit\n"
)";
    Config cfg3;
    err.clear();
    assert(!loadYamlConfig(dup_yaml, cfg3, err));
    assert(err.find("duplicate leader sequence") != std::string::npos);

    // 4. Validation error: prefix collision
    const std::string prefix_yaml = R"(
leader:
  key: capslock
  sequences:
    "w": ":w\n"
    "w q": ":wq\n"
)";
    Config cfg4;
    err.clear();
    assert(!loadYamlConfig(prefix_yaml, cfg4, err));
    assert(err.find("prefix") != std::string::npos);

    // 5. Validation error: negative timeout
    const std::string neg_yaml = R"(
leader:
  key: capslock
  timeout_ms: -50
  sequences:
    "w q": ":wq\n"
)";
    Config cfg5;
    err.clear();
    assert(!loadYamlConfig(neg_yaml, cfg5, err));
    assert(err.find("positive") != std::string::npos);

    // 6. Validation error: conflict with combos
    const std::string conflict_yaml = R"(
combos:
  capslock + a: b

leader:
  key: capslock
  sequences:
    "w q": ":wq\n"
)";
    Config cfg6;
    err.clear();
    assert(!loadYamlConfig(conflict_yaml, cfg6, err));
    assert(err.find("cannot be used in both combos and leader") != std::string::npos);

    std::cout << "test_yaml_parsing_and_validation: PASSED\n";
}

static void test_cheatsheet_leader() {
    const std::string yaml = R"(
leader:
  key: capslock
  timeout_ms: 1000
  sequences:
    "w q": ":wq\n"
    "b": ctrl+b
)";
    Config cfg;
    std::string err;
    assert(loadYamlConfig(yaml, cfg, err));

    CheatsheetOptions opts;
    opts.color = false;
    opts.markdown = false;
    std::string ansi_out = Cheatsheet::generate(cfg, opts);
    assert(ansi_out.find("[ Sequential Leader Sequences ]") != std::string::npos);
    assert(ansi_out.find("w q") != std::string::npos);
    assert(ansi_out.find("\":wq\\n\"") != std::string::npos);

    opts.markdown = true;
    std::string md_out = Cheatsheet::generate(cfg, opts);
    assert(md_out.find("## Sequential Leader Key Sequences") != std::string::npos);
    assert(md_out.find("`w q`") != std::string::npos);

    std::cout << "test_cheatsheet_leader: PASSED\n";
}

static void test_rp2040_leader_integration() {
    RP2040Platform platform;
    const std::string yaml = R"(
leader:
  key: capslock
  timeout_ms: 1000
  sequences:
    "w q": ":wq\n"
)";
    bool loaded = platform.loadConfiguration(yaml);
    assert(loaded);
    bool initialized = platform.initialize();
    assert(initialized);

    // USB HID usage codes:
    // CapsLock: 0x39
    // W: 0x1A
    // Q: 0x14

    // 1. Tap CapsLock (0x39)
    platform.processHostKeyEvent(0x39, true);
    platform.processHostKeyEvent(0x39, false);

    // 2. Tap W (0x1A)
    platform.processHostKeyEvent(0x1A, true);
    platform.processHostKeyEvent(0x1A, false);

    // 3. Tap Q (0x14)
    platform.processHostKeyEvent(0x14, true);
    platform.processHostKeyEvent(0x14, false);

    const auto& emitted = platform.getEmittedKeys();
    assert(!emitted.empty());

    std::cout << "test_rp2040_leader_integration: PASSED\n";
}

int main() {
    std::cout << "=== Running Leader Sequence Tests ===\n";
    test_sequential_text_snippet();
    test_sequential_out_keys_chord();
    test_multi_step_sequence();
    test_inactivity_timeout_and_replay();
    test_mismatch_cancellation_and_replay();
    test_leader_double_tap_cancellation();
    test_dual_role_tap_leader();
    test_yaml_parsing_and_validation();
    test_cheatsheet_leader();
    test_rp2040_leader_integration();
    std::cout << "All leader tests PASSED successfully!\n";
    return 0;
}
