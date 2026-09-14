#ifndef KEY_MAPPER_H
#define KEY_MAPPER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include "key_events.h"

/**
 * @brief Maps detected key combinations to output key sequences
 *
 * This class handles the configuration-driven mapping of input key
 * combinations to output key sequences. It is platform-independent.
 */
class KeyMapper {
public:
    /**
     * @brief Constructor
     */
    KeyMapper();

    /**
     * @brief Load mappings from a configuration file
     * @param config_file Path to YAML configuration file
     * @return true if successful, false otherwise
     */
    bool loadConfiguration(const std::string& config_file);

    /**
     * @brief Get the output key sequence for a combination
     * @param combination The detected key combination
     * @return Vector of output key codes, empty if no mapping
     */
    std::vector<uint32_t> getMappedKeys(const KeyCombination& combination) const;

    /**
     * @brief Add a mapping programmatically
     * @param first_key First key in combination
     * @param second_key Second key in combination
     * @param output_keys Vector of output key codes
     */
    void addMapping(uint32_t first_key, uint32_t second_key, const std::vector<uint32_t>& output_keys);

    /**
     * @brief Clear all mappings
     */
    void clearMappings();

    /**
     * @brief Get number of mappings
     */
    size_t getMappingCount() const { return mappings_.size(); }

private:
    // Key combination hash function for unordered_map
    struct KeyComboHash {
        std::size_t operator()(const std::pair<uint32_t, uint32_t>& k) const {
            return std::hash<uint32_t>{}(k.first) ^ (std::hash<uint32_t>{}(k.second) << 1);
        }
    };

    std::unordered_map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>, KeyComboHash> mappings_;

    /**
     * @brief Parse a key name to key code
     * @param key_name String name of key (e.g., "f", "space", "ctrl")
     * @return Key code, or 0 if unknown
     */
    uint32_t parseKeyName(const std::string& key_name) const;

    /**
     * @brief Parse a key code from string representation
     * @param key_str String representation of key code
     * @return Key code, or 0 if invalid
     */
    uint32_t parseKeyCode(const std::string& key_str) const;

    /**
     * @brief Load TFF-specific configuration
     * @param config_file Path to TFF YAML configuration file
     * @return true if successful, false otherwise
     */
    bool loadTffConfiguration(const std::string& config_file);

    /**
     * @brief Process a single combo entry from YAML
     * @param keys_str String containing input keys
     * @param outkeys_str String containing output keys
     * @return true if successful, false otherwise
     */
    bool processComboEntry(const std::string& keys_str, const std::string& outkeys_str);
};

#endif // KEY_MAPPER_H