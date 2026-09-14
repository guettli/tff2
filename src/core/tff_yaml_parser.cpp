#include "key_mapper.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

bool KeyMapper::loadTffConfiguration(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open configuration file: " << config_file << std::endl;
        return false;
    }

    // Clear existing mappings
    clearMappings();

    std::string line;
    bool in_combos_section = false;
    std::string keys_line, outkeys_line;

    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ' ' || line[0] == '\t') {
            continue;
        }

        // Look for the combos section
        if (line.find("combos:") != std::string::npos) {
            in_combos_section = true;
            continue;
        }

        // Process combo entries
        if (in_combos_section) {
            if (line.find("- keys:") != std::string::npos) {
                // Extract keys
                size_t start = line.find(":") + 2;
                size_t end = line.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    keys_line = line.substr(start, end - start + 1);
                }
            } else if (line.find("outKeys:") != std::string::npos) {
                // Extract outKeys
                size_t start = line.find(":") + 2;
                size_t end = line.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    outkeys_line = line.substr(start, end - start + 1);
                }

                // Process the complete combo
                processComboEntry(keys_line, outkeys_line);

                // Reset for next combo
                keys_line.clear();
                outkeys_line.clear();
            }
        }
    }

    file.close();
    return true;
}

bool KeyMapper::processComboEntry(const std::string& keys_str, const std::string& outkeys_str) {
    // Parse keys - format: "key1 key2" or "key1 key2 key3"
    std::vector<std::string> keys;
    std::istringstream key_stream(keys_str);
    std::string key;
    while (key_stream >> key) {
        keys.push_back(key);
    }

    if (keys.size() < 2) {
        return false; // Need at least 2 keys for a combo
    }

    // Parse outKeys - format: "single_key" or "[key1, key2, ...]"
    std::vector<uint32_t> output_keys;

    // Remove brackets if present
    std::string clean_outkeys = outkeys_str;
    if (!clean_outkeys.empty() && clean_outkeys[0] == '[') {
        clean_outkeys = clean_outkeys.substr(1, clean_outkeys.length() - 2); // Remove brackets
    }

    // Split by comma if multiple keys
    std::istringstream outkey_stream(clean_outkeys);
    std::string outkey;
    while (std::getline(outkey_stream, outkey, ',')) {
        // Trim whitespace
        outkey.erase(0, outkey.find_first_not_of(" \t"));
        outkey.erase(outkey.find_last_not_of(" \t") + 1);

        uint32_t key_code = parseKeyName(outkey);
        if (key_code != 0) {
            output_keys.push_back(key_code);
        }
    }

    // If no output keys found, try parsing the whole string
    if (output_keys.empty()) {
        uint32_t key_code = parseKeyName(outkeys_str);
        if (key_code != 0) {
            output_keys.push_back(key_code);
        }
    }

    if (output_keys.empty()) {
        return false;
    }

    // Add mapping for the first two keys (we don't support triple combos yet)
    if (keys.size() >= 2) {
        uint32_t first_key = parseKeyName(keys[0]);
        uint32_t second_key = parseKeyName(keys[1]);

        if (first_key != 0 && second_key != 0) {
            addMapping(first_key, second_key, output_keys);
            return true;
        }
    }

    return false;
}