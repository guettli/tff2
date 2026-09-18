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

    std::cout << "================================================\n";
    std::cout << "ALL TRIPLE COMBO & N-KEY CHORDING TESTS PASSED! ✓\n";
    std::cout << "================================================\n";
    return 0;
}
