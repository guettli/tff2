#ifndef KEY_MAPPER_H
#define KEY_MAPPER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>
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
     * @brief Load mappings from a YAML configuration file
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
};

#endif // KEY_MAPPER_H
