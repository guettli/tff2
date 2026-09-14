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
            int32_t val = (action == '_' ? KEY_VAL_DOWN : (action == '/' ? KEY_VAL_UP : -1));
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

bool loadYamlCombos(const std::string& yaml_str, std::vector<Combo>& combos, std::string& err_msg) {
    std::istringstream iss(yaml_str);
    std::string line;
    bool in_combos = false;
    std::string current_keys;
    std::string current_outkeys;
    bool has_combos_tag = false;

    while (std::getline(iss, line)) {
        std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;

        if (t.rfind("combos:", 0) == 0) {
            in_combos = true;
            has_combos_tag = true;
            continue;
        } else if (t.rfind("combos", 0) == 0 && t.find(':') == std::string::npos) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (!in_combos) continue;

        if (t.find("- keys:") != std::string::npos) {
            if (!current_keys.empty() && current_outkeys.empty()) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            auto colon = t.find(':');
            current_keys = trim(t.substr(colon + 1));
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

            auto out_words = fields(current_outkeys);
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

            combos.push_back(c);
            current_keys.clear();
            current_outkeys.clear();
        }
    }

    if (!current_keys.empty() && current_outkeys.empty()) {
        err_msg = "empty list in 'outKeys' is not allowed";
        return false;
    }

    if (!has_combos_tag && !yaml_str.empty()) {
        err_msg = "missing combos section";
        return false;
    }

    return true;
}

} // namespace tff
