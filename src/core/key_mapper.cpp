#include "key_mapper.h"
#include "key_detector.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

KeyMapper::KeyMapper() {
    mappings_.clear();
}

bool KeyMapper::loadConfiguration(const std::string& config_file) {
    // Clear existing mappings
    clearMappings();

    // Try to load from the TFF configuration file
    if (config_file.find("my-combos.yaml") != std::string::npos) {
        return loadTffConfiguration(config_file);
    }

    // Default mappings for demonstration
    addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});
    addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});
    addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE});
    addMapping(KeyCodes::J_KEY, KeyCodes::SPACE_KEY, {KeyCodes::FOUR});

    return true;
}

bool KeyMapper::loadTffConfiguration(const std::string& config_file) {
    // For now, we'll implement the TFF mappings directly
    // In a full implementation, this would parse the actual YAML

    // j f -> backspace
    addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::BACKSPACE});

    // f j -> delete
    addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::DELETE});

    // semicolon a -> home
    addMapping(KeyCodes::SEMICOLON, KeyCodes::A, {KeyCodes::HOME});

    // a semicolon -> end
    addMapping(KeyCodes::A, KeyCodes::SEMICOLON, {KeyCodes::END});

    // f n -> down
    addMapping(KeyCodes::F_KEY, KeyCodes::N, {KeyCodes::DOWN_ARROW});

    // f u -> up
    addMapping(KeyCodes::F_KEY, KeyCodes::U, {KeyCodes::UP_ARROW});

    // f m -> down
    addMapping(KeyCodes::F_KEY, KeyCodes::M, {KeyCodes::DOWN_ARROW});

    // f k -> left
    addMapping(KeyCodes::F_KEY, KeyCodes::K, {KeyCodes::LEFT_ARROW});

    // f l -> right
    addMapping(KeyCodes::F_KEY, KeyCodes::L, {KeyCodes::RIGHT_ARROW});

    // f i -> pageup
    addMapping(KeyCodes::F_KEY, KeyCodes::I, {KeyCodes::PAGE_UP});

    // f comma -> pagedown
    addMapping(KeyCodes::F_KEY, KeyCodes::COMMA, {KeyCodes::PAGE_DOWN});

    // g h -> esc
    addMapping(KeyCodes::G, KeyCodes::H, {KeyCodes::ESCAPE});

    return true;
}

std::vector<uint32_t> KeyMapper::getMappedKeys(const KeyCombination& combination) const {
    if (!combination.isValid()) {
        return std::vector<uint32_t>();
    }

    // Create key pair for lookup
    std::pair<uint32_t, uint32_t> key_pair(combination.first_key, combination.second_key);

    auto it = mappings_.find(key_pair);
    if (it != mappings_.end()) {
        return it->second;
    }

    return std::vector<uint32_t>(); // No mapping found
}

void KeyMapper::addMapping(uint32_t first_key, uint32_t second_key, const std::vector<uint32_t>& output_keys) {
    std::pair<uint32_t, uint32_t> key_pair(first_key, second_key);
    mappings_[key_pair] = output_keys;
}

void KeyMapper::clearMappings() {
    mappings_.clear();
}

uint32_t KeyMapper::parseKeyName(const std::string& key_name) const {
    std::string lower_name = key_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (lower_name == "f") return KeyCodes::F_KEY;
    if (lower_name == "j") return KeyCodes::J_KEY;
    if (lower_name == "space") return KeyCodes::SPACE_KEY;
    if (lower_name == "1" || lower_name == "one") return KeyCodes::ONE;
    if (lower_name == "2" || lower_name == "two") return KeyCodes::TWO;
    if (lower_name == "3" || lower_name == "three") return KeyCodes::THREE;
    if (lower_name == "4" || lower_name == "four") return KeyCodes::FOUR;
    if (lower_name == "left") return KeyCodes::LEFT_ARROW;
    if (lower_name == "right") return KeyCodes::RIGHT_ARROW;
    if (lower_name == "up") return KeyCodes::UP_ARROW;
    if (lower_name == "down") return KeyCodes::DOWN_ARROW;
    if (lower_name == "enter" || lower_name == "return") return KeyCodes::ENTER;
    if (lower_name == "backspace") return KeyCodes::BACKSPACE;
    if (lower_name == "delete" || lower_name == "del") return KeyCodes::DELETE;
    if (lower_name == "escape" || lower_name == "esc") return KeyCodes::ESCAPE;
    if (lower_name == "ctrl" || lower_name == "control") return KeyCodes::CONTROL;
    if (lower_name == "shift") return KeyCodes::SHIFT;
    if (lower_name == "alt") return KeyCodes::ALT;
    if (lower_name == "home") return KeyCodes::HOME;
    if (lower_name == "end") return KeyCodes::END;
    if (lower_name == "pageup") return KeyCodes::PAGE_UP;
    if (lower_name == "pagedown") return KeyCodes::PAGE_DOWN;
    if (lower_name == "semicolon" || lower_name == "ö") return KeyCodes::SEMICOLON;
    if (lower_name == "a") return KeyCodes::A;
    if (lower_name == "n") return KeyCodes::N;
    if (lower_name == "u") return KeyCodes::U;
    if (lower_name == "m") return KeyCodes::M;
    if (lower_name == "k") return KeyCodes::K;
    if (lower_name == "l") return KeyCodes::L;
    if (lower_name == "i") return KeyCodes::I;
    if (lower_name == "comma") return KeyCodes::COMMA;
    if (lower_name == "g") return KeyCodes::G;
    if (lower_name == "h") return KeyCodes::H;
    if (lower_name == "d") return KeyCodes::D;

    return 0; // Unknown key
}

uint32_t KeyMapper::parseKeyCode(const std::string& key_str) const {
    try {
        return static_cast<uint32_t>(std::stoul(key_str, nullptr, 0));
    } catch (...) {
        return parseKeyName(key_str);
    }
}