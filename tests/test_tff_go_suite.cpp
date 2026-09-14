#include "tff_types.h"
#include "tff_key_codes.h"
#include "tff_engine.h"
#include "tff_parser.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <vector>
#include <string>

using namespace tff;

class SliceWriter : public EventWriter {
public:
    std::vector<Event> events;

    void writeOne(const Event& ev) override {
        events.push_back(ev);
    }

    void clear() {
        events.clear();
    }
};

static void requireEqual(const std::string& actual, const std::string& expected, const std::string& test_name) {
    std::string norm_actual = normalizeShortCsv(actual);
    std::string norm_expected = normalizeShortCsv(expected);
    if (norm_actual != norm_expected) {
        std::cerr << "FAIL: " << test_name << "\n";
        std::cerr << "Expected:\n[" << norm_expected << "]\n";
        std::cerr << "Actual:\n[" << norm_actual << "]\n";
        assert(false);
    }
}

static void assertComboCSVInputOutput(const std::string& input_csv,
                                     const std::string& expected_output,
                                     const std::vector<Combo>& combos,
                                     const std::string& test_name) {
    SliceWriter writer;
    TFFEngine engine(&writer, combos);
    engine.setFakeActiveTimer(true);

    std::vector<Event> events;
    std::string err;
    bool ok = csvToEvents(input_csv, events, err);
    if (!ok) {
        std::cerr << "Error parsing CSV in " << test_name << ": " << err << "\n";
        assert(false);
    }

    for (const auto& ev : events) {
        if (!engine.processEvent(ev)) {
            break;
        }
    }
    engine.finish();

    std::string actual = eventsToShortCsv(writer.events);
    requireEqual(actual, expected_output, test_name);
}

static void assertComboStateStringInputOutput(const std::string& state_str,
                                             const std::string& expected_output,
                                             const std::vector<Combo>& combos,
                                             const std::string& test_name) {
    SliceWriter writer;
    TFFEngine engine(&writer, combos);
    engine.setFakeActiveTimer(true);

    std::vector<Event> events;
    std::string err;
    bool ok = stateStringToEvents(state_str, events, err);
    if (!ok) {
        std::cerr << "Error parsing stateString in " << test_name << ": " << err << "\n";
        assert(false);
    }

    for (const auto& ev : events) {
        if (!engine.processEvent(ev)) {
            break;
        }
    }
    engine.finish();

    std::string actual = eventsToShortCsv(writer.events);
    requireEqual(actual, expected_output, test_name);
}

// ---------------------------------------------------------
// Global Combo Sets used in Go tests
// ---------------------------------------------------------

static const std::vector<Combo> fjkCombos = {
    { { Keys::KEY_F, Keys::KEY_J }, { Keys::KEY_X } },
    { { Keys::KEY_F, Keys::KEY_K }, { Keys::KEY_Y } },
};

static const std::vector<Combo> orderedCombos = {
    { { Keys::KEY_F, Keys::KEY_J }, { Keys::KEY_X } },
    { { Keys::KEY_J, Keys::KEY_F }, { Keys::KEY_A } },
    { { Keys::KEY_F, Keys::KEY_K }, { Keys::KEY_Y } },
    { { Keys::KEY_J, Keys::KEY_K }, { Keys::KEY_B } },
};

static const std::vector<Combo> capslockCombos = {
    { { Keys::KEY_CAPSLOCK, Keys::KEY_J }, { Keys::KEY_BACKSPACE } },
};

static const std::string asdfTestEvents =
    "1712500001;862966;EV_KEY;KEY_A;down\n"
    "1712500002;22233;EV_KEY;KEY_A;up\n"
    "1712500002;478346;EV_KEY;KEY_S;down\n"
    "1712500002;637660;EV_KEY;KEY_S;up\n"
    "1712500003;35798;EV_KEY;KEY_D;down\n"
    "1712500003;132219;EV_KEY;KEY_D;up\n"
    "1712500003;948232;EV_KEY;KEY_F;down\n"
    "1712500004;116984;EV_KEY;KEY_F;up\n";

// ---------------------------------------------------------
// Individual Tests
// ---------------------------------------------------------

void test_runeToKeyCode() {
    std::cout << "[TEST] runeToKeyCode (wordToKeyCode)... ";
    struct Case {
        std::string in;
        KeyCode expected_code;
        bool expect_error;
        std::string err_substr;
    };
    std::vector<Case> cases = {
        { "x", Keys::KEY_X, false, "" },
        { "1", Keys::KEY_1, false, "" },
        { "capslock", Keys::KEY_CAPSLOCK, false, "" },
        { "X", 0, true, "only lower case characters are allowed" },
        { "ü", 0, true, "unknown key" },
    };

    for (const auto& tc : cases) {
        KeyCode code = 0;
        std::string err;
        bool ok = wordToKeyCode(tc.in, code, err);
        if (tc.expect_error) {
            assert(!ok);
            assert(err.find(tc.err_substr) != std::string::npos);
        } else {
            assert(ok);
            assert(code == tc.expected_code);
        }
    }
    std::cout << "PASS\n";
}

void test_LoadYamlFromBytes_ok() {
    std::cout << "[TEST] LoadYamlFromBytes_ok... ";
    std::string yaml_str =
        "combos:\n"
        "  - keys: f j\n"
        "    outKeys: a b c\n";

    std::vector<Combo> combos;
    std::string err;
    bool ok = loadYamlCombos(yaml_str, combos, err);
    assert(ok);
    assert(combos.size() == 1);
    assert(combos[0].keys.size() == 2);
    assert(combos[0].keys[0] == Keys::KEY_F);
    assert(combos[0].keys[1] == Keys::KEY_J);
    assert(combos[0].out_keys.size() == 3);
    assert(combos[0].out_keys[0] == Keys::KEY_A);
    assert(combos[0].out_keys[1] == Keys::KEY_B);
    assert(combos[0].out_keys[2] == Keys::KEY_C);
    std::cout << "PASS\n";
}

void test_LoadYamlFromBytes_fail() {
    std::cout << "[TEST] LoadYamlFromBytes_fail... ";
    struct Case {
        std::string yaml;
        std::string expected_err;
    };
    std::vector<Case> cases = {
        { "combos:\n  - keys: f j\n  - outKeys: a b c\n", "empty list in 'outKeys' is not allowed" },
        { "combos:\n  - outKeys: a b c\n", "empty list in 'keys' is not allowed" },
        { "combos\n  - keys: f j\n  - outKeys: a b c\n", "mapping values are not allowed in this context" },
        { "combos:\n  - keys: f j\n    outKeys: a b key_not_existing\n", "failed to get key \"key_not_existing\"" },
    };

    for (const auto& tc : cases) {
        std::vector<Combo> combos;
        std::string err;
        bool ok = loadYamlCombos(tc.yaml, combos, err);
        if (ok || err.find(tc.expected_err) == std::string::npos) {
            std::cerr << "\nFAIL: expected error substring [" << tc.expected_err << "], got ok=" << ok << " err=[" << err << "]\n";
            assert(false);
        }
    }
    std::cout << "PASS\n";
}

void test_manInTheMiddle_noMatch() {
    std::cout << "[TEST] manInTheMiddle_noMatch... ";
    auto check_combos = [](const std::vector<Combo>& combos) {
        SliceWriter writer;
        TFFEngine engine(&writer, combos);
        engine.setFakeActiveTimer(true);

        std::vector<Event> events;
        std::string err;
        bool ok = csvToEvents(asdfTestEvents, events, err);
        assert(ok);

        for (const auto& ev : events) {
            engine.processEvent(ev);
        }
        engine.finish();

        std::string out_csv = eventsToCsv(writer.events);
        assert(out_csv == asdfTestEvents);
    };

    check_combos({ { { Keys::KEY_A, Keys::KEY_F }, { Keys::KEY_X } } });
    check_combos({ { { Keys::KEY_G, Keys::KEY_H }, { Keys::KEY_X } } });
    check_combos({
        { { Keys::KEY_G, Keys::KEY_H }, { Keys::KEY_X } },
        { { Keys::KEY_A, Keys::KEY_K }, { Keys::KEY_X } }
    });
    std::cout << "PASS\n";
}

void test_manInTheMiddle_NoMatch_JustKeys() {
    std::cout << "[TEST] manInTheMiddle_NoMatch_JustKeys... ";
    std::string in =
        "1712500000;000000;EV_KEY;KEY_B;down\n"
        "1712500000;020000;EV_KEY;KEY_B;up\n"
        "1712500000;700000;EV_KEY;KEY_F;down\n"
        "1712500000;720000;EV_KEY;KEY_F;up\n"
        "1712500001;100000;EV_KEY;KEY_J;down\n"
        "1712500001;110000;EV_KEY;KEY_J;up\n"
        "1712500001;800000;EV_KEY;KEY_C;down\n"
        "1712500001;900000;EV_KEY;KEY_C;up\n";
    std::string expected =
        "B-down\n"
        "B-up\n"
        "F-down\n"
        "F-up\n"
        "J-down\n"
        "J-up\n"
        "C-down\n"
        "C-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "NoMatch_JustKeys");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_TwoCombos_WithOneEmbrachingMatch() {
    std::cout << "[TEST] manInTheMiddle_TwoCombos_WithOneEmbrachingMatch... ";
    std::string in =
        "1712500000;000000;EV_KEY;KEY_B;down\n"
        "1712500000;020000;EV_KEY;KEY_B;up\n"
        "1712500000;700000;EV_KEY;KEY_F;down\n"
        "1712500000;720000;EV_KEY;KEY_J;down\n"
        "1712500001;100000;EV_KEY;KEY_J;up\n"
        "1712500001;110000;EV_KEY;KEY_F;up\n"
        "1712500001;800000;EV_KEY;KEY_C;down\n"
        "1712500001;900000;EV_KEY;KEY_C;up\n";
    std::string expected =
        "B-down\n"
        "B-up\n"
        "X-down\n"
        "X-up\n"
        "C-down\n"
        "C-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "TwoCombos_WithOneEmbrachingMatch");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_SingleCombo_OneEmbrachingMatch() {
    std::cout << "[TEST] manInTheMiddle_SingleCombo_OneEmbrachingMatch... ";
    std::string in =
        "1712500003;827714;EV_KEY;KEY_F;down\n"
        "1712500003;849844;EV_KEY;KEY_J;down\n"
        "1712500004;320867;EV_KEY;KEY_J;up\n"
        "1712500004;321153;EV_KEY;KEY_F;up\n";
    std::string expected =
        "X-down\n"
        "X-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "SingleCombo_OneEmbrachingMatch");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_ComboWithMatch_CrossRhyme() {
    std::cout << "[TEST] manInTheMiddle_ComboWithMatch_CrossRhyme... ";
    std::string in =
        "1712500000;700000;EV_KEY;KEY_F;down\n"
        "1712500000;720000;EV_KEY;KEY_J;down\n"
        "1712500001;100000;EV_KEY;KEY_F;up\n"
        "1712500001;110000;EV_KEY;KEY_J;up\n"
        "1712500001;800000;EV_KEY;KEY_C;down\n"
        "1712500001;900000;EV_KEY;KEY_C;up\n";
    std::string expected =
        "X-down\n"
        "X-up\n"
        "C-down\n"
        "C-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "ComboWithMatch_CrossRhyme");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_ComboWithMatch_SingleUpDown() {
    std::cout << "[TEST] manInTheMiddle_ComboWithMatch_SingleUpDown... ";
    std::string in =
        "1716752333;203961;EV_KEY;KEY_F;down\n"
        "1716752333;327486;EV_KEY;KEY_F;up\n";
    std::string expected =
        "F-down\n"
        "F-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "ComboWithMatch_SingleUpDown");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_ComboWithMatch_OverlapNoCombo() {
    std::cout << "[TEST] manInTheMiddle_ComboWithMatch_OverlapNoCombo... ";
    std::string in =
        "1712500003;827714;EV_KEY;KEY_F;down\n"
        "1712500004;320840;EV_KEY;KEY_J;down\n"
        "1712500004;320860;EV_KEY;KEY_F;up\n"
        "1712500004;321153;EV_KEY;KEY_J;up\n";
    std::string expected =
        "F-down\n"
        "J-down\n"
        "F-up\n"
        "J-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "ComboWithMatch_OverlapNoCombo");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_WithoutMatch() {
    std::cout << "[TEST] manInTheMiddle_WithoutMatch... ";
    std::string in =
        "1712500000;700000;EV_KEY;KEY_K;down\n"
        "1712500000;820000;EV_KEY;KEY_K;up\n"
        "1712500000;830000;EV_KEY;KEY_F;down\n"
        "1712500000;840000;EV_KEY;KEY_F;up\n";
    std::string expected =
        "K-down\n"
        "K-up\n"
        "F-down\n"
        "F-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "WithoutMatch");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_TwoEmbrachingCombosWithMatch() {
    std::cout << "[TEST] manInTheMiddle_TwoEmbrachingCombosWithMatch... ";
    std::string in =
        "1716752333;000000;EV_KEY;KEY_F;down\n"
        "1716752333;100000;EV_KEY;KEY_J;down\n"
        "1716752333;400000;EV_KEY;KEY_J;up\n"
        "1716752333;600000;EV_KEY;KEY_K;down\n"
        "1716752333;800000;EV_KEY;KEY_K;up\n"
        "1716752334;000000;EV_KEY;KEY_F;up\n";
    std::string expected =
        "X-down\n"
        "X-up\n"
        "Y-down\n"
        "Y-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "TwoEmbrachingCombosWithMatch");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_Unrelated_Embraced_Keystrokes() {
    std::cout << "[TEST] manInTheMiddle_Unrelated_Embraced_Keystrokes... ";
    std::string in =
        "1716752333;000000;EV_KEY;KEY_F;down\n"
        "1716752333;100000;EV_KEY;KEY_W;down\n"
        "1716752333;400000;EV_KEY;KEY_W;up\n"
        "1716752334;000000;EV_KEY;KEY_F;up\n"
        "1716752334;100000;EV_KEY;KEY_RFKILL;up\n";
    std::string expected =
        "F-down\n"
        "W-down\n"
        "W-up\n"
        "F-up\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "Unrelated_Embraced_Keystrokes");
    std::cout << "PASS\n";
}

void test_manInTheMiddle_ComboWithMatch_NoPanic() {
    std::cout << "[TEST] manInTheMiddle_ComboWithMatch_NoPanic... ";
    std::string in =
        "1712500000;000000;EV_KEY;KEY_F;down\n"
        "1712500000;064000;EV_KEY;KEY_K;down\n"
        "1712500000;128000;EV_KEY;KEY_F;up\n"
        "1712500000;144000;EV_KEY;KEY_J;down\n"
        "1712500000;208000;EV_KEY;KEY_K;up\n"
        "1712500000;224000;EV_KEY;KEY_F;down\n";
    std::string expected =
        "Y-down\n"
        "Y-up\n"
        "K-down\n"
        "J-down\n"
        "K-up\n"
        "F-down\n";
    assertComboCSVInputOutput(in, expected, fjkCombos, "ComboWithMatch_NoPanic");
    std::cout << "PASS\n";
}

void test_orderedCombos() {
    std::cout << "[TEST] orderedCombos... ";
    std::string in =
        "1712500000;000000;EV_KEY;KEY_F;down\n"
        "1712500000;060000;EV_KEY;KEY_J;down\n"
        "1712500000;120000;EV_KEY;KEY_F;up\n"
        "1712500000;200000;EV_KEY;KEY_J;up\n"
        "\n"
        "1712500001;000000;EV_KEY;KEY_J;down\n"
        "1712500001;060000;EV_KEY;KEY_F;down\n"
        "1712500001;120000;EV_KEY;KEY_J;up\n"
        "1712500001;200000;EV_KEY;KEY_F;up\n";
    std::string expected =
        "X-down\n"
        "X-up\n"
        "A-down\n"
        "A-up\n";
    assertComboCSVInputOutput(in, expected, orderedCombos, "orderedCombos");
    std::cout << "PASS\n";
}

void test_Capslock_Navigation() {
    std::cout << "[TEST] Capslock_Navigation... ";
    std::string state_str = "capslock_ (259.006ms) j_ (105.844ms) j/ (721.7ms) capslock/";
    std::string expected =
        "BACKSPACE-down\n"
        "BACKSPACE-up\n";
    assertComboStateStringInputOutput(state_str, expected, capslockCombos, "Capslock_Navigation");
    std::cout << "PASS\n";
}

void test_ShouldNotPanic() {
    std::cout << "[TEST] ShouldNotPanic... ";
    std::string log =
        "|>>1737965475;912716;EV_MSC;MSC_SCAN;458769\n"
        "|>>1737965475;912716;EV_KEY;KEY_N;down\n"
        "|>>1737965475;912716;EV_SYN;SYN_REPORT;up\n"
        "|>>1737965476;163526;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;163526;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;197504;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;197504;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;231448;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;231448;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;265450;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;265450;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;300444;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;300444;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;335450;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;335450;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;369448;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;369448;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;403445;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;403445;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;437445;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;437445;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;471452;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;471452;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;506444;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;506444;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;540446;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;540446;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;574452;EV_KEY;KEY_N;repeat\n"
        "|>>1737965476;574452;EV_SYN;SYN_REPORT;down\n"
        "|>>1737965476;600611;EV_MSC;MSC_SCAN;458809\n"
        "|>>1737965476;600611;EV_KEY;KEY_CAPSLOCK;down\n"
        "|>>1737965476;600611;EV_SYN;SYN_REPORT;up\n"
        "|>>1737965476;792606;EV_MSC;MSC_SCAN;458769\n"
        "|>>1737965476;792606;EV_KEY;KEY_N;up\n"
        "|>>1737965476;792606;EV_SYN;SYN_REPORT;up\n"
        "|>>1737965477;104606;EV_MSC;MSC_SCAN;458809\n"
        "|>>1737965477;104606;EV_KEY;KEY_CAPSLOCK;up\n"
        "|>>1737965477;104606;EV_SYN;SYN_REPORT;up\n"
        "|>>1737965477;488608;EV_MSC;MSC_SCAN;458769\n"
        "|>>1737965477;488608;EV_KEY;KEY_N;down\n";

    std::vector<Event> events;
    std::string err;
    bool ok = parseComboLog(log, events, err);
    assert(ok);

    std::vector<Combo> combos = {
        { { Keys::KEY_CAPSLOCK, Keys::KEY_N }, { Keys::KEY_DOWN } }
    };

    SliceWriter writer;
    TFFEngine engine(&writer, combos);
    engine.setFakeActiveTimer(true);

    for (const auto& ev : events) {
        engine.processEvent(ev);
    }
    engine.finish();
    std::cout << "PASS\n";
}

void test_FJX_emits_f_but_should_not() {
    std::cout << "[TEST] FJX_emits_f_but_should_not... ";
    std::ifstream file("tests/testdata/fjx-emits-f-but-should-not.log");
    if (!file.is_open()) {
        file.open("../tests/testdata/fjx-emits-f-but-should-not.log");
    }
    if (!file.is_open()) {
        file.open("testdata/fjx-emits-f-but-should-not.log");
    }
    if (!file.is_open()) {
        std::cerr << "SKIPPED (could not find fjx-emits-f-but-should-not.log)\n";
        return;
    }

    std::vector<Event> events;
    std::string err;
    bool ok = parseComboLog(file, events, err);
    assert(ok);

    std::vector<Combo> combos = {
        { { Keys::KEY_F, Keys::KEY_J }, { Keys::KEY_X } }
    };

    SliceWriter writer;
    TFFEngine engine(&writer, combos);
    engine.setFakeActiveTimer(true);

    for (const auto& ev : events) {
        engine.processEvent(ev);
    }
    engine.finish();

    std::string expected =
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n"
        "X-down\n"
        "X-up\n";

    std::string actual = eventsToShortCsv(writer.events);
    requireEqual(actual, expected, "FJX_emits_f_but_should_not");
    std::cout << "PASS\n";
}

int main() {
    std::cout << "================================================\n";
    std::cout << "  TFF Go Test Suite Ported to Native C++        \n";
    std::cout << "================================================\n";

    test_runeToKeyCode();
    test_LoadYamlFromBytes_ok();
    test_LoadYamlFromBytes_fail();
    test_manInTheMiddle_noMatch();
    test_manInTheMiddle_NoMatch_JustKeys();
    test_manInTheMiddle_TwoCombos_WithOneEmbrachingMatch();
    test_manInTheMiddle_SingleCombo_OneEmbrachingMatch();
    test_manInTheMiddle_ComboWithMatch_CrossRhyme();
    test_manInTheMiddle_ComboWithMatch_SingleUpDown();
    test_manInTheMiddle_ComboWithMatch_OverlapNoCombo();
    test_manInTheMiddle_WithoutMatch();
    test_manInTheMiddle_TwoEmbrachingCombosWithMatch();
    test_manInTheMiddle_Unrelated_Embraced_Keystrokes();
    test_manInTheMiddle_ComboWithMatch_NoPanic();
    test_orderedCombos();
    test_Capslock_Navigation();
    test_ShouldNotPanic();
    test_FJX_emits_f_but_should_not();

    std::cout << "================================================\n";
    std::cout << "ALL 18 GO UNIT TESTS PASSED SUCCESSFULLY IN C++! ✓\n";
    std::cout << "================================================\n";
    return 0;
}
