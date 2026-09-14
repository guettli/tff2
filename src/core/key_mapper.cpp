#include "key_mapper.h"
#include "key_detector.h"
#include "tff_parser.h"
#include <fstream>
#include <sstream>

KeyMapper::KeyMapper() {
    mappings_.clear();
}

bool KeyMapper::loadConfiguration(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::vector<tff::Combo> combos;
    std::string err_msg;
    if (!tff::loadYamlCombos(buffer.str(), combos, err_msg)) {
        return false;
    }

    clearMappings();
    for (const auto& combo : combos) {
        if (combo.keys.size() == 2 && !combo.out_keys.empty()) {
            std::vector<uint32_t> out_keys(combo.out_keys.begin(), combo.out_keys.end());
            addMapping(combo.keys[0], combo.keys[1], out_keys);
        }
    }

    return true;
}

std::vector<uint32_t> KeyMapper::getMappedKeys(const KeyCombination& combination) const {
    if (!combination.isValid()) {
        return std::vector<uint32_t>();
    }

    std::pair<uint32_t, uint32_t> key_pair(combination.first_key, combination.second_key);
    auto it = mappings_.find(key_pair);
    if (it != mappings_.end()) {
        return it->second;
    }

    return std::vector<uint32_t>();
}

void KeyMapper::addMapping(uint32_t first_key, uint32_t second_key, const std::vector<uint32_t>& output_keys) {
    std::pair<uint32_t, uint32_t> key_pair(first_key, second_key);
    mappings_[key_pair] = output_keys;
}

void KeyMapper::clearMappings() {
    mappings_.clear();
}
