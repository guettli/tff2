#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "key_mapper.h"
#include <string>
#include <vector>

/**
 * @brief Configuration manager for loading key mappings from YAML files
 *
 * This class handles parsing YAML configuration files and populating
 * the KeyMapper with user-defined key mappings.
 */
class ConfigManager {
public:
    /**
     * @brief Constructor
     */
    ConfigManager();

    /**
     * @brief Load key mappings from a YAML configuration file
     * @param filepath Path to the YAML configuration file
     * @param key_mapper Reference to the KeyMapper to populate
     * @return true if successful
     */
    bool loadFromFile(const std::string& filepath, KeyMapper& key_mapper);

    /**
     * @brief Save current key mappings to a YAML configuration file
     * @param filepath Path to the YAML configuration file
     * @param key_mapper Reference to the KeyMapper to serialize
     * @return true if successful
     */
    bool saveToFile(const std::string& filepath, const KeyMapper& key_mapper);

private:
    // Helper functions for parsing YAML (stub implementations)
    bool parseYamlContent(const std::string& content, KeyMapper& key_mapper);
    std::string serializeToYaml(const KeyMapper& key_mapper);
};

#endif // CONFIG_MANAGER_H