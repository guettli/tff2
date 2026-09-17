#include "tff_cheatsheet.h"
#include "tff_key_codes.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cassert>

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

std::string formatMarkdownCode(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '|') {
            res += "\\|";
        } else {
            res += c;
        }
    }
    if (res.find('`') != std::string::npos) {
        return "`` " + res + " ``";
    }
    return "`" + res + "`";
}

std::string formatOutputAction(const std::vector<KeyCode>& out_keys, const std::string& text, const std::string& toggle_layer = "", const MouseAction& mouse = MouseAction{}) {
    if (mouse.type != MouseActionType::None) {
        return mouseActionToWord(mouse);
    }
    if (!toggle_layer.empty()) {
        return "toggle_layer(" + toggle_layer + ")";
    }
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
    assert(cells_plain.size() == cells_colored.size());
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
    if (code == Keys::BTN_LEFT) return "mouse_left";
    if (code == Keys::BTN_RIGHT) return "mouse_right";
    if (code == Keys::BTN_MIDDLE) return "mouse_middle";
    if (code == Keys::BTN_SIDE) return "mouse_side";
    if (code == Keys::BTN_EXTRA) return "mouse_extra";
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
                std::string tap_act = formatKey(th.tap_key);
                if (!th.tap_toggle_layer.empty()) {
                    tap_act = "toggle_layer(" + th.tap_toggle_layer + ")";
                } else if (th.tap_leader) {
                    tap_act = "leader";
                } else if (th.tap_one_shot_modifier != 0) {
                    tap_act = "osm(" + formatKey(th.tap_one_shot_modifier) + ")";
                } else if (!th.tap_one_shot_layer.empty()) {
                    tap_act = "osl(" + th.tap_one_shot_layer + ")";
                }
                std::string hold_target = !th.hold_layer.empty()
                    ? ("Layer: `" + th.hold_layer + "`")
                    : (formatMarkdownCode(formatKey(th.hold_key)));
                ss << "| " << formatMarkdownCode(formatKey(th.key)) << " | "
                   << formatMarkdownCode(tap_act) << " | "
                   << hold_target << " | "
                   << (th.timeout_us / 1000) << "ms |\n";
            }
            ss << "\n";
        }

        // One-Shot Keys (OSM / OSL)
        if (!config.one_shot_keys.empty()) {
            ss << "## One-Shot / Sticky Keys (OSM & OSL)\n\n"
               << "| Key | Modifier / Layer | Type | Timeout |\n"
               << "|:---|:---|:---|:---|\n";
            for (const auto& osk : config.one_shot_keys) {
                std::string target = !osk.layer.empty() ? ("Layer: `" + osk.layer + "`") : formatMarkdownCode(formatKey(osk.modifier));
                std::string type = !osk.layer.empty() ? "One-Shot Layer (OSL)" : "One-Shot Modifier (OSM)";
                ss << "| " << formatMarkdownCode(formatKey(osk.key)) << " | "
                   << target << " | " << type << " | "
                   << (osk.timeout_us / 1000) << "ms |\n";
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
                std::string out_act = formatOutputAction(c.out_keys, c.text, c.toggle_layer, c.mouse);
                std::string type = (c.mouse.type != MouseActionType::None) ? "Mouse Action" :
                                   (!c.toggle_layer.empty() ? "Toggle Layer" :
                                   (!c.text.empty() ? "Text Snippet" :
                                   (c.out_keys.size() == 1 && (c.out_keys[0] >= Keys::BTN_LEFT && c.out_keys[0] <= Keys::BTN_EXTRA) ? "Mouse Action" :
                                   (c.out_keys.size() > 1 ? "Modifier Chord" : "Single Key"))));
                ss << "| " << formatMarkdownCode(in_keys) << " | "
                   << formatMarkdownCode(out_act) << " | " << type << " |\n";
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
                    std::string out_act = formatOutputAction(act.out_keys, act.text, act.toggle_layer, act.mouse);
                    std::string type = (act.mouse.type != MouseActionType::None) ? "Mouse Action" :
                                       (!act.toggle_layer.empty() ? "Toggle Layer" :
                                       (!act.text.empty() ? "Text Snippet" :
                                       (act.out_keys.size() == 1 && (act.out_keys[0] >= Keys::BTN_LEFT && act.out_keys[0] <= Keys::BTN_EXTRA) ? "Mouse Action" :
                                       (act.out_keys.size() > 1 ? "Modifier Chord" : "Single Key"))));
                    ss << "| " << formatMarkdownCode(formatKey(k)) << " | "
                       << formatMarkdownCode(out_act) << " | " << type << " |\n";
                }
                ss << "\n";
            }
        }

        // 4. Sequential Leader Sequences
        if (!config.leader.sequences.empty()) {
            std::string trigger_str = config.leader.key != 0 ? formatKey(config.leader.key) : "(custom)";
            ss << "## Sequential Leader Key Sequences\n\n"
               << "Trigger: " << formatMarkdownCode(trigger_str) << " (Timeout: " << (config.leader.timeout_us / 1000) << "ms)\n\n"
               << "| Sequence | Output Action | Type |\n"
               << "|:---|:---|:---|\n";
            for (const auto& seq : config.leader.sequences) {
                std::string in_keys = formatKeys(seq.keys);
                std::string out_act = formatOutputAction(seq.out_keys, seq.text, seq.toggle_layer, seq.mouse);
                std::string type = (seq.mouse.type != MouseActionType::None) ? "Mouse Action" :
                                   (!seq.toggle_layer.empty() ? "Toggle Layer" :
                                   (!seq.text.empty() ? "Text Snippet" :
                                   (seq.out_keys.size() == 1 && (seq.out_keys[0] >= Keys::BTN_LEFT && seq.out_keys[0] <= Keys::BTN_EXTRA) ? "Mouse Action" :
                                   (seq.out_keys.size() > 1 ? "Modifier Chord" : "Single Key"))));
                ss << "| " << formatMarkdownCode(in_keys) << " | "
                   << formatMarkdownCode(out_act) << " | " << type << " |\n";
            }
            ss << "\n";
        }

        // 5. Auto-Shift
        if (config.auto_shift.enabled) {
            ss << "## Auto-Shift (Long-Press Capitalization)\n\n"
               << "- **Status**: Enabled\n"
               << "- **Timeout**: " << (config.auto_shift.timeout_us / 1000) << " ms\n"
               << "- **Keys**: " << config.auto_shift.keys.size() << " keys active\n\n";
        }

        // 6. Global Settings (if customized)
        if (config.settings != Settings{}) {
            ss << "## Global Settings\n\n"
               << "| Setting | Value |\n"
               << "|:---|:---|\n"
               << "| Combo Overlap Window | `" << config.settings.combo_timeout_ms << "ms` |\n"
               << "| Default Tap-Hold Timeout | `" << config.settings.tap_hold_timeout_ms << "ms` |\n"
               << "| Exclusive Grab | `" << (config.settings.exclusive_grab ? "true" : "false") << "` |\n"
               << "| Inotify Hotplug | `" << (config.settings.hotplug ? "true" : "false") << "` |\n\n";
        }

        if (config.combos.empty() && config.tap_hold_keys.empty() && config.layers.empty() &&
            config.one_shot_keys.empty() && config.leader.sequences.empty() && !config.auto_shift.enabled) {
            ss << "_No combos, tap-hold keys, one-shot keys, auto-shift, or layers configured._\n\n";
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
            if (!th.tap_toggle_layer.empty()) {
                plain_tap = "toggle_layer(" + th.tap_toggle_layer + ")";
            } else if (th.tap_leader) {
                plain_tap = "leader";
            } else if (th.tap_one_shot_modifier != 0) {
                plain_tap = "osm(" + formatKey(th.tap_one_shot_modifier) + ")";
            } else if (!th.tap_one_shot_layer.empty()) {
                plain_tap = "osl(" + th.tap_one_shot_layer + ")";
            }
            std::string plain_hold = !th.hold_layer.empty() ? ("[layer: " + th.hold_layer + "]") : formatKey(th.hold_key);
            std::string plain_timeout = std::to_string(th.timeout_us / 1000) + "ms";

            std::string hold_col = !th.hold_layer.empty() ? magenta(plain_hold, col) : green(plain_hold, col);

            printRow(ss, "  ",
                {{plain_key, 16}, {plain_tap, 18}, {plain_hold, 28}, {plain_timeout, 10}},
                {cyan(plain_key, col), green(plain_tap, col), hold_col, dim(plain_timeout, col)});
        }
        ss << "\n";
    }

    // One-Shot Keys (OSM / OSL)
    if (!config.one_shot_keys.empty()) {
        ss << bold(blue("[ One-Shot / Sticky Keys (OSM / OSL) ] (" + std::to_string(config.one_shot_keys.size()) + " active)", col), col) << "\n\n";
        printRow(ss, "  ",
            {{"Key", 16}, {"Target", 20}, {"Type", 26}, {"Timeout", 10}},
            {bold("Key", col), bold("Target", col), bold("Type", col), bold("Timeout", col)});
        ss << "  " << dim(std::string(72, '-'), col) << "\n";

        for (const auto& osk : config.one_shot_keys) {
            std::string plain_key = formatKey(osk.key);
            std::string plain_target = !osk.layer.empty() ? ("[layer: " + osk.layer + "]") : formatKey(osk.modifier);
            std::string plain_type = !osk.layer.empty() ? "One-Shot Layer (OSL)" : "One-Shot Modifier (OSM)";
            std::string plain_timeout = std::to_string(osk.timeout_us / 1000) + "ms";

            std::string target_col = !osk.layer.empty() ? magenta(plain_target, col) : green(plain_target, col);

            printRow(ss, "  ",
                {{plain_key, 16}, {plain_target, 20}, {plain_type, 26}, {plain_timeout, 10}},
                {cyan(plain_key, col), target_col, dim(plain_type, col), dim(plain_timeout, col)});
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
            std::string plain_out = formatOutputAction(c.out_keys, c.text, c.toggle_layer, c.mouse);
            std::string type_plain = (c.mouse.type != MouseActionType::None) ? "Mouse Action" :
                               (!c.toggle_layer.empty() ? "Toggle Layer" :
                               (!c.text.empty() ? "Text Snippet" :
                               (c.out_keys.size() == 1 && (c.out_keys[0] >= Keys::BTN_LEFT && c.out_keys[0] <= Keys::BTN_EXTRA) ? "Mouse Action" :
                               (c.out_keys.size() > 1 ? "Modifier Chord" : "Single Key"))));

            std::string out_col = (c.mouse.type != MouseActionType::None || (c.out_keys.size() == 1 && c.out_keys[0] >= Keys::BTN_LEFT && c.out_keys[0] <= Keys::BTN_EXTRA)) ? blue(plain_out, col) :
                                  (!c.toggle_layer.empty() ? magenta(plain_out, col) :
                                  (!c.text.empty() ? yellow(plain_out, col) : green(plain_out, col)));

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
                std::string plain_out = formatOutputAction(act.out_keys, act.text, act.toggle_layer, act.mouse);
                std::string type_plain = (act.mouse.type != MouseActionType::None) ? "Mouse Action" :
                                   (!act.toggle_layer.empty() ? "Toggle Layer" :
                                   (!act.text.empty() ? "Text Snippet" :
                                   (act.out_keys.size() == 1 && (act.out_keys[0] >= Keys::BTN_LEFT && act.out_keys[0] <= Keys::BTN_EXTRA) ? "Mouse Action" :
                                   (act.out_keys.size() > 1 ? "Modifier Chord" : "Single Key"))));

                std::string out_col = (act.mouse.type != MouseActionType::None || (act.out_keys.size() == 1 && act.out_keys[0] >= Keys::BTN_LEFT && act.out_keys[0] <= Keys::BTN_EXTRA)) ? blue(plain_out, col) :
                                      (!act.toggle_layer.empty() ? magenta(plain_out, col) :
                                      (!act.text.empty() ? yellow(plain_out, col) : green(plain_out, col)));

                printRow(ss, "    ",
                    {{plain_k, 16}, {plain_out, 34}, {type_plain, 14}},
                    {cyan(plain_k, col), out_col, dim(type_plain, col)});
            }
            ss << "\n";
        }
    }

    // 4. Sequential Leader Sequences
    if (!config.leader.sequences.empty()) {
        std::string trigger_str = config.leader.key != 0 ? formatKey(config.leader.key) : "(custom)";
        ss << bold(blue("[ Sequential Leader Sequences ] (" + std::to_string(config.leader.sequences.size()) + " active)", col), col) << "\n";
        ss << "  " << dim("Trigger: ", col) << cyan(trigger_str, col)
           << dim(" | Timeout: " + std::to_string(config.leader.timeout_us / 1000) + "ms", col) << "\n\n";
        printRow(ss, "  ",
            {{"Sequence", 24}, {"Output Action", 34}, {"Type", 14}},
            {bold("Sequence", col), bold("Output Action", col), bold("Type", col)});
        ss << "  " << dim(std::string(72, '-'), col) << "\n";

        for (const auto& seq : config.leader.sequences) {
            std::string plain_in = formatKeys(seq.keys);
            std::string plain_out = formatOutputAction(seq.out_keys, seq.text, seq.toggle_layer, seq.mouse);
            std::string type_plain = (seq.mouse.type != MouseActionType::None) ? "Mouse Action" :
                               (!seq.toggle_layer.empty() ? "Toggle Layer" :
                               (!seq.text.empty() ? "Text Snippet" :
                               (seq.out_keys.size() == 1 && (seq.out_keys[0] >= Keys::BTN_LEFT && seq.out_keys[0] <= Keys::BTN_EXTRA) ? "Mouse Action" :
                               (seq.out_keys.size() > 1 ? "Modifier Chord" : "Single Key"))));

            std::string out_col = (seq.mouse.type != MouseActionType::None || (seq.out_keys.size() == 1 && seq.out_keys[0] >= Keys::BTN_LEFT && seq.out_keys[0] <= Keys::BTN_EXTRA)) ? blue(plain_out, col) :
                                  (!seq.toggle_layer.empty() ? magenta(plain_out, col) :
                                  (!seq.text.empty() ? yellow(plain_out, col) : green(plain_out, col)));

            printRow(ss, "  ",
                {{plain_in, 24}, {plain_out, 34}, {type_plain, 14}},
                {cyan(plain_in, col), out_col, dim(type_plain, col)});
        }
        ss << "\n";
    }

    // 5. Auto-Shift
    if (config.auto_shift.enabled) {
        ss << bold(blue("[ Auto-Shift (Long-Press Capitalization) ]", col), col) << "\n";
        ss << "  " << dim("Status: ", col) << green("Enabled", col)
           << dim(" | Timeout: " + std::to_string(config.auto_shift.timeout_us / 1000) + "ms", col)
           << dim(" | Active Keys: " + std::to_string(config.auto_shift.keys.size()), col) << "\n\n";
    }

    // 6. Global Settings (if customized)
    if (config.settings != Settings{}) {
        ss << bold(blue("[ Global Settings ]", col), col) << "\n";
        ss << "  " << dim("Combo Timeout: ", col) << cyan(std::to_string(config.settings.combo_timeout_ms) + "ms", col)
           << dim(" | Tap-Hold Timeout: ", col) << cyan(std::to_string(config.settings.tap_hold_timeout_ms) + "ms", col)
           << dim(" | Exclusive Grab: ", col) << cyan(config.settings.exclusive_grab ? "yes" : "no", col)
           << dim(" | Hotplug: ", col) << cyan(config.settings.hotplug ? "yes" : "no", col) << "\n\n";
    }

    if (config.combos.empty() && config.tap_hold_keys.empty() && config.layers.empty() &&
        config.one_shot_keys.empty() && config.leader.sequences.empty() && !config.auto_shift.enabled) {
        ss << dim("  (No combos, tap-hold keys, one-shot keys, auto-shift, or layers configured)", col) << "\n\n";
    }

    return ss.str();
}

} // namespace tff
