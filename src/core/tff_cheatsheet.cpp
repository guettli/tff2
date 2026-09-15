#include "tff_cheatsheet.h"
#include "tff_key_codes.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>

namespace tff {

namespace {

std::string ansi(const std::string& code, const std::string& text, bool enable) {
    if (!enable) return text;
    return "\033[" + code + "m" + text + "\033[0m";
}

std::string bold(const std::string& s, bool enable)    { return ansi("1", s, enable); }
std::string dim(const std::string& s, bool enable)     { return ansi("2", s, enable); }
std::string cyan(const std::string& s, bool enable)    { return ansi("36", s, enable); }
std::string green(const std::string& s, bool enable)   { return ansi("32", s, enable); }
std::string yellow(const std::string& s, bool enable)  { return ansi("33", s, enable); }
std::string magenta(const std::string& s, bool enable) { return ansi("35", s, enable); }
std::string blue(const std::string& s, bool enable)    { return ansi("34", s, enable); }

std::string escapeMarkdown(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '|' || c == '`' || c == '\\') {
            res += '\\';
        }
        res += c;
    }
    return res;
}

std::string formatOutputAction(const std::vector<KeyCode>& out_keys, const std::string& text) {
    if (!text.empty()) {
        std::string escaped;
        for (char c : text) {
            if (c == '\n') escaped += "\\n";
            else if (c == '\t') escaped += "\\t";
            else if (c == '\r') escaped += "\\r";
            else if (c == '"') escaped += "\\\"";
            else escaped += c;
        }
        return "\"" + escaped + "\"";
    }
    if (out_keys.empty()) return "(none)";
    std::string out;
    for (size_t i = 0; i < out_keys.size(); ++i) {
        if (i > 0) out += " + ";
        out += Cheatsheet::formatKey(out_keys[i]);
    }
    return out;
}

void printRow(std::ostream& os, const std::string& prefix,
              const std::vector<std::pair<std::string, size_t>>& cells_plain,
              const std::vector<std::string>& cells_colored) {
    os << prefix;
    for (size_t i = 0; i < cells_plain.size(); ++i) {
        os << cells_colored[i];
        if (i + 1 < cells_plain.size()) {
            size_t width = cells_plain[i].second;
            size_t actual_len = cells_plain[i].first.size();
            size_t pad = (actual_len < width) ? (width - actual_len) : 1;
            os << std::string(pad, ' ');
        }
    }
    os << "\n";
}

} // anonymous namespace

std::string Cheatsheet::formatKey(KeyCode code) {
    if (code == Keys::KEY_LEFTMETA) return "super";
    if (code == Keys::KEY_LEFTCTRL) return "ctrl";
    if (code == Keys::KEY_LEFTSHIFT) return "shift";
    if (code == Keys::KEY_LEFTALT) return "alt";
    return keyCodeToWord(code);
}

std::string Cheatsheet::formatKeys(const std::vector<KeyCode>& keys) {
    std::string out;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (i > 0) out += " ";
        out += formatKey(keys[i]);
    }
    return out;
}

std::string Cheatsheet::generate(const Config& config, const CheatsheetOptions& opts) {
    std::ostringstream ss;
    bool col = opts.color && !opts.markdown;

    if (opts.markdown) {
        ss << "# Ten Flying Fingers — Cheat Sheet\n\n";

        // 1. Tap-vs-Hold
        if (!config.tap_hold_keys.empty()) {
            ss << "## Tap-vs-Hold Keys (Dual-Role)\n\n"
               << "| Key | Tap Action | Hold Action / Layer | Timeout |\n"
               << "|:---|:---|:---|:---|\n";
            for (const auto& th : config.tap_hold_keys) {
                std::string hold_target = !th.hold_layer.empty()
                    ? ("Layer: `" + th.hold_layer + "`")
                    : ("`" + formatKey(th.hold_key) + "`");
                ss << "| `" << formatKey(th.key) << "` | `"
                   << formatKey(th.tap_key) << "` | "
                   << hold_target << " | "
                   << (th.timeout_us / 1000) << "ms |\n";
            }
            ss << "\n";
        }

        // 2. Combos & Chords
        if (!config.combos.empty()) {
            ss << "## Home Row Combos & Chords\n\n"
               << "| Input Keys | Output Action | Type |\n"
               << "|:---|:---|:---|\n";
            for (const auto& c : config.combos) {
                std::string in_keys = formatKeys(c.keys);
                std::string out_act = formatOutputAction(c.out_keys, c.text);
                std::string type = !c.text.empty() ? "Text Snippet" :
                                   (c.out_keys.size() > 1 ? "Modifier Chord" : "Single Key");
                ss << "| `" << in_keys << "` | `" << escapeMarkdown(out_act) << "` | " << type << " |\n";
            }
            ss << "\n";
        }

        // 3. Modal Layers
        if (!config.layers.empty()) {
            ss << "## Modal Keyboard Layers\n\n";
            for (const auto& lyr : config.layers) {
                ss << "### Layer: `" << lyr.name << "`\n\n"
                   << "| Key | Mapped Output | Type |\n"
                   << "|:---|:---|:---|\n";
                // Sort keys for deterministic output
                std::vector<KeyCode> sorted_keys;
                for (const auto& kv : lyr.mappings) {
                    sorted_keys.push_back(kv.first);
                }
                std::sort(sorted_keys.begin(), sorted_keys.end());
                for (KeyCode k : sorted_keys) {
                    const auto& act = lyr.mappings.at(k);
                    std::string out_act = formatOutputAction(act.out_keys, act.text);
                    std::string type = !act.text.empty() ? "Text Snippet" :
                                       (act.out_keys.size() > 1 ? "Modifier Chord" : "Single Key");
                    ss << "| `" << formatKey(k) << "` | `" << escapeMarkdown(out_act) << "` | " << type << " |\n";
                }
                ss << "\n";
            }
        }

        return ss.str();
    }

    // Terminal Text / ANSI Box Format
    std::string sep(76, '=');

    ss << bold(sep, col) << "\n";
    ss << bold("                   TEN FLYING FINGERS — CHEAT SHEET", col) << "\n";
    ss << bold(sep, col) << "\n\n";

    // 1. Tap-vs-Hold
    if (!config.tap_hold_keys.empty()) {
        ss << bold(blue("[ Tap-vs-Hold Keys (Dual-Role) ]", col), col) << "\n\n";
        printRow(ss, "  ",
            {{"Key", 16}, {"Tap Action", 18}, {"Hold Target / Layer", 28}, {"Timeout", 10}},
            {bold("Key", col), bold("Tap Action", col), bold("Hold Target / Layer", col), bold("Timeout", col)});
        ss << "  " << dim(std::string(72, '-'), col) << "\n";

        for (const auto& th : config.tap_hold_keys) {
            std::string plain_key = formatKey(th.key);
            std::string plain_tap = formatKey(th.tap_key);
            std::string plain_hold = !th.hold_layer.empty() ? ("[layer: " + th.hold_layer + "]") : formatKey(th.hold_key);
            std::string plain_timeout = std::to_string(th.timeout_us / 1000) + "ms";

            std::string hold_col = !th.hold_layer.empty() ? magenta(plain_hold, col) : green(plain_hold, col);

            printRow(ss, "  ",
                {{plain_key, 16}, {plain_tap, 18}, {plain_hold, 28}, {plain_timeout, 10}},
                {cyan(plain_key, col), green(plain_tap, col), hold_col, dim(plain_timeout, col)});
        }
        ss << "\n";
    }

    // 2. Combos
    if (!config.combos.empty()) {
        ss << bold(blue("[ Combos & Chords ] (" + std::to_string(config.combos.size()) + " active)", col), col) << "\n\n";
        printRow(ss, "  ",
            {{"Input Keys", 24}, {"Output Action", 34}, {"Type", 14}},
            {bold("Input Keys", col), bold("Output Action", col), bold("Type", col)});
        ss << "  " << dim(std::string(72, '-'), col) << "\n";

        for (const auto& c : config.combos) {
            std::string plain_in = formatKeys(c.keys);
            std::string plain_out = formatOutputAction(c.out_keys, c.text);
            std::string type_plain = !c.text.empty() ? "Text Snippet" :
                               (c.out_keys.size() > 1 ? "Modifier Chord" : "Key");

            std::string out_col = !c.text.empty() ? yellow(plain_out, col) : green(plain_out, col);

            printRow(ss, "  ",
                {{plain_in, 24}, {plain_out, 34}, {type_plain, 14}},
                {cyan(plain_in, col), out_col, dim(type_plain, col)});
        }
        ss << "\n";
    }

    // 3. Modal Layers
    if (!config.layers.empty()) {
        ss << bold(blue("[ Modal Keyboard Layers ] (" + std::to_string(config.layers.size()) + " layer" + (config.layers.size() > 1 ? "s" : "") + ")", col), col) << "\n\n";
        for (const auto& lyr : config.layers) {
            ss << "  " << bold(magenta("* Layer: " + lyr.name, col), col) << "\n";
            printRow(ss, "    ",
                {{"Key", 16}, {"Mapped Output", 34}, {"Type", 14}},
                {bold("Key", col), bold("Mapped Output", col), bold("Type", col)});
            ss << "    " << dim(std::string(64, '-'), col) << "\n";

            std::vector<KeyCode> sorted_keys;
            for (const auto& kv : lyr.mappings) {
                sorted_keys.push_back(kv.first);
            }
            std::sort(sorted_keys.begin(), sorted_keys.end());

            for (KeyCode k : sorted_keys) {
                const auto& act = lyr.mappings.at(k);
                std::string plain_k = formatKey(k);
                std::string plain_out = formatOutputAction(act.out_keys, act.text);
                std::string type_plain = !act.text.empty() ? "Text Snippet" :
                                   (act.out_keys.size() > 1 ? "Modifier Chord" : "Key");

                std::string out_col = !act.text.empty() ? yellow(plain_out, col) : green(plain_out, col);

                printRow(ss, "    ",
                    {{plain_k, 16}, {plain_out, 34}, {type_plain, 14}},
                    {cyan(plain_k, col), out_col, dim(type_plain, col)});
            }
            ss << "\n";
        }
    }

    if (config.combos.empty() && config.tap_hold_keys.empty() && config.layers.empty()) {
        ss << dim("  (No combos, tap-hold keys, or layers configured)", col) << "\n\n";
    }

    return ss.str();
}

} // namespace tff
