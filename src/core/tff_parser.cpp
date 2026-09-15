#include "tff_parser.h"
#include "tff_key_codes.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace tff {

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(s);
    while (std::getline(token_stream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> fields(const std::string& s) {
    std::vector<std::string> tokens;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> parseOutputWords(const std::string& s) {
    std::vector<std::string> result;
    auto raw_tokens = fields(s);
    for (const auto& token : raw_tokens) {
        if (token.find('+') != std::string::npos) {
            auto sub_parts = split(token, '+');
            for (const auto& sp : sub_parts) {
                std::string t = trim(sp);
                if (!t.empty()) result.push_back(t);
            }
        } else {
            result.push_back(token);
        }
    }
    return result;
}

} // namespace

bool parseDurationMicros(const std::string& str, int64_t& out_us) {
    std::string s = trim(str);
    if (!s.empty() && s.front() == '(') s = s.substr(1);
    if (!s.empty() && s.back() == ')') s.pop_back();
    s = trim(s);

    if (s.empty()) return false;

    // Determine unit
    if (s.length() > 2 && s.substr(s.length() - 2) == "ms") {
        double val = std::stod(s.substr(0, s.length() - 2));
        out_us = static_cast<int64_t>(val * 1000.0 + 0.5);
        return true;
    } else if (s.length() > 2 && s.substr(s.length() - 2) == "us") {
        double val = std::stod(s.substr(0, s.length() - 2));
        out_us = static_cast<int64_t>(val + 0.5);
        return true;
    } else if (s.length() > 2 && s.substr(s.length() - 2) == "ns") {
        double val = std::stod(s.substr(0, s.length() - 2));
        out_us = static_cast<int64_t>(val / 1000.0 + 0.5);
        return true;
    } else if (s.length() > 1 && s.back() == 's') {
        double val = std::stod(s.substr(0, s.length() - 1));
        out_us = static_cast<int64_t>(val * 1000000.0 + 0.5);
        return true;
    }
    return false;
}

bool csvLineToEvent(const std::string& line, Event& ev, std::string& err_msg) {
    std::string trimmed = trim(line);
    auto parts = split(trimmed, ';');
    if (parts.size() != 5) {
        err_msg = "failed to parse csv line: " + line;
        return false;
    }

    try {
        ev.time.sec = std::stoll(parts[0]);
        ev.time.usec = std::stoll(parts[1]);
    } catch (...) {
        err_msg = "failed to parse timestamps from line: " + line;
        return false;
    }

    if (!parseTypeName(parts[2], ev.type)) {
        err_msg = "failed to parse EvType: " + parts[2];
        return false;
    }

    if (!parseCodeName(ev.type, parts[3], ev.code)) {
        err_msg = "failed to parse Key: " + parts[3];
        return false;
    }

    if (parts[4] == "up") {
        ev.value = KEY_VAL_UP;
    } else if (parts[4] == "down") {
        ev.value = KEY_VAL_DOWN;
    } else if (parts[4] == "repeat") {
        ev.value = KEY_VAL_REPEAT;
    } else {
        try {
            ev.value = std::stoi(parts[4]);
        } catch (...) {
            err_msg = "failed to parse value: " + parts[4];
            return false;
        }
    }

    return true;
}

bool csvToEvents(const std::string& csv_str, std::vector<Event>& events, std::string& err_msg) {
    std::istringstream iss(csv_str);
    std::string line;
    while (std::getline(iss, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }
        Event ev;
        if (!csvLineToEvent(t, ev, err_msg)) {
            return false;
        }
        events.push_back(ev);
    }
    return true;
}

bool stateStringToEvents(const std::string& state_str, std::vector<Event>& events, std::string& err_msg) {
    TimeVal current_time{1716752333, 0};
    auto parts = fields(state_str);
    if (parts.size() % 2 != 1) {
        err_msg = "stateString has an even number of parts";
        return false;
    }

    for (size_t i = 0; i < parts.size(); ++i) {
        if (i % 2 == 0) {
            // Key part, e.g. "capslock_", "j/", "f_"
            const std::string& part = parts[i];
            if (part.size() < 2) {
                err_msg = "invalid key token: " + part;
                return false;
            }
            char action = part.back();
            int32_t val = -1;
            if (action == '_') {
                val = KEY_VAL_DOWN;
            } else if (action == '/') {
                val = KEY_VAL_UP;
            }
            if (val == -1) {
                err_msg = "invalid action in token: " + part;
                return false;
            }
            std::string word = part.substr(0, part.length() - 1);
            KeyCode code = 0;
            if (!wordToKeyCode(word, code, err_msg)) {
                return false;
            }
            Event ev;
            ev.time = current_time;
            ev.type = EV_KEY;
            ev.code = code;
            ev.value = val;
            events.push_back(ev);
        } else {
            // Duration part, e.g. "(259.006ms)"
            int64_t dur_us = 0;
            if (!parseDurationMicros(parts[i], dur_us)) {
                err_msg = "failed to parse duration: " + parts[i];
                return false;
            }
            int64_t new_us = current_time.toMicros() + dur_us;
            current_time = TimeVal::fromMicros(new_us);
        }
    }
    return true;
}

bool parseComboLog(std::istream& in, std::vector<Event>& events, std::string& err_msg) {
    std::string line;
    while (std::getline(in, line)) {
        auto idx = line.find("|>>");
        if (idx == std::string::npos) {
            continue;
        }
        std::string csv_part = line.substr(idx + 3);
        Event ev;
        if (!csvLineToEvent(csv_part, ev, err_msg)) {
            return false;
        }
        events.push_back(ev);
    }
    return true;
}

bool parseComboLog(const std::string& log_str, std::vector<Event>& events, std::string& err_msg) {
    std::istringstream iss(log_str);
    return parseComboLog(iss, events, err_msg);
}

std::string eventToCsvLine(const Event& ev) {
    std::string val_str;
    switch (ev.value) {
    case KEY_VAL_DOWN: val_str = "down"; break;
    case KEY_VAL_UP: val_str = "up"; break;
    case KEY_VAL_REPEAT: val_str = "repeat"; break;
    default: val_str = std::to_string(ev.value); break;
    }
    return std::to_string(ev.time.sec) + ";" +
           std::to_string(ev.time.usec) + ";" +
           typeName(ev.type) + ";" +
           codeName(ev.type, ev.code) + ";" +
           val_str + "\n";
}

std::string eventsToCsv(const std::vector<Event>& events) {
    std::string out;
    for (const auto& ev : events) {
        if (ev.type == EV_SYN || (ev.type == EV_MSC && ev.code == MSC_SCAN)) {
            continue;
        }
        out += eventToCsvLine(ev);
    }
    return out;
}

std::string eventsToShortCsv(const std::vector<Event>& events) {
    std::string out;
    for (const auto& ev : events) {
        if (ev.type == EV_SYN || (ev.type == EV_MSC && ev.code == MSC_SCAN)) {
            continue;
        }
        if (ev.type == EV_KEY) {
            std::string line = keyCodeToShortName(ev.code) + "-" +
                               (ev.value == KEY_VAL_DOWN ? "down" : "up");
            out += line + "\n";
        }
    }
    return trim(out);
}

std::string normalizeShortCsv(const std::string& input) {
    std::istringstream iss(input);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(iss, line)) {
        std::string t = trim(line);
        if (!t.empty()) {
            lines.push_back(t);
        }
    }
    std::string res;
    for (size_t i = 0; i < lines.size(); ++i) {
        res += lines[i];
        if (i + 1 < lines.size()) {
            res += "\n";
        }
    }
    return res;
}

bool loadYamlConfig(const std::string& yaml_str, Config& config, std::string& err_msg) {
    std::istringstream iss(yaml_str);
    std::string line;
    bool in_combos = false;
    bool in_tap_hold = false;
    std::string current_keys;
    std::string current_outkeys;
    bool has_combos_tag = false;
    bool has_tap_hold_tag = false;
    size_t combos_base_indent = 0;
    size_t tap_hold_base_indent = 0;
    std::string current_leader;
    size_t leader_indent = 0;

    // For multi-line tap_hold definitions
    TapHoldKey pending_th;
    bool has_pending_th = false;
    size_t pending_th_indent = 0;

    auto flush_pending_th = [&]() -> bool {
        if (has_pending_th) {
            if (pending_th.key != 0) {
                if (pending_th.tap_key == 0 || pending_th.hold_key == 0) {
                    err_msg = "tap_hold definition requires both 'tap' and 'hold'";
                    return false;
                }
                config.tap_hold_keys.push_back(pending_th);
            }
            pending_th = TapHoldKey{};
            has_pending_th = false;
        }
        return true;
    };

    while (std::getline(iss, line)) {
        // Strip inline comment if any
        auto comment_pos = line.find('#');
        std::string clean_line = (comment_pos != std::string::npos) ? line.substr(0, comment_pos) : line;
        std::string t = trim(clean_line);
        if (t.empty()) continue;

        size_t current_indent = line.find_first_not_of(" \t");

        if (t.rfind("combos:", 0) == 0) {
            if (!flush_pending_th()) return false;
            in_combos = true;
            in_tap_hold = false;
            has_combos_tag = true;
            combos_base_indent = current_indent;
            current_leader.clear();
            continue;
        } else if (t.rfind("combos", 0) == 0 && t.find(':') == std::string::npos) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (t.rfind("tap_hold:", 0) == 0) {
            if (!flush_pending_th()) return false;
            in_tap_hold = true;
            in_combos = false;
            has_tap_hold_tag = true;
            tap_hold_base_indent = current_indent;
            continue;
        }

        if (in_combos && current_indent <= combos_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            in_combos = false;
            current_leader.clear();
        }

        if (in_tap_hold && current_indent <= tap_hold_base_indent && t.find(':') != std::string::npos) {
            if (!flush_pending_th()) return false;
            in_tap_hold = false;
        }

        if (in_tap_hold) {
            auto colon = t.find(':');
            if (colon == std::string::npos) continue;

            std::string key_part = trim(t.substr(0, colon));
            std::string val_part = trim(t.substr(colon + 1));

            // Check if this is a sub-property of pending_th (e.g. "tap: esc", "hold: super", "timeout_ms: 200")
            if (has_pending_th && current_indent > pending_th_indent) {
                if (key_part == "tap") {
                    if (!wordToKeyCode(val_part, pending_th.tap_key, err_msg)) {
                        return false;
                    }
                } else if (key_part == "hold") {
                    if (!wordToKeyCode(val_part, pending_th.hold_key, err_msg)) {
                        return false;
                    }
                } else if (key_part == "timeout_ms" || key_part == "timeout") {
                    try {
                        long long val = std::stoll(val_part);
                        if (val <= 0) {
                            err_msg = "timeout_ms must be positive: " + val_part;
                            return false;
                        }
                        pending_th.timeout_us = val * 1000LL;
                    } catch (...) {
                        err_msg = "invalid timeout value: " + val_part;
                        return false;
                    }
                } else {
                    err_msg = "unknown tap_hold property: " + key_part;
                    return false;
                }
                continue;
            }

            // New key under tap_hold
            if (!flush_pending_th()) return false;

            KeyCode th_code = 0;
            if (!wordToKeyCode(key_part, th_code, err_msg)) {
                return false;
            }

            if (val_part.empty()) {
                // Multi-line property block, e.g. "capslock:"
                has_pending_th = true;
                pending_th_indent = current_indent;
                pending_th.key = th_code;
                pending_th.timeout_us = 200000LL;
            } else {
                // Inline compact format: "capslock: [esc, super]" or "capslock: esc super"
                std::string clean_val = val_part;
                if (clean_val.front() == '[') clean_val = clean_val.substr(1);
                if (!clean_val.empty() && clean_val.back() == ']') clean_val.pop_back();

                std::vector<std::string> parts;
                if (clean_val.find(',') != std::string::npos) {
                    auto raw_parts = split(clean_val, ',');
                    for (const auto& p : raw_parts) {
                        std::string w = trim(p);
                        if (!w.empty()) parts.push_back(w);
                    }
                } else {
                    parts = fields(clean_val);
                }

                if (parts.size() < 2) {
                    err_msg = "tap_hold for key '" + key_part + "' requires at least tap and hold keys";
                    return false;
                }

                TapHoldKey thk;
                thk.key = th_code;
                if (!wordToKeyCode(parts[0], thk.tap_key, err_msg)) return false;
                if (!wordToKeyCode(parts[1], thk.hold_key, err_msg)) return false;
                if (parts.size() >= 3) {
                    try {
                        long long val = std::stoll(parts[2]);
                        if (val <= 0) {
                            err_msg = "timeout_ms must be positive: " + parts[2];
                            return false;
                        }
                        thk.timeout_us = val * 1000LL;
                    } catch (...) {
                        err_msg = "invalid timeout value: " + parts[2];
                        return false;
                    }
                }
                config.tap_hold_keys.push_back(thk);
            }
            continue;
        }

        if (!in_combos) continue;

        // 1. Classic verbose format
        if (t.find("- keys:") != std::string::npos) {
            if (!current_keys.empty() && current_outkeys.empty()) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            auto colon = t.find(':');
            current_keys = trim(t.substr(colon + 1));
            continue;
        } else if (t.find("- outKeys:") != std::string::npos) {
            if (!current_keys.empty() && current_outkeys.empty()) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            err_msg = "empty list in 'keys' is not allowed";
            return false;
        } else if (t.find("outKeys:") != std::string::npos) {
            auto colon = t.find(':');
            current_outkeys = trim(t.substr(colon + 1));

            if (current_keys.empty()) {
                err_msg = "empty list in 'keys' is not allowed";
                return false;
            }
            if (current_outkeys.empty()) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }

            Combo c;
            auto key_words = fields(current_keys);
            if (key_words.empty()) {
                err_msg = "empty list in 'keys' is not allowed";
                return false;
            }
            for (const auto& w : key_words) {
                KeyCode code = 0;
                if (!wordToKeyCode(w, code, err_msg)) {
                    return false;
                }
                c.keys.push_back(code);
            }

            auto out_words = parseOutputWords(current_outkeys);
            if (out_words.empty()) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            for (const auto& w : out_words) {
                KeyCode code = 0;
                if (!wordToKeyCode(w, code, err_msg)) {
                    return false;
                }
                c.out_keys.push_back(code);
            }

            config.combos.push_back(c);
            current_keys.clear();
            current_outkeys.clear();
            continue;
        }

        // 2. Compact dictionary format or leader prefix
        auto colon = t.find(':');
        if (colon != std::string::npos) {
            std::string key_part = trim(t.substr(0, colon));
            std::string val_part = trim(t.substr(colon + 1));

            // Leader prefix case, e.g. "f:"
            if (val_part.empty()) {
                current_leader = key_part;
                leader_indent = current_indent;
                continue;
            }

            // End of leader block if indentation dropped
            if (!current_leader.empty() && current_indent <= leader_indent) {
                current_leader.clear();
            }

            bool is_symmetric = (key_part.find('+') != std::string::npos);
            std::vector<std::string> chord_words;
            if (is_symmetric) {
                auto raw_parts = split(key_part, '+');
                for (const auto& p : raw_parts) {
                    std::string word = trim(p);
                    if (!word.empty()) chord_words.push_back(word);
                }
                if (chord_words.size() != 2) {
                    err_msg = "symmetric combos with '+' require exactly two keys";
                    return false;
                }
            } else {
                chord_words = fields(key_part);
            }

            if (chord_words.empty()) {
                err_msg = "empty list in 'keys' is not allowed";
                return false;
            }

            std::vector<KeyCode> chord_codes;
            for (const auto& w : chord_words) {
                KeyCode code = 0;
                if (!wordToKeyCode(w, code, err_msg)) {
                    return false;
                }
                chord_codes.push_back(code);
            }

            // If a leader key is active, resolve it
            std::vector<KeyCode> leader_codes;
            if (!current_leader.empty()) {
                auto leader_words = fields(current_leader);
                for (const auto& lw : leader_words) {
                    KeyCode lcode = 0;
                    if (!wordToKeyCode(lw, lcode, err_msg)) {
                        return false;
                    }
                    leader_codes.push_back(lcode);
                }
            }

            auto out_words = parseOutputWords(val_part);
            if (out_words.empty()) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            std::vector<KeyCode> out_codes;
            for (const auto& w : out_words) {
                KeyCode code = 0;
                if (!wordToKeyCode(w, code, err_msg)) {
                    return false;
                }
                out_codes.push_back(code);
            }

            if (is_symmetric) {
                Combo c1;
                c1.keys = leader_codes;
                c1.keys.push_back(chord_codes[0]);
                c1.keys.push_back(chord_codes[1]);
                c1.out_keys = out_codes;
                config.combos.push_back(c1);

                Combo c2;
                c2.keys = leader_codes;
                c2.keys.push_back(chord_codes[1]);
                c2.keys.push_back(chord_codes[0]);
                c2.out_keys = out_codes;
                config.combos.push_back(c2);
            } else {
                Combo c;
                c.keys = leader_codes;
                c.keys.insert(c.keys.end(), chord_codes.begin(), chord_codes.end());
                c.out_keys = out_codes;
                config.combos.push_back(c);
            }
        }
    }

    if (!flush_pending_th()) return false;

    if (!current_keys.empty() && current_outkeys.empty()) {
        err_msg = "empty list in 'outKeys' is not allowed";
        return false;
    }

    if (!has_combos_tag && !has_tap_hold_tag && !yaml_str.empty()) {
        err_msg = "missing combos section";
        return false;
    }

    for (const auto& th : config.tap_hold_keys) {
        for (const auto& combo : config.combos) {
            for (KeyCode k : combo.keys) {
                if (k == th.key) {
                    err_msg = "key '" + keyCodeToWord(th.key) + "' cannot be used in both combos and tap_hold";
                    return false;
                }
            }
        }
    }

    return true;
}

bool loadYamlCombos(const std::string& yaml_str, std::vector<Combo>& combos, std::string& err_msg) {
    Config config;
    if (!loadYamlConfig(yaml_str, config, err_msg)) {
        return false;
    }
    combos = std::move(config.combos);
    return true;
}

} // namespace tff
