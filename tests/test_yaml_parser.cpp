#include "tff_parser.h"
#include "key_mapper.h"
#include "tff_key_codes.h"
#include <iostream>
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

    // Test 3: KeyMapper::loadConfiguration on repo config
    {
        KeyMapper mapper;
        bool ok = mapper.loadConfiguration("config/tff-combos.yaml");
        if (!ok) {
            ok = mapper.loadConfiguration("../config/tff-combos.yaml");
        }
        assert(ok);
        assert(mapper.getMappingCount() > 0);
        std::cout << "✓ Test 3 passed: KeyMapper loads tff-combos.yaml (" 
                  << mapper.getMappingCount() << " mappings)\n";
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
        assert(combos[0].keys[0] == 39); // KEY_SEMICOLON
        assert(combos[4].keys[1] == 53); // KEY_SLASH
        assert(combos[5].keys[1] == 43); // KEY_BACKSLASH
        assert(combos[6].keys[1] == 13); // KEY_EQUAL
        assert(combos[7].keys[1] == 26); // KEY_LEFTBRACE
        assert(combos[8].keys[1] == 27); // KEY_RIGHTBRACE
        assert(combos[9].keys[1] == 40); // KEY_APOSTROPHE
        assert(combos[10].keys[1] == 41); // KEY_GRAVE
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
        assert(combos[0].keys[0] == 12); // KEY_MINUS
        assert(combos[0].keys[1] == 30); // KEY_A
        assert(combos[1].keys[0] == 30); // KEY_A
        assert(combos[1].keys[1] == 12); // KEY_MINUS
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
        assert(combos[0].keys[0] == 34); // KEY_G
        assert(combos[0].keys[1] == 35); // KEY_H
        assert(combos[0].out_keys[0] == 1); // KEY_ESC
        // c2: h g -> esc
        assert(combos[1].keys[0] == 35); // KEY_H
        assert(combos[1].keys[1] == 34); // KEY_G
        assert(combos[1].out_keys[0] == 1); // KEY_ESC
        // c3: j k -> enter
        assert(combos[2].keys[0] == 36); // KEY_J
        assert(combos[2].keys[1] == 37); // KEY_K
        assert(combos[2].out_keys[0] == 28); // KEY_ENTER
        // c4: k j -> enter
        assert(combos[3].keys[0] == 37); // KEY_K
        assert(combos[3].keys[1] == 36); // KEY_J
        assert(combos[3].out_keys[0] == 28); // KEY_ENTER
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
        assert(combos[0].keys[0] == 33); // KEY_F
        assert(combos[0].keys[1] == 34); // KEY_G
        assert(combos[0].keys[2] == 35); // KEY_H
        assert(combos[0].out_keys[0] == 1); // KEY_ESC
        // c2: f h g -> esc
        assert(combos[1].keys.size() == 3);
        assert(combos[1].keys[0] == 33); // KEY_F
        assert(combos[1].keys[1] == 35); // KEY_H
        assert(combos[1].keys[2] == 34); // KEY_G
        assert(combos[1].out_keys[0] == 1); // KEY_ESC
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
        assert(combos[0].keys[0] == 33); // KEY_F
        assert(combos[0].keys[1] == 49); // KEY_N
        assert(combos[0].out_keys[0] == 108); // KEY_DOWN
        // j k -> backspace (no leader)
        assert(combos[2].keys.size() == 2);
        assert(combos[2].keys[0] == 36); // KEY_J
        assert(combos[2].keys[1] == 37); // KEY_K
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
        bool ok_classic = tff::loadYamlCombos("combos:\n  - keys: d + f + j\n    outKeys: esc\n", combos, err_msg);
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

        // Incomplete combo with '+' is rejected
        combos.clear();
        assert(!tff::loadYamlCombos("combos:\n  g +: esc\n", combos, err_msg));
        assert(err_msg.find("require at least two keys") != std::string::npos);
        std::cout << "✓ Test 9 passed: N-key symmetric combos (3-key, 4-key), duplicate detection, and validation\n";
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
        assert(combos[0].out_keys[0] == 29); // KEY_LEFTCTRL
        assert(combos[0].out_keys[1] == 31); // KEY_S
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
        assert(combos[0].keys[0] == 58); // KEY_CAPSLOCK
        assert(combos[0].out_keys[0] == 125); // KEY_LEFTMETA
        assert(combos[1].out_keys[0] == 111); // KEY_DELETE
        assert(combos[2].out_keys[0] == 28); // KEY_ENTER
        assert(combos[3].out_keys[0] == 104); // KEY_PAGEUP
        assert(combos[4].out_keys[0] == 109); // KEY_PAGEDOWN
        std::cout << "✓ Test 11 passed: friendly key name aliases\n";
    }

    std::cout << "\nAll YAML parser tests passed!\n";
    return 0;
}
