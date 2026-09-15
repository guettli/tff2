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

    // Test 4: Compact dictionary syntax with literal symbols (; , . / - =)
    {
        std::string compact_yaml = R"(
combos:
  j f: backspace # inline comment
  f j: delete
  ; a: home
  a ;: end
  f ,: pagedown
  f .: pageup
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(compact_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 6);
        // ; a -> home
        assert(combos[2].keys[0] == 39); // KEY_SEMICOLON
        assert(combos[2].keys[1] == 30); // KEY_A
        assert(combos[2].out_keys[0] == 102); // KEY_HOME
        std::cout << "✓ Test 4 passed: compact dictionary syntax with literal symbols\n";
    }

    // Test 5: Symmetric combos with "+" generates both permutations
    {
        std::string sym_yaml = R"(
combos:
  g + h: esc
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(sym_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 2);
        // c1: g h -> esc
        assert(combos[0].keys[0] == 34); // KEY_G
        assert(combos[0].keys[1] == 35); // KEY_H
        assert(combos[0].out_keys[0] == 1); // KEY_ESC
        // c2: h g -> esc
        assert(combos[1].keys[0] == 35); // KEY_H
        assert(combos[1].keys[1] == 34); // KEY_G
        assert(combos[1].out_keys[0] == 1); // KEY_ESC
        std::cout << "✓ Test 5 passed: symmetric combos with '+' generate both permutations\n";
    }

    // Test 6: Leader key grouping
    {
        std::string leader_yaml = R"(
combos:
  f:
    n: down
    u: up
    k: left
    l: right
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(leader_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 4);
        assert(combos[0].keys[0] == 33); // KEY_F
        assert(combos[0].keys[1] == 49); // KEY_N
        assert(combos[0].out_keys[0] == 108); // KEY_DOWN
        std::cout << "✓ Test 6 passed: leader key grouping\n";
    }

    // Test 7: Hotkey modifier output syntax (ctrl+s, alt+tab)
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
        std::cout << "✓ Test 7 passed: hotkey output syntax (ctrl+s)\n";
    }

    // Test 8: Friendly key name aliases (super, win, del, return, caps)
    {
        std::string alias_yaml = R"(
combos:
  caps a: super
  j k: del
  f d: return
)";
        std::vector<tff::Combo> combos;
        std::string err_msg;
        bool ok = tff::loadYamlCombos(alias_yaml, combos, err_msg);
        assert(ok);
        assert(combos.size() == 3);
        assert(combos[0].keys[0] == 58); // KEY_CAPSLOCK
        assert(combos[0].out_keys[0] == 125); // KEY_LEFTMETA
        assert(combos[1].out_keys[0] == 111); // KEY_DELETE
        assert(combos[2].out_keys[0] == 28); // KEY_ENTER
        std::cout << "✓ Test 8 passed: friendly key name aliases\n";
    }

    std::cout << "\nAll YAML parser tests passed!\n";
    return 0;
}
