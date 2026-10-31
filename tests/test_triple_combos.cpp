#include "tff_types.h"
#include "tff_key_codes.h"
#include "tff_engine.h"
#include "tff_parser.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>
#include <cctype>

using namespace tff;

class VectorWriter : public EventWriter {
public:
    std::vector<Event> events;

    void writeOne(const Event& ev) override { events.push_back(ev); }

    void clear() { events.clear(); }
};

static void runAndCheck(const std::vector<Event>& input_events, const std::vector<Combo>& combos,
                        const std::string& expected_output, const std::string& test_name) {
    VectorWriter writer;
    TFFEngine engine(&writer, combos);
    engine.setFakeActiveTimer(true);

    for (const auto& ev : input_events) {
        if (!engine.processEvent(ev)) {
            break;
        }
    }
    engine.finish();

    std::string actual = eventsToShortCsv(writer.events);
    std::string norm_actual = normalizeShortCsv(actual);
    std::string norm_expected = normalizeShortCsv(expected_output);

    if (norm_actual != norm_expected) {
        std::cerr << "FAIL in " << test_name << "\n";
        std::cerr << "Expected:\n[" << norm_expected << "]\n";
        std::cerr << "Actual:\n[" << norm_actual << "]\n";
        assert(false);
    }
}

// 1. Test all 3! = 6 arrival permutations of d + f + j -> esc
void test_all_6_arrival_permutations() {
    std::cout << "[TEST] Triple combo all 6 arrival permutations... ";

    std::string yaml = "combos:\n  d + f + j: esc\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));
    assert(combos.size() == 6);

    std::vector<KeyCode> keys = {Keys::KEY_D, Keys::KEY_F, Keys::KEY_J};
    std::vector<std::vector<KeyCode>> perms;
    std::sort(keys.begin(), keys.end());
    do {
        perms.push_back(keys);
    } while (std::next_permutation(keys.begin(), keys.end()));
    assert(perms.size() == 6);

    for (size_t p = 0; p < perms.size(); ++p) {
        const auto& order = perms[p];
        std::vector<Event> events;
        int64_t t_us = 1000000;

        // Down in permutation order
        for (KeyCode k : order) {
            events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k, KEY_VAL_DOWN});
            t_us += 15000;  // 15ms apart
        }

        t_us += 50000;  // hold for 50ms

        // Up in reverse order
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, *it, KEY_VAL_UP});
            t_us += 10000;
        }

        std::string expected = "ESC-down\nESC-up\n";
        runAndCheck(events, combos, expected, "Permutation " + std::to_string(p));
    }

    std::cout << "PASS\n";
}

// 2. Test different release orders for d + f + j -> esc
void test_different_release_orders() {
    std::cout << "[TEST] Triple combo different release orders... ";

    std::string yaml = "combos:\n  d + f + j: esc\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    KeyCode k1 = Keys::KEY_D;
    KeyCode k2 = Keys::KEY_F;
    KeyCode k3 = Keys::KEY_J;

    std::vector<std::vector<KeyCode>> release_orders = {
        {k1, k2, k3},  // 1st released first
        {k3, k2, k1},  // 3rd released first
        {k2, k1, k3},  // 2nd released first
        {k2, k3, k1}   // 2nd released first, then 3rd
    };

    for (size_t r = 0; r < release_orders.size(); ++r) {
        const auto& rel = release_orders[r];
        std::vector<Event> events;
        int64_t t_us = 2000000;

        // Down order: D, F, J
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k1, KEY_VAL_DOWN});
        t_us += 20000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k2, KEY_VAL_DOWN});
        t_us += 20000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k3, KEY_VAL_DOWN});
        t_us += 60000;

        // Up in specified release order
        for (KeyCode k : rel) {
            events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k, KEY_VAL_UP});
            t_us += 15000;
        }

        std::string expected = "ESC-down\nESC-up\n";
        runAndCheck(events, combos, expected, "Release order " + std::to_string(r));
    }

    std::cout << "PASS\n";
}

// 3. Test 4-key symmetric combo (a + s + d + f -> mute)
void test_quadruple_combo() {
    std::cout << "[TEST] Quadruple combo (4 keys chorded)... ";

    std::string yaml = "combos:\n  a + s + d + f: mute\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));
    assert(combos.size() == 24);

    // Arbitrary touchdown order: S, F, A, D
    std::vector<KeyCode> down_order = {Keys::KEY_S, Keys::KEY_F, Keys::KEY_A, Keys::KEY_D};
    std::vector<Event> events;
    int64_t t_us = 3000000;

    for (KeyCode k : down_order) {
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k, KEY_VAL_DOWN});
        t_us += 10000;
    }

    t_us += 50000;

    // Release in different order: A, D, S, F
    std::vector<KeyCode> up_order = {Keys::KEY_A, Keys::KEY_D, Keys::KEY_S, Keys::KEY_F};
    for (KeyCode k : up_order) {
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, k, KEY_VAL_UP});
        t_us += 10000;
    }

    std::string expected = "MUTE-down\nMUTE-up\n";
    runAndCheck(events, combos, expected, "Quadruple combo");

    std::cout << "PASS\n";
}

// 4. Test leader key with triple combo (space: d + f + j: esc)
void test_leader_with_triple_combo() {
    std::cout << "[TEST] Leader key with triple combo... ";

    std::string yaml = R"(
combos:
  space:
    d + f + j: esc
)";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));
    assert(combos.size() == 6);

    std::vector<Event> events;
    int64_t t_us = 4000000;

    // Leader: Space
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_SPACE, KEY_VAL_DOWN});
    t_us += 20000;

    // Chord: J, D, F
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    t_us += 50000;

    // Up: Space, then chord keys
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_SPACE, KEY_VAL_UP});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_J, KEY_VAL_UP});

    std::string expected = "ESC-down\nESC-up\n";
    runAndCheck(events, combos, expected, "Leader with triple combo");

    std::cout << "PASS\n";
}

// 5. Incomplete chord: key released before 3rd key pressed -> passthrough
void test_incomplete_chord_released_early() {
    std::cout << "[TEST] Incomplete chord released early (flushes normally)... ";

    std::string yaml = "combos:\n  d + f + j: esc\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    std::vector<Event> events;
    int64_t t_us = 5000000;

    // User presses D, then F, but releases D before J
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    t_us += 20000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    t_us += 20000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    t_us += 20000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_F, KEY_VAL_UP});

    // Output should pass through D and F
    std::string expected = "D-down\nF-down\nD-up\nF-up\n";
    runAndCheck(events, combos, expected, "Incomplete chord");

    std::cout << "PASS\n";
}

// 6. Test repo config tff-combos.yaml has active d + f + j: esc
void test_repo_config_triple_combo() {
    std::cout << "[TEST] Repo config tff-combos.yaml triple combo... ";

    Config config;
    std::string err;
    std::ifstream file("config/tff-combos.yaml");
    if (!file.is_open()) {
        file.open("../config/tff-combos.yaml");
    }
    assert(file.is_open());
    std::stringstream buf;
    buf << file.rdbuf();

    assert(loadYamlConfig(buf.str(), config, err));

    // Verify d + f + j -> esc is present in combos
    bool found_dfj = false;
    for (const auto& c : config.combos) {
        if (c.keys.size() == 3 && c.out_keys.size() == 1 && c.out_keys[0] == Keys::KEY_ESC) {
            std::vector<KeyCode> sorted = c.keys;
            std::sort(sorted.begin(), sorted.end());
            if (sorted[0] == Keys::KEY_D && sorted[1] == Keys::KEY_F && sorted[2] == Keys::KEY_J) {
                found_dfj = true;
                break;
            }
        }
    }
    assert(found_dfj);

    // Test execution with repo combos
    std::vector<Event> events;
    int64_t t_us = 6000000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    t_us += 15000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    t_us += 15000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    t_us += 60000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    t_us += 10000;
    events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_J, KEY_VAL_UP});

    std::string expected = "ESC-down\nESC-up\n";
    runAndCheck(events, config.combos, expected, "Repo config execution");

    std::cout << "PASS\n";
}

// 7. Per-combo custom timeout: required overlap differs from global settings
void test_per_combo_custom_timeout() {
    std::cout << "[TEST] Per-combo custom timeout (required overlap override)... ";

    // Global combo timeout is 40ms.
    // Combo 'a + s: esc' overrides timeout_ms: 80 (requires 80ms overlap).
    std::string yaml = R"(
settings:
  combo_timeout_ms: 40
combos:
  - keys: a + s
    outKeys: esc
    timeout_ms: 80
)";
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    assert(config.combos.size() == 2);
    assert(config.combos[0].overlap_us == 80000);

    // Scenario A: Overlap is 50ms (greater than global 40ms, but less than combo's 80ms)
    // Keys pressed: A down at t=1000ms, S down at t=1020ms.
    // A released at t=1070ms -> overlap = 50ms.
    // Because 50ms < 80ms, the combo must NOT fire! It passes through raw A and S.
    {
        std::vector<Event> events;
        int64_t t_us = 1000000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
        t_us += 20000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_S, KEY_VAL_DOWN});
        t_us += 50000;  // Overlap = 50ms
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
        t_us += 20000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_S, KEY_VAL_UP});

        std::string expected = "A-down\nS-down\nA-up\nS-up\n";
        runAndCheck(events, config.combos, expected, "50ms overlap insufficient for 80ms combo");
    }

    // Scenario B: Overlap is 90ms (greater than combo's 80ms requirement)
    // Keys pressed: A down at t=2000ms, S down at t=2020ms.
    // A released at t=2110ms -> overlap = 90ms.
    // 90ms >= 80ms -> combo matches and emits ESC!
    {
        std::vector<Event> events;
        int64_t t_us = 2000000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
        t_us += 20000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_S, KEY_VAL_DOWN});
        t_us += 90000;  // Overlap = 90ms
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_A, KEY_VAL_UP});
        t_us += 20000;
        events.push_back(Event{TimeVal::fromMicros(t_us), EV_KEY, Keys::KEY_S, KEY_VAL_UP});

        std::string expected = "ESC-down\nESC-up\n";
        runAndCheck(events, config.combos, expected, "90ms overlap matches 80ms combo");
    }

    std::cout << "PASS\n";
}

void test_held_combo_key_times_out_and_emits_down() {
    std::cout << "[TEST] Held combo key times out and emits down for autorepeat... ";

    std::string yaml = "combos:\n  d + f + j: esc\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    VectorWriter writer;
    TFFEngine engine(&writer, combos);
    engine.setFakeActiveTimer(true);

    // 1. Press 'd' down at t = 1,000,000 us (1.0s)
    engine.processEvent(Event{TimeVal::fromMicros(1000000), EV_KEY, Keys::KEY_D, KEY_VAL_DOWN});
    // Immediately after down, key is buffered waiting for possible combo candidates
    assert(writer.events.empty());
    assert(engine.getBufferSize() == 1);

    // 2. Advance time past the 150ms chord collection timeout (t = 1,150,000 us)
    // and fire onTimer. The engine must recognize the timeout, flush 'd' DOWN to the OS,
    // and clear the buffer so the host OS can start autorepeating 'd'.
    engine.onTimer(TimeVal::fromMicros(1150000));
    assert(engine.getBufferSize() == 0);
    assert(!writer.events.empty());
    bool found_d_down = false;
    for (const auto& ev : writer.events) {
        if (ev.code == Keys::KEY_D && ev.value == KEY_VAL_DOWN) {
            found_d_down = true;
        }
    }
    assert(found_d_down);

    // 3. User continues holding 'd' for 850ms, then releases 'd' at t = 2,000,000 us (2.0s)
    engine.processEvent(Event{TimeVal::fromMicros(2000000), EV_KEY, Keys::KEY_D, KEY_VAL_UP});
    assert(engine.getBufferSize() == 0);
    bool found_d_up = false;
    for (const auto& ev : writer.events) {
        if (ev.code == Keys::KEY_D && ev.value == KEY_VAL_UP) {
            found_d_up = true;
        }
    }
    assert(found_d_up);

    std::cout << "PASS\n";
}

// Runs `steps` ("a_ 20 s_ 90 a/ ..." = key down/up and ms to wait) with the config's settings
static void runTimed(const std::string& yaml, const std::string& steps, const std::string& expected,
                     const std::string& test_name) {
    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));
    VectorWriter writer;
    TFFEngine engine(&writer, config.combos);
    engine.setSettings(config.settings);
    engine.setFakeActiveTimer(true);
    int64_t t_us = 1000000;
    std::istringstream in(steps);
    std::string step;
    while (in >> step) {
        if (std::isdigit(static_cast<unsigned char>(step[0]))) {
            t_us += std::stoll(step) * 1000;
            continue;
        }
        KeyCode code = 0;
        assert(wordToKeyCode(step.substr(0, step.size() - 1), code, err));
        engine.processEvent(Event{TimeVal::fromMicros(t_us), EV_KEY, code,
                                  step.back() == '_' ? KEY_VAL_DOWN : KEY_VAL_UP});
    }
    engine.finish();
    const std::string actual = normalizeShortCsv(eventsToShortCsv(writer.events));
    if (actual != normalizeShortCsv(expected)) {
        std::cerr << "FAIL in " << test_name << "\nExpected:\n[" << normalizeShortCsv(expected)
                  << "]\nActual:\n[" << actual << "]\n";
        assert(false);
    }
}

// combo_overlap_ms: how long all keys are held together; combo_gap_ms: max time between presses
void test_combo_overlap_and_gap() {
    std::cout << "[TEST] Combo overlap and gap... ";
    const std::string raw = "A-down\nS-down\nA-up\nS-up\n";
    const std::string esc = "ESC-down\nESC-up\n";
    const std::string combo = "combos:\n  a + s: esc\n";

    // Overlap
    runTimed(combo, "a_ 20 s_ a/ 20 s/", raw, "no overlap types the keys");
    runTimed(combo, "a_ 20 s_ 30 a/ 20 s/", raw, "30ms overlap < 40ms");
    runTimed(combo, "a_ 20 s_ 50 a/ 20 s/", esc, "50ms overlap >= 40ms");
    runTimed("settings:\n  combo_overlap_ms: 60\n" + combo, "a_ 20 s_ 50 a/ 20 s/", raw,
             "50ms overlap < combo_overlap_ms 60");
    runTimed("settings:\n  combo_timeout_ms: 60\n" + combo, "a_ 20 s_ 50 a/ 20 s/", raw,
             "combo_timeout_ms is another name for combo_overlap_ms");

    // Held keys trigger the combo after 140ms, or after a longer combo_overlap_ms
    runTimed(combo, "a_ 20 s_ 145 a/ 20 s/", esc, "held 145ms");
    runTimed("settings:\n  combo_overlap_ms: 200\n" + combo, "a_ 20 s_ 145 a/ 20 s/", raw,
             "held 145ms < combo_overlap_ms 200");
    runTimed("settings:\n  combo_overlap_ms: 200\n" + combo, "a_ 20 s_ 210 a/ 20 s/", esc,
             "held 210ms >= combo_overlap_ms 200");

    // Gap
    runTimed(combo, "a_ 120 s_ 50 a/ 20 s/", esc, "120ms gap <= 150ms");
    runTimed(combo, "a_ 200 s_ 50 a/ 20 s/", raw, "200ms gap > 150ms");
    runTimed("settings:\n  combo_gap_ms: 250\n" + combo, "a_ 200 s_ 50 a/ 20 s/", esc,
             "200ms gap <= combo_gap_ms 250");
    runTimed("combos:\n  a + s: { out: esc, gap_ms: 250 }\n", "a_ 200 s_ 50 a/ 20 s/", esc,
             "per-combo gap_ms");

    runTimed("combos:\n  a + s: { out: esc, gap_ms: 50 }\n", "a_ 100 s_ 50 a/ 20 s/", raw,
             "per-combo gap_ms below the default");
    runTimed(combo + "  a + d: { out: f1, gap_ms: 300 }\n", "a_ 200 s_ 50 a/ 20 s/", raw,
             "another combo's longer gap_ms doesn't apply");
    runTimed("settings:\n  combo_overlap_ms: 200\n" + combo, "a_ 180 s_ 250 a/ 20 s/", raw,
             "a longer overlap doesn't lengthen the gap");
    // Held keys trigger after 140ms even with a long gap; a key tapped meanwhile comes with it
    runTimed("settings:\n  combo_gap_ms: 400\n" + combo, "a_ 20 s_ 200 d_ 10 d/ 50 a/ s/",
             "ESC-down\nD-down\nD-up\nESC-up\n", "held combo with a long gap");
    runTimed(combo + "  a + s + d: { out: f1, gap_ms: 300 }\n", "a_ 20 s_ 200 x_ x/ 50 a/ s/",
             "ESC-down\nX-down\nX-up\nESC-up\n", "longer combo's gap doesn't delay a held combo");

    // A per-combo timeout_ms (= overlap_ms) used to shrink the gap to the same value
    runTimed("combos:\n  a + s: { out: esc, timeout_ms: 80 }\n", "a_ 120 s_ 90 a/ 20 s/", esc,
             "timeout_ms 80 keeps the 150ms gap");
    std::cout << "PASS\n";
}

int main() {
    std::cout << "================================================\n";
    std::cout << "  Testing Triple Combos & N-Key Chording        \n";
    std::cout << "================================================\n";

    test_all_6_arrival_permutations();
    test_different_release_orders();
    test_quadruple_combo();
    test_leader_with_triple_combo();
    test_incomplete_chord_released_early();
    test_repo_config_triple_combo();
    test_per_combo_custom_timeout();
    test_held_combo_key_times_out_and_emits_down();
    test_combo_overlap_and_gap();

    std::cout << "================================================\n";
    std::cout << "ALL TRIPLE COMBO & N-KEY CHORDING TESTS PASSED! ✓\n";
    std::cout << "================================================\n";
    return 0;
}
