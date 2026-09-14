#include "tff_parser.h"
#include "key_mapper.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing YAML Configuration Parsers\n";
    std::cout << "==================================\n";

    // Test 1: Valid TFF YAML string with loadYamlCombos
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

    std::cout << "\nAll YAML parser tests passed!\n";
    return 0;
}
