#ifndef TFF_CHEATSHEET_H
#define TFF_CHEATSHEET_H

#include "tff_types.h"
#include <string>
#include <vector>

namespace tff {

struct CheatsheetOptions {
    bool color = true;      // Use ANSI escape colors
    bool markdown = false;  // Render as GitHub-flavored Markdown
};

class Cheatsheet {
public:
    /**
     * @brief Generates a formatted cheat sheet from a Config struct.
     */
    static std::string generate(const Config& config, const CheatsheetOptions& opts = {});

    /**
     * @brief Formats a single key or key list into human-friendly strings.
     */
    static std::string formatKeys(const std::vector<KeyCode>& keys);
    static std::string formatKey(KeyCode code);
};

}  // namespace tff

#endif  // TFF_CHEATSHEET_H
