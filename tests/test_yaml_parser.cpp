#include "tff_parser.h"
#include "tff_engine.h"
#include "tff_key_codes.h"
#include <iostream>
#include <fstream>
#include <cassert>

int main() {
    std::cout << "Testing YAML Configuration Parsers\n";
    std::cout << "==================================\n";

    // Test 1: Valid classic TFF YAML string with loadYamlCombos
    {
        std::string yaml_input = R"(
# Test combos
combos:
  - keys: j f
    outKeys: backspace

  - keys: f j
    outKeys: delete

  - keys: semicolon a
    outKeys: home
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(yaml_input, combos, err_msg);
        assert(ok);
        assert(combos.size() == 3);
        assert(combos[0].keys.size() == 2);
        assert(combos[0].out_keys.size() == 1);
        std::cout << "✓ Test 1 passed: parse valid YAML combos\n";
    }

    // Test 2: Unknown key name produces informative error
    {
        std::string invalid_yaml = R"(
combos:
  - keys: unknownkeyxyz f
    outKeys: backspace
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(invalid_yaml, combos, err_msg);
        assert(!ok);
        assert(!err_msg.empty());
        std::cout << "✓ Test 2 passed: rejected unknown key in YAML\n";
    }

    // Test 3: Load repo config file via tff::loadYamlConfig
    {
        std::ifstream file("config/tff-combos.yaml");
        if (!file.is_open()) {
            file.open("../config/tff-combos.yaml");
        }
        assert(file.is_open());
        std::string yaml_content((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
        tff::Config config;
        std::string err_msg;
        bool ok = tff::loadYamlConfig(yaml_content, config, err_msg);
        assert(ok);
        assert(!config.combos.empty());
        assert(!config.tap_hold_keys.empty());
        std::cout << "✓ Test 3 passed: loadYamlConfig loads tff-combos.yaml ("
                  << config.combos.size() << " combos, " << config.tap_hold_keys.size()
                  << " tap-hold keys)\n";
    }

    // Test 4: Compact dictionary syntax with ALL literal punctuation symbols
    {
        std::string compact_yaml = R"(
combos:
  ; a: home
  a ;: end
  f ,: pagedown
  f .: pageup
  f /: slash
  f \: backslash
  f =: equal
  f [: leftbrace
  f ]: rightbrace
  f ': apostrophe
  f `: grave
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(compact_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 11);
        assert(combos[0].keys[0] == 39);   // KEY_SEMICOLON
        assert(combos[4].keys[1] == 53);   // KEY_SLASH
        assert(combos[5].keys[1] == 43);   // KEY_BACKSLASH
        assert(combos[6].keys[1] == 13);   // KEY_EQUAL
        assert(combos[7].keys[1] == 26);   // KEY_LEFTBRACE
        assert(combos[8].keys[1] == 27);   // KEY_RIGHTBRACE
        assert(combos[9].keys[1] == 40);   // KEY_APOSTROPHE
        assert(combos[10].keys[1] == 41);  // KEY_GRAVE
        std::cout << "✓ Test 4 passed: all literal punctuation symbols supported\n";
    }

    // Test 5: Minus key '-' as first and second key (not stripped as bullet)
    {
        std::string minus_yaml = R"(
combos:
  - a: home
  a -: end
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(minus_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 2);
        assert(combos[0].keys[0] == 12);  // KEY_MINUS
        assert(combos[0].keys[1] == 30);  // KEY_A
        assert(combos[1].keys[0] == 30);  // KEY_A
        assert(combos[1].keys[1] == 12);  // KEY_MINUS
        std::cout << "✓ Test 5 passed: minus key '-' correctly parsed as first and second key\n";
    }

    // Test 6: Symmetric combos with '+' (with and without space)
    {
        std::string sym_yaml = R"(
combos:
  g + h: esc
  j+k: enter
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(sym_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 4);
        // c1: g h -> esc
        assert(combos[0].keys[0] == 34);     // KEY_G
        assert(combos[0].keys[1] == 35);     // KEY_H
        assert(combos[0].out_keys[0] == 1);  // KEY_ESC
        // c2: h g -> esc
        assert(combos[1].keys[0] == 35);     // KEY_H
        assert(combos[1].keys[1] == 34);     // KEY_G
        assert(combos[1].out_keys[0] == 1);  // KEY_ESC
        // c3: j k -> enter
        assert(combos[2].keys[0] == 36);      // KEY_J
        assert(combos[2].keys[1] == 37);      // KEY_K
        assert(combos[2].out_keys[0] == 28);  // KEY_ENTER
        // c4: k j -> enter
        assert(combos[3].keys[0] == 37);      // KEY_K
        assert(combos[3].keys[1] == 36);      // KEY_J
        assert(combos[3].out_keys[0] == 28);  // KEY_ENTER
        std::cout << "✓ Test 6 passed: symmetric combos with and without spaces\n";
    }

    // Test 7: Symmetric combos under leader key
    {
        std::string leader_sym_yaml = R"(
combos:
  f:
    g + h: esc
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(leader_sym_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 2);
        // c1: f g h -> esc
        assert(combos[0].keys.size() == 3);
        assert(combos[0].keys[0] == 33);     // KEY_F
        assert(combos[0].keys[1] == 34);     // KEY_G
        assert(combos[0].keys[2] == 35);     // KEY_H
        assert(combos[0].out_keys[0] == 1);  // KEY_ESC
        // c2: f h g -> esc
        assert(combos[1].keys.size() == 3);
        assert(combos[1].keys[0] == 33);     // KEY_F
        assert(combos[1].keys[1] == 35);     // KEY_H
        assert(combos[1].keys[2] == 34);     // KEY_G
        assert(combos[1].out_keys[0] == 1);  // KEY_ESC
        std::cout << "✓ Test 7 passed: symmetric combos nested under leader key\n";
    }

    // Test 8: Leader key grouping and return to base indentation
    {
        std::string leader_yaml = R"(
combos:
  f:
    n: down
    u: up
  j k: backspace
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(leader_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 3);
        // f n -> down
        assert(combos[0].keys.size() == 2);
        assert(combos[0].keys[0] == 33);       // KEY_F
        assert(combos[0].keys[1] == 49);       // KEY_N
        assert(combos[0].out_keys[0] == 108);  // KEY_DOWN
        // j k -> backspace (no leader)
        assert(combos[2].keys.size() == 2);
        assert(combos[2].keys[0] == 36);  // KEY_J
        assert(combos[2].keys[1] == 37);  // KEY_K
        std::cout << "✓ Test 8 passed: leader key grouping and un-indentation\n";
    }

    // Test 9: Symmetric combos with 3+ keys, duplicate detection, and malformed rejection
    {
        std::vector<tff::Combo> combos;
        std::string err_msg;

        // 3-key symmetric combo generates 3! = 6 permutations
        bool ok3 = tff::loadYamlCombos("combos:\n  d + f + j: esc\n", combos, err_msg);
        assert(ok3);
        assert(combos.size() == 6);

        // Classic format with '+' also generates permutations
        combos.clear();
        bool ok_classic = tff::loadYamlCombos("combos:\n  - keys: d + f + j\n    outKeys: esc\n",
                                              combos, err_msg);
        assert(ok_classic);
        assert(combos.size() == 6);

        // 4-key symmetric combo generates 4! = 24 permutations
        combos.clear();
        bool ok4 = tff::loadYamlCombos("combos:\n  a + s + d + f: mute\n", combos, err_msg);
        assert(ok4);
        assert(combos.size() == 24);

        // Duplicate key in symmetric combo is rejected
        combos.clear();
        assert(!tff::loadYamlCombos("combos:\n  d + f + d: esc\n", combos, err_msg));
        assert(err_msg.find("duplicate key") != std::string::npos);

        // Exceeding maximum chord size (5 keys) is rejected
        combos.clear();
        assert(!tff::loadYamlCombos("combos:\n  a + s + d + f + g + h: mute\n", combos, err_msg));
        assert(err_msg.find("exceeds maximum supported chord size") != std::string::npos);

        // Leader key collision with chord key is rejected
        combos.clear();
        assert(!tff::loadYamlCombos("combos:\n  f:\n    f + j: esc\n", combos, err_msg));
        assert(err_msg.find("conflicts with leader key") != std::string::npos);

        // Incomplete combo with '+' is rejected
        combos.clear();
        assert(!tff::loadYamlCombos("combos:\n  g +: esc\n", combos, err_msg));
        assert(err_msg.find("require at least two keys") != std::string::npos);
        std::cout << "✓ Test 9 passed: N-key symmetric combos (3-key, 4-key), max limit, duplicate "
                     "detection, and validation\n";
    }

    // Test 10: Hotkey modifier output syntax (ctrl+s, alt+tab)
    {
        std::string hotkey_yaml = R"(
combos:
  f space: ctrl+s
  j space: ctrl+z
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(hotkey_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 2);
        assert(combos[0].out_keys.size() == 2);
        assert(combos[0].out_keys[0] == 29);  // KEY_LEFTCTRL
        assert(combos[0].out_keys[1] == 31);  // KEY_S
        std::cout << "✓ Test 10 passed: hotkey output syntax (ctrl+s)\n";
    }

    // Test 11: Friendly key name aliases (super, win, del, return, caps, pgup, pgdn)
    {
        std::string alias_yaml = R"(
combos:
  caps a: super
  j k: del
  f d: return
  f u: pgup
  f m: pgdn
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(alias_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 5);
        assert(combos[0].keys[0] == 58);       // KEY_CAPSLOCK
        assert(combos[0].out_keys[0] == 125);  // KEY_LEFTMETA
        assert(combos[1].out_keys[0] == 111);  // KEY_DELETE
        assert(combos[2].out_keys[0] == 28);   // KEY_ENTER
        assert(combos[3].out_keys[0] == 104);  // KEY_PAGEUP
        assert(combos[4].out_keys[0] == 109);  // KEY_PAGEDOWN
        std::cout << "✓ Test 11 passed: friendly key name aliases\n";
    }

    // Test 12: Text snippets and macros in YAML
    {
        std::string snippet_yaml = R"(
combos:
  f n: "println!(\"\");"
  j k: 'single quotes'
  space:
    e: "user@example.com"
  - keys: d f
    text: "macro_test"
  - in: [a, b]
    type: "inline_in"
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(snippet_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 5);
        assert(combos[0].text == "println!(\"\");");
        assert(combos[1].text == "single quotes");
        assert(combos[2].text == "user@example.com");
        assert(combos[3].text == "macro_test");
        assert(combos[4].text == "inline_in");
        std::cout << "✓ Test 12 passed: text snippets and macro expansions\n";
    }

    // Test 13: One-shot modifiers and layers in YAML
    {
        std::string one_shot_yaml = R"(
layers:
  nav:
    h: left
    j: down
    k: up
    l: right

one_shot:
  leftshift: 1500
  leftctrl: 1200
  space: [nav, 2000]
  - key: capslock
    modifier: alt
    timeout_ms: 1000

tap_hold:
  tab:
    tap: osm(shift)
    hold: super
    timeout_ms: 200
  backspace:
    tap: osl(nav)
    hold: alt
    timeout_ms: 250
)";
        tff::Config config;
        std::string err_msg;
        bool ok = tff::loadYamlConfig(one_shot_yaml, config, err_msg);
        assert(ok);
        assert(config.one_shot_keys.size() == 4);
        assert(config.one_shot_keys[0].key == tff::Keys::KEY_LEFTSHIFT);
        assert(config.one_shot_keys[0].modifier == tff::Keys::KEY_LEFTSHIFT);
        assert(config.one_shot_keys[0].timeout_us == 1500000LL);

        assert(config.one_shot_keys[1].key == tff::Keys::KEY_LEFTCTRL);
        assert(config.one_shot_keys[1].modifier == tff::Keys::KEY_LEFTCTRL);

        assert(config.one_shot_keys[2].key == tff::Keys::KEY_SPACE);
        assert(config.one_shot_keys[2].layer == "nav");
        assert(config.one_shot_keys[2].timeout_us == 2000000LL);

        assert(config.one_shot_keys[3].key == tff::Keys::KEY_CAPSLOCK);
        assert(config.one_shot_keys[3].modifier == tff::Keys::KEY_LEFTALT);
        assert(config.one_shot_keys[3].timeout_us == 1000000LL);

        assert(config.tap_hold_keys.size() == 2);
        assert(config.tap_hold_keys[0].tap_one_shot_modifier == tff::Keys::KEY_LEFTSHIFT);
        assert(config.tap_hold_keys[1].tap_one_shot_layer == "nav");

        // Verify explicit layer overrides default modifier when key itself is a modifier
        std::string layer_override_yaml = R"(
layers:
  nav:
    h: left
one_shot:
  leftshift: [nav, 2000]
  - key: rightshift
    layer: nav
)";
        tff::Config cfg_override;
        assert(tff::loadYamlConfig(layer_override_yaml, cfg_override, err_msg));
        assert(cfg_override.one_shot_keys.size() == 2);
        assert(cfg_override.one_shot_keys[0].key == tff::Keys::KEY_LEFTSHIFT);
        assert(cfg_override.one_shot_keys[0].layer == "nav");
        assert(cfg_override.one_shot_keys[0].modifier == 0);
        assert(cfg_override.one_shot_keys[1].key == tff::Keys::KEY_RIGHTSHIFT);
        assert(cfg_override.one_shot_keys[1].layer == "nav");
        assert(cfg_override.one_shot_keys[1].modifier == 0);

        // Validation rejection: non-modifier key in one_shot
        std::string bad_osm_yaml = R"(
one_shot:
  - key: capslock
    modifier: a
    timeout_ms: 1000
)";
        tff::Config cfg_bad_osm;
        assert(!tff::loadYamlConfig(bad_osm_yaml, cfg_bad_osm, err_msg));
        assert(err_msg.find("invalid modifier key") != std::string::npos);

        // Validation rejection: non-modifier key in tap_hold osm(...)
        std::string bad_th_osm_yaml = R"(
tap_hold:
  capslock:
    tap: osm(a)
    hold: super
    timeout_ms: 200
)";
        tff::Config cfg_bad_th;
        assert(!tff::loadYamlConfig(bad_th_osm_yaml, cfg_bad_th, err_msg));
        assert(err_msg.find("invalid modifier key") != std::string::npos);

        std::cout << "✓ Test 13 passed: one-shot modifiers and layers\n";
    }

    // Test 14: Settings section parsing, defaults, and validation
    {
        // 1. Default settings when omitted
        std::string minimal_yaml = R"(
combos:
  j f: backspace
)";
        tff::Config cfg_default;
        std::string err_msg;
        assert(tff::loadYamlConfig(minimal_yaml, cfg_default, err_msg));
        assert(cfg_default.settings.combo_timeout_ms == 40);
        assert(cfg_default.settings.tap_hold_timeout_ms == 200);
        assert(cfg_default.settings.exclusive_grab == true);
        assert(cfg_default.settings.hotplug == true);

        // 2. Custom settings and propagation to tap_hold default timeout
        std::string custom_settings_yaml = R"(
settings:
  combo_timeout_ms: 60
  tap_hold_timeout_ms: 350
  exclusive_grab: false
  hotplug: false

combos:
  j f: backspace

tap_hold:
  capslock:
    tap: esc
    hold: super
  space:
    tap: space
    hold: super
    timeout_ms: 150
)";
        tff::Config cfg_custom;
        assert(tff::loadYamlConfig(custom_settings_yaml, cfg_custom, err_msg));
        assert(cfg_custom.settings.combo_timeout_ms == 60);
        assert(cfg_custom.settings.tap_hold_timeout_ms == 350);
        assert(cfg_custom.settings.exclusive_grab == false);
        assert(cfg_custom.settings.hotplug == false);
        assert(cfg_custom.tap_hold_keys.size() == 2);
        // capslock inherited custom default (350ms)
        assert(cfg_custom.tap_hold_keys[0].timeout_us == 350000LL);
        // space kept its explicit timeout (150ms)
        assert(cfg_custom.tap_hold_keys[1].timeout_us == 150000LL);

        // 3. Settings aliases and alternative boolean syntax (yes/no, on/off, 1/0)
        std::string aliases_yaml = R"(
settings:
  combo_timeout: 45
  tap_hold_timeout: 280
  grab: off
  hotplug: yes
)";
        tff::Config cfg_aliases;
        assert(tff::loadYamlConfig(aliases_yaml, cfg_aliases, err_msg));
        assert(cfg_aliases.settings.combo_timeout_ms == 45);
        assert(cfg_aliases.settings.tap_hold_timeout_ms == 280);
        assert(cfg_aliases.settings.exclusive_grab == false);
        assert(cfg_aliases.settings.hotplug == true);

        // 4. Validation: unknown field
        std::string bad_field_yaml = R"(
settings:
  unknown_opt: 123
)";
        tff::Config cfg_bad_field;
        assert(!tff::loadYamlConfig(bad_field_yaml, cfg_bad_field, err_msg));
        assert(err_msg.find("unknown field 'unknown_opt' in settings section") !=
               std::string::npos);

        // 5. Validation: combo_timeout_ms out of range
        std::string bad_combo_yaml = R"(
settings:
  combo_timeout_ms: 0
)";
        tff::Config cfg_bad_combo;
        assert(!tff::loadYamlConfig(bad_combo_yaml, cfg_bad_combo, err_msg));
        assert(err_msg.find("combo_timeout_ms must be between 1 and 5000") != std::string::npos);

        // 6. Validation: invalid integer
        std::string bad_int_yaml = R"(
settings:
  tap_hold_timeout_ms: invalid_num
)";
        tff::Config cfg_bad_int;
        assert(!tff::loadYamlConfig(bad_int_yaml, cfg_bad_int, err_msg));
        assert(err_msg.find("invalid integer for tap_hold_timeout_ms") != std::string::npos);

        // 7. Validation: invalid boolean
        std::string bad_bool_yaml = R"(
settings:
  exclusive_grab: maybe
)";
        tff::Config cfg_bad_bool;
        assert(!tff::loadYamlConfig(bad_bool_yaml, cfg_bad_bool, err_msg));
        assert(err_msg.find("invalid boolean for exclusive_grab") != std::string::npos);

        // 8. Validation: mapping value error without colon
        std::string no_colon_yaml = R"(
settings
  combo_timeout_ms: 50
)";
        tff::Config cfg_no_colon;
        assert(!tff::loadYamlConfig(no_colon_yaml, cfg_no_colon, err_msg));
        assert(err_msg == "mapping values are not allowed in this context");

        // 9. TFFEngine integration with Settings
        tff::TFFEngine engine;
        engine.setConfig(cfg_custom);
        assert(engine.getSettings().combo_timeout_ms == 60);
        assert(engine.getComboTimeoutMs() == 60);

        engine.setComboTimeoutMs(75);
        assert(engine.getComboTimeoutMs() == 75);
        assert(engine.getSettings().combo_timeout_ms == 75);

        std::cout << "✓ Test 14 passed: settings section parsing, defaults, and validation\n";
    }

    // Test 15: Rejection of removed compact inline tap_hold and tg() shorthand
    {
        tff::Config cfg;
        std::string err_msg;

        // 1. Rejection of compact inline tap_hold
        std::string inline_th_yaml = R"(
tap_hold:
  capslock: [esc, super, 200]
)";
        assert(!tff::loadYamlConfig(inline_th_yaml, cfg, err_msg));
        assert(err_msg.find("compact inline format for tap_hold") != std::string::npos);

        // 2. Rejection of tg() shorthand in combos
        std::string tg_yaml = R"(
combos:
  f + space: tg(nav)
layers:
  nav:
    h: left
)";
        assert(!tff::loadYamlConfig(tg_yaml, cfg, err_msg));
        assert(err_msg.find("shorthand 'tg()' is removed") != std::string::npos);

        std::cout << "✓ Test 15 passed: rejection of compact inline tap_hold and tg() shorthand\n";
    }

    std::cout << "\nAll YAML parser tests passed!\n";
    return 0;
}
