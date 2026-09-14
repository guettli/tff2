#include "config_manager.h"
#include "key_mapper.h"
#include <iostream>
#include <cassert>
#include <fstream>

int main() {
    std::cout << "Testing ConfigManager Implementation\n";
    std::cout << "=====================================\n";

    // Create a temporary test configuration file
    std::string test_config = R"(# Test configuration
settings:
  overlap_threshold_ms: 100

mappings:
  - name: "test_mapping"
    combo: ["f", "j"]
    output: "1"
)";

    std::ofstream test_file("test_config.yaml");
    test_file << test_config;
    test_file.close();

    // Test ConfigManager
    ConfigManager config_manager;
    KeyMapper key_mapper;

    // Test loading from file
    bool load_result = config_manager.loadFromFile("test_config.yaml", key_mapper);
    assert(load_result);
    std::cout << "✓ ConfigManager load from file successful\n";

    // Test saving to file
    bool save_result = config_manager.saveToFile("test_output.yaml", key_mapper);
    assert(save_result);
    std::cout << "✓ ConfigManager save to file successful\n";

    // Clean up temporary files
    std::remove("test_config.yaml");
    std::remove("test_output.yaml");

    std::cout << "\nAll ConfigManager tests passed!\n";
    return 0;
}