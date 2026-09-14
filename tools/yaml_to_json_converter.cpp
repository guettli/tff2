#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Simple function to convert TFF YAML to JSON
bool convertTffYamlToJson(const std::string& yamlFile, const std::string& jsonFile) {
    std::ifstream inFile(yamlFile);
    if (!inFile.is_open()) {
        std::cerr << "Failed to open input file: " << yamlFile << std::endl;
        return false;
    }

    std::ofstream outFile(jsonFile);
    if (!outFile.is_open()) {
        std::cerr << "Failed to create output file: " << jsonFile << std::endl;
        return false;
    }

    // Write JSON header
    outFile << "{\n";
    outFile << "  \"settings\": {\n";
    outFile << "    \"overlap_threshold_ms\": 100\n";
    outFile << "  },\n";
    outFile << "  \"mappings\": [\n";

    std::string line;
    bool inCombosSection = false;
    std::string currentKeys;
    std::string currentOutKeys;
    bool firstMapping = true;

    while (std::getline(inFile, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Look for combos section
        if (line.find("combos:") != std::string::npos) {
            inCombosSection = true;
            continue;
        }

        // Process combo entries
        if (inCombosSection) {
            if (line.find("- keys:") != std::string::npos) {
                // Extract keys
                size_t startPos = line.find(":") + 2;
                if (startPos < line.length()) {
                    currentKeys = line.substr(startPos);
                    // Trim whitespace
                    currentKeys.erase(0, currentKeys.find_first_not_of(" \t"));
                    currentKeys.erase(currentKeys.find_last_not_of(" \t") + 1);
                }
            } else if (line.find("outKeys:") != std::string::npos) {
                // Extract outKeys
                size_t startPos = line.find(":") + 2;
                if (startPos < line.length()) {
                    currentOutKeys = line.substr(startPos);
                    // Trim whitespace
                    currentOutKeys.erase(0, currentOutKeys.find_first_not_of(" \t"));
                    currentOutKeys.erase(currentOutKeys.find_last_not_of(" \t") + 1);
                }

                // Write the mapping
                if (!currentKeys.empty() && !currentOutKeys.empty()) {
                    if (!firstMapping) {
                        outFile << ",\n";
                    }

                    outFile << "    {\n";
                    outFile << "      \"keys\": \"" << currentKeys << "\",\n";
                    outFile << "      \"outKeys\": \"" << currentOutKeys << "\"\n";
                    outFile << "    }";

                    firstMapping = false;
                    currentKeys.clear();
                    currentOutKeys.clear();
                }
            }
        }
    }

    // Close JSON structure
    outFile << "\n  ]\n";
    outFile << "}\n";

    inFile.close();
    outFile.close();

    std::cout << "Converted " << yamlFile << " to " << jsonFile << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.yaml> <output.json>" << std::endl;
        return 1;
    }

    std::string yamlFile = argv[1];
    std::string jsonFile = argv[2];

    if (!convertTffYamlToJson(yamlFile, jsonFile)) {
        return 1;
    }

    return 0;
}