#include "tff_types.h"
#include "tff_key_codes.h"
#include "tff_engine.h"
#include "tff_parser.h"

#include <iostream>
#include <vector>
#include <string>
#include <cassert>

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

static void runAndCheckWithLayout(const std::vector<Event>& input_events,
                                  const std::vector<Combo>& combos, const std::string& layout,
                                  const std::string& expected_output,
                                  const std::string& test_name) {
    VectorWriter writer;
    TFFEngine engine(&writer, combos);
    Settings s;
    s.layout = layout;
    engine.setSettings(s);
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

// 1. Test asciiToKeyStroke mapping for alphanumeric, whitespace, and symbols
void test_ascii_to_keystroke_mapping() {
    std::cout << "[TEST] ASCII to keystroke mapping... ";

    KeyCode code = 0;
    bool shift = false;

    std::string err;
    // Lowercase letters
    for (char c = 'a'; c <= 'z'; ++c) {
        assert(asciiToKeyStroke(c, code, shift));
        assert(!shift);
        KeyCode expected = 0;
        assert(wordToKeyCode(std::string(1, c), expected, err));
        assert(code == expected);
    }

    // Uppercase letters
    for (char c = 'A'; c <= 'Z'; ++c) {
        assert(asciiToKeyStroke(c, code, shift));
        assert(shift);
        KeyCode expected = 0;
        assert(wordToKeyCode(std::string(1, static_cast<char>(std::tolower(c))), expected, err));
        assert(code == expected);
    }

    // Digits
    for (char c = '1'; c <= '9'; ++c) {
        assert(asciiToKeyStroke(c, code, shift));
        assert(!shift);
        assert(code == static_cast<KeyCode>(Keys::KEY_1 + (c - '1')));
    }
    assert(asciiToKeyStroke('0', code, shift));
    assert(!shift);
    assert(code == Keys::KEY_0);

    // Whitespace
    assert(asciiToKeyStroke(' ', code, shift) && !shift && code == Keys::KEY_SPACE);
    assert(asciiToKeyStroke('\t', code, shift) && !shift && code == Keys::KEY_TAB);
    assert(asciiToKeyStroke('\n', code, shift) && !shift && code == Keys::KEY_ENTER);

    // Unshifted punctuation
    assert(asciiToKeyStroke('-', code, shift) && !shift && code == Keys::KEY_MINUS);
    assert(asciiToKeyStroke('=', code, shift) && !shift && code == Keys::KEY_EQUAL);
    assert(asciiToKeyStroke('[', code, shift) && !shift && code == Keys::KEY_LEFTBRACE);
    assert(asciiToKeyStroke(']', code, shift) && !shift && code == Keys::KEY_RIGHTBRACE);
    assert(asciiToKeyStroke('\\', code, shift) && !shift && code == Keys::KEY_BACKSLASH);
    assert(asciiToKeyStroke(';', code, shift) && !shift && code == Keys::KEY_SEMICOLON);
    assert(asciiToKeyStroke('\'', code, shift) && !shift && code == Keys::KEY_APOSTROPHE);
    assert(asciiToKeyStroke('`', code, shift) && !shift && code == Keys::KEY_GRAVE);
    assert(asciiToKeyStroke(',', code, shift) && !shift && code == Keys::KEY_COMMA);
    assert(asciiToKeyStroke('.', code, shift) && !shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke('/', code, shift) && !shift && code == Keys::KEY_SLASH);

    // Shifted punctuation
    assert(asciiToKeyStroke('!', code, shift) && shift && code == Keys::KEY_1);
    assert(asciiToKeyStroke('@', code, shift) && shift && code == Keys::KEY_2);
    assert(asciiToKeyStroke('#', code, shift) && shift && code == Keys::KEY_3);
    assert(asciiToKeyStroke('$', code, shift) && shift && code == Keys::KEY_4);
    assert(asciiToKeyStroke('%', code, shift) && shift && code == Keys::KEY_5);
    assert(asciiToKeyStroke('^', code, shift) && shift && code == Keys::KEY_6);
    assert(asciiToKeyStroke('&', code, shift) && shift && code == Keys::KEY_7);
    assert(asciiToKeyStroke('*', code, shift) && shift && code == Keys::KEY_8);
    assert(asciiToKeyStroke('(', code, shift) && shift && code == Keys::KEY_9);
    assert(asciiToKeyStroke(')', code, shift) && shift && code == Keys::KEY_0);
    assert(asciiToKeyStroke('_', code, shift) && shift && code == Keys::KEY_MINUS);
    assert(asciiToKeyStroke('+', code, shift) && shift && code == Keys::KEY_EQUAL);
    assert(asciiToKeyStroke('{', code, shift) && shift && code == Keys::KEY_LEFTBRACE);
    assert(asciiToKeyStroke('}', code, shift) && shift && code == Keys::KEY_RIGHTBRACE);
    assert(asciiToKeyStroke('|', code, shift) && shift && code == Keys::KEY_BACKSLASH);
    assert(asciiToKeyStroke(':', code, shift) && shift && code == Keys::KEY_SEMICOLON);
    assert(asciiToKeyStroke('"', code, shift) && shift && code == Keys::KEY_APOSTROPHE);
    assert(asciiToKeyStroke('~', code, shift) && shift && code == Keys::KEY_GRAVE);
    assert(asciiToKeyStroke('<', code, shift) && shift && code == Keys::KEY_COMMA);
    assert(asciiToKeyStroke('>', code, shift) && shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke('?', code, shift) && shift && code == Keys::KEY_SLASH);

    // Unsupported characters
    assert(!asciiToKeyStroke('\0', code, shift));
    assert(!asciiToKeyStroke('\x01', code, shift));
    assert(!asciiToKeyStroke('\x7f', code, shift));

    std::cout << "OK\n";
}

void test_ascii_to_keystroke_mapping_de() {
    std::cout << "[TEST] German (de) layout keystroke mapping... ";
    KeyCode code = 0;
    bool shift = false;

    // y <-> z swap
    assert(asciiToKeyStroke('y', code, shift, "de") && !shift && code == Keys::KEY_Z);
    assert(asciiToKeyStroke('Y', code, shift, "de") && shift && code == Keys::KEY_Z);
    assert(asciiToKeyStroke('z', code, shift, "de") && !shift && code == Keys::KEY_Y);
    assert(asciiToKeyStroke('Z', code, shift, "de") && shift && code == Keys::KEY_Y);

    // Diagnostic dump punctuation
    assert(asciiToKeyStroke(':', code, shift, "de") && shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke('_', code, shift, "de") && shift && code == Keys::KEY_SLASH);
    assert(asciiToKeyStroke('/', code, shift, "de") && shift && code == Keys::KEY_7);
    assert(asciiToKeyStroke('(', code, shift, "de") && shift && code == Keys::KEY_8);
    assert(asciiToKeyStroke(')', code, shift, "de") && shift && code == Keys::KEY_9);
    assert(asciiToKeyStroke('-', code, shift, "de") && !shift && code == Keys::KEY_SLASH);
    assert(asciiToKeyStroke('=', code, shift, "de") && shift && code == Keys::KEY_0);
    assert(asciiToKeyStroke('?', code, shift, "de") && shift && code == Keys::KEY_MINUS);
    assert(asciiToKeyStroke('+', code, shift, "de") && !shift && code == Keys::KEY_RIGHTBRACE);
    assert(asciiToKeyStroke('*', code, shift, "de") && shift && code == Keys::KEY_RIGHTBRACE);
    assert(asciiToKeyStroke(';', code, shift, "de") && shift && code == Keys::KEY_COMMA);
    assert(asciiToKeyStroke('<', code, shift, "de") && !shift && code == Keys::KEY_102ND);
    assert(asciiToKeyStroke('>', code, shift, "de") && shift && code == Keys::KEY_102ND);

    // Full diagnostic dump string translation check
    std::string dump = "IN: semicolon_ a_ (250ms) a/ semicolon/\nOUT: home_ home/\n";
    for (char c : dump) {
        assert(asciiToKeyStroke(c, code, shift, "de"));
    }

    std::cout << "OK\n";
}

void test_ascii_to_keystroke_mapping_fr() {
    std::cout << "[TEST] French (fr) AZERTY layout keystroke mapping... ";
    KeyCode code = 0;
    bool shift = false;

    // AZERTY letters
    assert(asciiToKeyStroke('a', code, shift, "fr") && !shift && code == Keys::KEY_Q);
    assert(asciiToKeyStroke('q', code, shift, "fr") && !shift && code == Keys::KEY_A);
    assert(asciiToKeyStroke('z', code, shift, "fr") && !shift && code == Keys::KEY_W);
    assert(asciiToKeyStroke('w', code, shift, "fr") && !shift && code == Keys::KEY_Z);
    assert(asciiToKeyStroke('m', code, shift, "fr") && !shift && code == Keys::KEY_SEMICOLON);

    // Digits require Shift in AZERTY
    for (char c = '1'; c <= '9'; ++c) {
        assert(asciiToKeyStroke(c, code, shift, "fr") && shift &&
               code == static_cast<KeyCode>(Keys::KEY_1 + (c - '1')));
    }
    assert(asciiToKeyStroke('0', code, shift, "fr") && shift && code == Keys::KEY_0);

    // Punctuation
    assert(asciiToKeyStroke(':', code, shift, "fr") && !shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke(';', code, shift, "fr") && !shift && code == Keys::KEY_COMMA);
    assert(asciiToKeyStroke(',', code, shift, "fr") && !shift && code == Keys::KEY_M);
    assert(asciiToKeyStroke('/', code, shift, "fr") && shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke('(', code, shift, "fr") && !shift && code == Keys::KEY_5);
    assert(asciiToKeyStroke(')', code, shift, "fr") && !shift && code == Keys::KEY_MINUS);
    assert(asciiToKeyStroke('_', code, shift, "fr") && !shift && code == Keys::KEY_8);

    // Full diagnostic dump string translation check
    std::string dump = "IN: semicolon_ a_ (250ms) a/ semicolon/\nOUT: home_ home/\n";
    for (char c : dump) {
        assert(asciiToKeyStroke(c, code, shift, "fr"));
    }

    std::cout << "OK\n";
}

void test_ascii_to_keystroke_mapping_gb() {
    std::cout << "[TEST] UK English (gb/uk) layout keystroke mapping... ";
    KeyCode code = 0;
    bool shift = false;

    assert(asciiToKeyStroke('"', code, shift, "gb") && shift && code == Keys::KEY_2);
    assert(asciiToKeyStroke('@', code, shift, "gb") && shift && code == Keys::KEY_APOSTROPHE);
    assert(asciiToKeyStroke('#', code, shift, "gb") && !shift && code == Keys::KEY_BACKSLASH);
    assert(asciiToKeyStroke('~', code, shift, "gb") && shift && code == Keys::KEY_BACKSLASH);
    assert(asciiToKeyStroke('\\', code, shift, "gb") && !shift && code == Keys::KEY_102ND);
    assert(asciiToKeyStroke('|', code, shift, "gb") && shift && code == Keys::KEY_102ND);

    // Normalization check ("uk" alias for "gb")
    assert(asciiToKeyStroke('"', code, shift, "uk") && shift && code == Keys::KEY_2);

    std::string dump = "IN: semicolon_ a_ (250ms) a/ semicolon/\nOUT: home_ home/\n";
    for (char c : dump) {
        assert(asciiToKeyStroke(c, code, shift, "gb"));
    }

    std::cout << "OK\n";
}

void test_ascii_to_keystroke_mapping_other_layouts() {
    std::cout << "[TEST] Spanish, Italian, Dvorak, Colemak, Neo layouts... ";
    KeyCode code = 0;
    bool shift = false;

    // Spanish / Italian
    assert(asciiToKeyStroke(':', code, shift, "es") && shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke(':', code, shift, "it") && shift && code == Keys::KEY_DOT);
    assert(asciiToKeyStroke('_', code, shift, "es") && shift && code == Keys::KEY_SLASH);
    assert(asciiToKeyStroke('/', code, shift, "it") && shift && code == Keys::KEY_7);

    // Dvorak
    assert(asciiToKeyStroke('c', code, shift, "dvorak") && !shift && code == Keys::KEY_I);
    assert(asciiToKeyStroke(':', code, shift, "dvorak") && shift && code == Keys::KEY_Z);
    assert(asciiToKeyStroke('_', code, shift, "dvorak") && shift && code == Keys::KEY_APOSTROPHE);

    // Colemak
    assert(asciiToKeyStroke('f', code, shift, "colemak") && !shift && code == Keys::KEY_E);
    assert(asciiToKeyStroke(';', code, shift, "colemak") && !shift && code == Keys::KEY_P);
    assert(asciiToKeyStroke(':', code, shift, "colemak") && shift && code == Keys::KEY_P);

    // Neo
    assert(asciiToKeyStroke('a', code, shift, "neo") && !shift && code == Keys::KEY_D);
    assert(asciiToKeyStroke('e', code, shift, "neo") && !shift && code == Keys::KEY_F);

    // Neo has the symbols on level 3 (the Mod3 key), not on Shift of the QWERTZ digits
    CharKeyStrokes strokes;
    assert(charToKeyStrokes('(', "neo", strokes) && strokes.count == 1 &&
           strokes.strokes[0].level3 && strokes.strokes[0].code == Keys::KEY_J);
    assert(layoutLevel3Key("neo") == Keys::KEY_BACKSLASH);

    std::string dump = "IN: semicolon_ a_ (250ms) a/ semicolon/\nOUT: home_ home/\n";
    for (const char* lay : {"es", "it", "dvorak", "colemak", "neo"}) {
        for (char c : dump) {
            assert(charToKeyStrokes(static_cast<unsigned char>(c), lay, strokes));
        }
    }

    std::cout << "OK\n";
}

void test_engine_emission_german_layout() {
    std::cout << "[TEST] Engine text emission with German (de) layout... ";

    std::string yaml = "combos:\n  f n: \"y:\"\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 70000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 80000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
    };

    // In German layout:
    // 'y' -> KEY_Z
    // ':' -> LEFTSHIFT + KEY_DOT
    std::string expected =
        "Z-down\n"
        "Z-up\n"
        "LEFTSHIFT-down\n"
        "DOT-down\n"
        "DOT-up\n"
        "LEFTSHIFT-up\n";

    runAndCheckWithLayout(inputs, combos, "de", expected, "test_engine_emission_german_layout");
    std::cout << "OK\n";
}

// Characters beyond one key + Shift: AltGr, dead keys, umlauts (tables generated from xkb)
void test_char_to_keystrokes() {
    std::cout << "[TEST] charToKeyStrokes (AltGr, dead keys, umlauts)... ";
    CharKeyStrokes s;
    // de: @ is AltGr+Q, ü has its own key, ^ is the dead ^ key followed by space
    assert(charToKeyStrokes('@', "de", s) && s.count == 1 && s.strokes[0].level3 &&
           !s.strokes[0].shift && s.strokes[0].code == Keys::KEY_Q);
    assert(layoutLevel3Key("de") == Keys::KEY_RIGHTALT);
    assert(charToKeyStrokes(0xFC, "de", s) && s.count == 1 && !s.strokes[0].level3 &&
           s.strokes[0].code == Keys::KEY_LEFTBRACE);
    assert(charToKeyStrokes(0xDC, "de", s) && s.strokes[0].shift);  // Ü
    assert(charToKeyStrokes('^', "de", s) && s.count == 2 && s.strokes[0].code == Keys::KEY_GRAVE &&
           s.strokes[1].code == Keys::KEY_SPACE);
    assert(charToKeyStrokes(0x20AC, "de", s) && s.strokes[0].level3 &&
           s.strokes[0].code == Keys::KEY_E);  // €
    // fr: AZERTY letters (b, c, n were on wrong keys in the hand-written table), é on 2
    assert(charToKeyStrokes('b', "fr", s) && s.strokes[0].code == Keys::KEY_B);
    assert(charToKeyStrokes('n', "fr", s) && s.strokes[0].code == Keys::KEY_N);
    assert(charToKeyStrokes(0xE9, "fr", s) && s.count == 1 && s.strokes[0].code == Keys::KEY_2);
    // es/it: AltGr characters on the keys printed on the keycaps (as on Windows), not on the
    // AltGr+digit keys that only xkb has
    assert(charToKeyStrokes('[', "es", s) && s.strokes[0].level3 &&
           s.strokes[0].code == Keys::KEY_LEFTBRACE);
    assert(charToKeyStrokes('{', "it", s) && s.strokes[0].level3 && s.strokes[0].shift &&
           s.strokes[0].code == Keys::KEY_LEFTBRACE);
    // us: no umlauts
    assert(!charToKeyStrokes(0xFC, "us", s));
    assert(charToKeyStrokes('\n', "neo", s) && s.strokes[0].code == Keys::KEY_ENTER);

    // UTF-8 decoding
    size_t pos = 0;
    uint32_t cp = 0;
    std::string text = "a\xC3\xBC\xE2\x82\xAC\xF0\x9F\x98\x80";  // a ü € 😀
    assert(decodeUtf8(text, pos, cp) && cp == 'a' && pos == 1);
    assert(decodeUtf8(text, pos, cp) && cp == 0xFC && pos == 3);
    assert(decodeUtf8(text, pos, cp) && cp == 0x20AC && pos == 6);
    assert(decodeUtf8(text, pos, cp) && cp == 0x1F600 && pos == 10);
    for (const char* bad : {"\xC3", "\xC3\x28", "\xC0\xAF", "\xED\xA0\x80", "\xFF"}) {
        pos = 0;
        assert(!decodeUtf8(bad, pos, cp) && pos == 0);
    }
    std::cout << "OK\n";
}

void test_engine_emission_altgr_and_dead_keys() {
    std::cout << "[TEST] Engine text emission with AltGr and dead keys (de)... ";
    Config cfg;
    std::string err;
    assert(loadYamlConfig("combos:\n  f n: \"\xC3\xBC@^\"\nsettings:\n  layout: de\n", cfg, err));
    const std::vector<Combo>& combos = cfg.combos;
    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 70000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 80000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
    };
    std::string expected =
        "LEFTBRACE-down\n"
        "LEFTBRACE-up\n"
        "RIGHTALT-down\n"
        "Q-down\n"
        "Q-up\n"
        "RIGHTALT-up\n"
        "GRAVE-down\n"
        "GRAVE-up\n"
        "SPACE-down\n"
        "SPACE-up\n";
    runAndCheckWithLayout(inputs, combos, "de", expected,
                          "test_engine_emission_altgr_and_dead_keys");
    std::cout << "OK\n";
}

void test_engine_emission_with_shift_held() {
    std::cout << "[TEST] Held Shift and AltGr don't change the snippet text... ";
    Config cfg;
    std::string err;
    assert(loadYamlConfig("combos:\n  f n: \"a@\"\nsettings:\n  layout: de\n", cfg, err));
    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 5000), EV_KEY, Keys::KEY_RIGHTALT, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 20000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 80000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 90000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 100000), EV_KEY, Keys::KEY_RIGHTALT, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 110000), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP},
    };
    std::string expected =
        "LEFTSHIFT-down\n"
        "RIGHTALT-down\n"
        "LEFTSHIFT-up\n"
        "RIGHTALT-up\n"
        "A-down\n"
        "A-up\n"
        "RIGHTALT-down\n"
        "Q-down\n"
        "Q-up\n"
        "RIGHTALT-up\n"
        "LEFTSHIFT-down\n"
        "RIGHTALT-down\n"
        "RIGHTALT-up\n"
        "LEFTSHIFT-up\n";
    runAndCheckWithLayout(inputs, cfg.combos, "de", expected,
                          "test_engine_emission_with_shift_held");

    // On us, Right Alt is plain Alt and stays down (no lone Alt tap)
    assert(loadYamlConfig("combos:\n  f n: \"a\"\n", cfg, err));
    runAndCheckWithLayout(inputs, cfg.combos, "us",
                          "LEFTSHIFT-down\n"
                          "RIGHTALT-down\n"
                          "LEFTSHIFT-up\n"
                          "A-down\n"
                          "A-up\n"
                          "LEFTSHIFT-down\n"
                          "RIGHTALT-up\n"
                          "LEFTSHIFT-up\n",
                          "test_engine_emission_with_shift_held us");
    assert(layoutCharModifierKeys("dvorak") ==
           std::vector<KeyCode>({Keys::KEY_LEFTSHIFT, Keys::KEY_RIGHTSHIFT}));
    assert(layoutCharModifierKeys("gb").back() == Keys::KEY_RIGHTALT);
    assert(layoutCharModifierKeys("neo") ==
           std::vector<KeyCode>({Keys::KEY_LEFTSHIFT, Keys::KEY_RIGHTSHIFT, Keys::KEY_CAPSLOCK,
                                 Keys::KEY_BACKSLASH, Keys::KEY_102ND, Keys::KEY_RIGHTALT}));
    std::cout << "OK\n";
}

void test_parser_snippet_layout_check() {
    std::cout << "[TEST] Parser checks snippets against settings.layout... ";
    Config cfg;
    std::string err;
    // settings may come after the snippets
    assert(
        loadYamlConfig("combos:\n  f n: \"Gr\xC3\xBC\xC3\x9F\xE2\x82\xAC @\"\n"
                       "settings:\n  layout: de\n",
                       cfg, err));
    // not on the layout
    assert(!loadYamlConfig("combos:\n  f n: \"\xC3\xB1\"\nsettings:\n  layout: de\n", cfg, err));
    assert(err.find("'\xC3\xB1' (not on keyboard layout \"de\"") != std::string::npos);
    // tap dance and layer snippets are checked too
    assert(!loadYamlConfig("tap_dance:\n  q:\n    double_tap: text \"\xC3\xBC\"\n", cfg, err));
    assert(err.find("tap_dance text snippet") != std::string::npos);
    assert(!loadYamlConfig("layers:\n  nav:\n    h: \"\xC3\xBC\"\n", cfg, err));
    // Windows types text as characters: anything goes, but not invalid UTF-8
    LoadOptions windows;
    windows.text_any_unicode = true;
    assert(loadYamlConfig("combos:\n  f n: \"\xC3\xB1 \xF0\x9F\x98\x80\"\n", cfg, err, windows));
    assert(!loadYamlConfig("combos:\n  f n: \"\xC3\x28\"\n", cfg, err, windows));
    assert(err.find("invalid UTF-8") != std::string::npos);
    std::cout << "OK\n";
}

// 2. Test YAML parsing of compact syntax and inline mappings
void test_yaml_parser_compact_syntax() {
    std::cout << "[TEST] YAML parser compact syntax snippets... ";

    std::string yaml =
        "combos:\n"
        "  f n: \"println!(\\\"\\\");\"\n"
        "  j k: 'Hello World'\n"
        "  a b: { text: \"import sys\" }\n"
        "  c d: { type: \"cargo test\" }\n"
        "  c m: \"#include <iostream>\" # C++ include comment\n";

    Config cfg;
    std::string err;
    assert(loadYamlConfig(yaml, cfg, err));
    assert(cfg.combos.size() == 5);

    assert(cfg.combos[0].keys == std::vector<KeyCode>({Keys::KEY_F, Keys::KEY_N}));
    assert(cfg.combos[0].text == "println!(\"\");");
    assert(cfg.combos[0].out_keys.empty());

    assert(cfg.combos[1].keys == std::vector<KeyCode>({Keys::KEY_J, Keys::KEY_K}));
    assert(cfg.combos[1].text == "Hello World");

    assert(cfg.combos[2].keys == std::vector<KeyCode>({Keys::KEY_A, Keys::KEY_B}));
    assert(cfg.combos[2].text == "import sys");

    assert(cfg.combos[3].keys == std::vector<KeyCode>({Keys::KEY_C, Keys::KEY_D}));
    assert(cfg.combos[3].text == "cargo test");

    assert(cfg.combos[4].keys == std::vector<KeyCode>({Keys::KEY_C, Keys::KEY_M}));
    assert(cfg.combos[4].text == "#include <iostream>");

    std::cout << "OK\n";
}

// 3. Test YAML parsing with symmetric combos (+) and leader keys
void test_yaml_parser_symmetric_and_leader() {
    std::cout << "[TEST] YAML parser symmetric combos and leader keys with snippets... ";

    std::string yaml =
        "combos:\n"
        "  f + n: \"status\"\n"
        "  space:\n"
        "    e: \"alice@example.com\"\n"
        "    g: \"git status\"\n";

    Config cfg;
    std::string err;
    assert(loadYamlConfig(yaml, cfg, err));
    assert(cfg.combos.size() == 4);  // 2 symmetric permutations + 2 leader combos

    // Symmetric f + n
    assert(cfg.combos[0].keys == std::vector<KeyCode>({Keys::KEY_F, Keys::KEY_N}));
    assert(cfg.combos[0].text == "status");
    assert(cfg.combos[1].keys == std::vector<KeyCode>({Keys::KEY_N, Keys::KEY_F}));
    assert(cfg.combos[1].text == "status");

    // Leader key space: e and g
    assert(cfg.combos[2].keys == std::vector<KeyCode>({Keys::KEY_SPACE, Keys::KEY_E}));
    assert(cfg.combos[2].text == "alice@example.com");
    assert(cfg.combos[3].keys == std::vector<KeyCode>({Keys::KEY_SPACE, Keys::KEY_G}));
    assert(cfg.combos[3].text == "git status");

    std::cout << "OK\n";
}

// 4. Test YAML parser classic verbose format with text and type
void test_yaml_parser_classic_syntax() {
    std::cout << "[TEST] YAML parser classic syntax snippets... ";

    std::string yaml =
        "combos:\n"
        "  - keys: f n\n"
        "    text: \"return 0;\\n\"\n"
        "  - in: [j, k]\n"
        "    type: \"foo::bar\"\n"
        "  - in: [d, f]\n"
        "    out: \"hello\"\n";

    Config cfg;
    std::string err;
    assert(loadYamlConfig(yaml, cfg, err));
    assert(cfg.combos.size() == 3);

    assert(cfg.combos[0].keys == std::vector<KeyCode>({Keys::KEY_F, Keys::KEY_N}));
    assert(cfg.combos[0].text == "return 0;\n");

    assert(cfg.combos[1].keys == std::vector<KeyCode>({Keys::KEY_J, Keys::KEY_K}));
    assert(cfg.combos[1].text == "foo::bar");

    assert(cfg.combos[2].keys == std::vector<KeyCode>({Keys::KEY_D, Keys::KEY_F}));
    assert(cfg.combos[2].text == "hello");

    std::cout << "OK\n";
}

// 5. Test parser validation errors
void test_parser_validation_errors() {
    std::cout << "[TEST] Parser validation errors for snippets... ";

    Config cfg;
    std::string err;

    // Empty text snippet
    assert(!loadYamlConfig("combos:\n  f n: \"\"\n", cfg, err));
    assert(err.find("empty text snippet") != std::string::npos);

    // Unsupported non-ASCII character (accented e)
    assert(!loadYamlConfig("combos:\n  f n: \"caf\xC3\xA9\"\n", cfg, err));
    assert(err.find("unsupported character in text snippet: '\xC3\xA9' (not on keyboard layout "
                    "\"us\"") != std::string::npos);

    // ... also in a leader sequence (the error used to show a broken single byte)
    assert(!loadYamlConfig(
        "leader:\n  key: rightctrl\n  sequences:\n    g g: text \"Gr\xC3\xBC\xC3\x9F"
        "e\"\n",
        cfg, err));
    assert(err.find("unsupported character in leader text snippet: '\xC3\xBC'") !=
           std::string::npos);

    // Classic format empty text
    assert(!loadYamlConfig("combos:\n  - keys: f n\n    text: \"\"\n", cfg, err));
    assert(err.find("empty text snippet") != std::string::npos);

    std::cout << "OK\n";
}

// 6. Test engine emission for lowercase text snippet
void test_engine_emission_lowercase() {
    std::cout << "[TEST] Engine emission for lowercase text snippet... ";

    std::string yaml = "combos:\n  f n: \"cat\"\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        // After min_age_us (50ms):
        {TimeVal::fromMicros(t + 70000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 80000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
    };

    std::string expected =
        "C-down\n"
        "C-up\n"
        "A-down\n"
        "A-up\n"
        "T-down\n"
        "T-up\n";

    runAndCheck(inputs, combos, expected, "test_engine_emission_lowercase");
    std::cout << "OK\n";
}

// 7. Test engine emission for shifted symbols and newlines
void test_engine_emission_shifted_and_multiline() {
    std::cout << "[TEST] Engine emission for shifted symbols and newlines... ";

    std::string yaml = "combos:\n  f n: \"Hi!\\n\"\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 70000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 80000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
    };

    std::string expected =
        "LEFTSHIFT-down\n"
        "H-down\n"
        "H-up\n"
        "LEFTSHIFT-up\n"
        "I-down\n"
        "I-up\n"
        "LEFTSHIFT-down\n"
        "1-down\n"
        "1-up\n"
        "LEFTSHIFT-up\n"
        "ENTER-down\n"
        "ENTER-up\n";

    runAndCheck(inputs, combos, expected, "test_engine_emission_shifted_and_multiline");
    std::cout << "OK\n";
}

// 8. Test fast chord tap triggering text snippet
void test_engine_fast_chord_tap() {
    std::cout << "[TEST] Engine fast chord tap for text snippet... ";

    std::string yaml = "combos:\n  f n: \"ok\"\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    // Rapid chord tap (< min_age_us)
    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 5000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 15000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
    };

    std::string expected =
        "O-down\n"
        "O-up\n"
        "K-down\n"
        "K-up\n";

    runAndCheck(inputs, combos, expected, "test_engine_fast_chord_tap");
    std::cout << "OK\n";
}

// 9. Test code macro with quotes and punctuation: println!("");
void test_engine_code_macro() {
    std::cout << "[TEST] Engine code macro println!(\"\");... ";

    std::string yaml = "combos:\n  f n: \"println!(\\\"\\\");\"\n";
    std::vector<Combo> combos;
    std::string err;
    assert(loadYamlCombos(yaml, combos, err));

    int64_t t = 1000000;
    std::vector<Event> inputs = {
        {TimeVal::fromMicros(t), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 10000), EV_KEY, Keys::KEY_N, KEY_VAL_DOWN},
        {TimeVal::fromMicros(t + 70000), EV_KEY, Keys::KEY_F, KEY_VAL_UP},
        {TimeVal::fromMicros(t + 80000), EV_KEY, Keys::KEY_N, KEY_VAL_UP},
    };

    // "println!(\"\");" breakdown:
    // p r i n t l n
    // ! (LEFTSHIFT + 1)
    // ( (LEFTSHIFT + 9)
    // " (LEFTSHIFT + APOSTROPHE)
    // " (LEFTSHIFT + APOSTROPHE)
    // ) (LEFTSHIFT + 0)
    // ; (SEMICOLON)
    std::string expected =
        "P-down\n"
        "P-up\n"
        "R-down\n"
        "R-up\n"
        "I-down\n"
        "I-up\n"
        "N-down\n"
        "N-up\n"
        "T-down\n"
        "T-up\n"
        "L-down\n"
        "L-up\n"
        "N-down\n"
        "N-up\n"
        "LEFTSHIFT-down\n"
        "1-down\n"
        "1-up\n"
        "LEFTSHIFT-up\n"
        "LEFTSHIFT-down\n"
        "9-down\n"
        "9-up\n"
        "LEFTSHIFT-up\n"
        "LEFTSHIFT-down\n"
        "APOSTROPHE-down\n"
        "APOSTROPHE-up\n"
        "LEFTSHIFT-up\n"
        "LEFTSHIFT-down\n"
        "APOSTROPHE-down\n"
        "APOSTROPHE-up\n"
        "LEFTSHIFT-up\n"
        "LEFTSHIFT-down\n"
        "0-down\n"
        "0-up\n"
        "LEFTSHIFT-up\n"
        "SEMICOLON-down\n"
        "SEMICOLON-up\n";

    runAndCheck(inputs, combos, expected, "test_engine_code_macro");
    std::cout << "OK\n";
}

int main() {
    std::cout << "Running text snippet tests...\n";

    test_ascii_to_keystroke_mapping();
    test_ascii_to_keystroke_mapping_de();
    test_ascii_to_keystroke_mapping_fr();
    test_ascii_to_keystroke_mapping_gb();
    test_ascii_to_keystroke_mapping_other_layouts();
    test_engine_emission_german_layout();
    test_char_to_keystrokes();
    test_engine_emission_altgr_and_dead_keys();
    test_engine_emission_with_shift_held();
    test_parser_snippet_layout_check();
    test_yaml_parser_compact_syntax();
    test_yaml_parser_symmetric_and_leader();
    test_yaml_parser_classic_syntax();
    test_parser_validation_errors();
    test_engine_emission_lowercase();
    test_engine_emission_shifted_and_multiline();
    test_engine_fast_chord_tap();
    test_engine_code_macro();

    std::cout << "All text snippet tests PASSED!\n";
    return 0;
}
