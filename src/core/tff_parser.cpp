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

std::string stripComment(const std::string& line) {
    bool in_double_quote = false;
    bool in_single_quote = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\\' && in_double_quote && i + 1 < line.size()) {
            ++i; // skip escaped character
            continue;
        }
        if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        } else if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '#' && !in_double_quote && !in_single_quote) {
            return line.substr(0, i);
        }
    }
    return line;
}

std::string unquoteAndUnescape(const std::string& s) {
    std::string str = trim(s);
    if (str.empty()) return "";

    // Check for inline dictionary like { text: "..." } or { type: "..." }
    if (str.front() == '{' && str.back() == '}') {
        std::string inner = trim(str.substr(1, str.size() - 2));
        auto col = inner.find(':');
        if (col != std::string::npos) {
            std::string key = trim(inner.substr(0, col));
            if (key == "text" || key == "type") {
                return unquoteAndUnescape(inner.substr(col + 1));
            }
        }
    }

    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        std::string res;
        size_t end = str.size() - 1;
        for (size_t i = 1; i < end; ++i) {
            if (str[i] == '\\' && i + 1 < end) {
                char next = str[i + 1];
                if (next == 'n') { res += '\n'; ++i; }
                else if (next == 't') { res += '\t'; ++i; }
                else if (next == 'r') { res += '\r'; ++i; }
                else if (next == '"') { res += '"'; ++i; }
                else if (next == '\'') { res += '\''; ++i; }
                else if (next == '\\') { res += '\\'; ++i; }
                else { res += next; ++i; }
            } else {
                res += str[i];
            }
        }
        return res;
    }

    if (str.size() >= 2 && str.front() == '\'' && str.back() == '\'') {
        std::string res;
        size_t end = str.size() - 1;
        for (size_t i = 1; i < end; ++i) {
            if (str[i] == '\'' && i + 1 < end && str[i + 1] == '\'') {
                res += '\'';
                ++i;
            } else {
                res += str[i];
            }
        }
        return res;
    }

    return str;
}

bool isTextSnippet(const std::string& val, std::string& text) {
    std::string s = trim(val);
    if (s.empty()) return false;

    // Direct quoted string: "..." or '...'
    if ((s.size() >= 2 && s.front() == '"' && s.back() == '"') ||
        (s.size() >= 2 && s.front() == '\'' && s.back() == '\'')) {
        text = unquoteAndUnescape(s);
        return true;
    }

    // Inline mapping: { text: "..." } or { type: "..." }
    if (s.front() == '{' && s.back() == '}') {
        std::string inner = trim(s.substr(1, s.size() - 2));
        auto col = inner.find(':');
        if (col != std::string::npos) {
            std::string key = trim(inner.substr(0, col));
            if (key == "text" || key == "type") {
                text = unquoteAndUnescape(inner.substr(col + 1));
                return true;
            }
        }
    }

    // Explicit prefix: text: "..." or type: "..."
    if (s.rfind("text:", 0) == 0 || s.rfind("type:", 0) == 0) {
        auto col = s.find(':');
        text = unquoteAndUnescape(s.substr(col + 1));
        return true;
    }

    return false;
}

bool isOneShotPattern(const std::string& str) {
    std::string s = trim(str);
    return !s.empty() && s.back() == ')' &&
        (s.rfind("osm(", 0) == 0 || s.rfind("osl(", 0) == 0 || s.rfind("one_shot(", 0) == 0);
}

bool parseOneShotTarget(const std::string& str, KeyCode& out_mod, std::string& out_layer, std::string& err_msg) {
    std::string s = trim(str);
    if (!s.empty() && s.back() == ')') {
        if (s.rfind("osm(", 0) == 0) {
            std::string inner = trim(s.substr(4, s.size() - 5));
            if (!wordToKeyCode(inner, out_mod, err_msg)) {
                return false;
            }
            if (!isModifier(out_mod)) {
                err_msg = "invalid modifier key '" + inner + "' in osm";
                return false;
            }
            out_layer.clear();
            return true;
        }
        if (s.rfind("osl(", 0) == 0) {
            std::string inner = trim(s.substr(4, s.size() - 5));
            out_layer = inner;
            out_mod = 0;
            return true;
        }
        if (s.rfind("one_shot(", 0) == 0) {
            std::string inner = trim(s.substr(9, s.size() - 10));
            std::string dummy_err;
            if (wordToKeyCode(inner, out_mod, dummy_err) && isModifier(out_mod)) {
                out_layer.clear();
                return true;
            } else {
                out_layer = inner;
                out_mod = 0;
                return true;
            }
        }
    }
    return false;
}

bool parseToggleLayerTarget(const std::string& str, std::string& out_layer, std::string& err_msg) {
    std::string s = trim(str);
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        s = trim(s.substr(1, s.size() - 2));
    }
    if (s.empty()) return false;

    // { toggle_layer: numpad } or { tg: numpad }
    if (s.front() == '{' && s.back() == '}') {
        std::string inner = trim(s.substr(1, s.size() - 2));
        auto col = inner.find(':');
        if (col != std::string::npos) {
            std::string key = trim(inner.substr(0, col));
            std::string val = trim(inner.substr(col + 1));
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = trim(val.substr(1, val.size() - 2));
            }
            if (key == "toggle_layer" || key == "tg") {
                if (val.empty()) {
                    err_msg = "empty layer name in toggle_layer";
                    return false;
                }
                out_layer = val;
                return true;
            }
        }
    }

    if (s.back() == ')') {
        if (s.rfind("toggle_layer(", 0) == 0) {
            std::string inner = trim(s.substr(13, s.size() - 14));
            if (inner.size() >= 2 && ((inner.front() == '"' && inner.back() == '"') || (inner.front() == '\'' && inner.back() == '\''))) {
                inner = trim(inner.substr(1, inner.size() - 2));
            }
            if (inner.empty()) {
                err_msg = "empty layer name in toggle_layer()";
                return false;
            }
            out_layer = inner;
            return true;
        }
        if (s.rfind("tg(", 0) == 0) {
            std::string inner = trim(s.substr(3, s.size() - 4));
            if (inner.size() >= 2 && ((inner.front() == '"' && inner.back() == '"') || (inner.front() == '\'' && inner.back() == '\''))) {
                inner = trim(inner.substr(1, inner.size() - 2));
            }
            if (inner.empty()) {
                err_msg = "empty layer name in tg()";
                return false;
            }
            out_layer = inner;
            return true;
        }
    }

    if (s.rfind("toggle_layer:", 0) == 0) {
        std::string inner = trim(s.substr(13));
        if (inner.size() >= 2 && ((inner.front() == '"' && inner.back() == '"') || (inner.front() == '\'' && inner.back() == '\''))) {
            inner = trim(inner.substr(1, inner.size() - 2));
        }
        if (inner.empty()) {
            err_msg = "empty layer name in toggle_layer";
            return false;
        }
        out_layer = inner;
        return true;
    }
    return false;
}

void addAutoShiftPreset(const std::string& preset, std::vector<KeyCode>& keys) {
    if (preset == "letters" || preset == "alpha") {
        static const KeyCode letter_codes[] = {
            Keys::KEY_A, Keys::KEY_B, Keys::KEY_C, Keys::KEY_D, Keys::KEY_E,
            Keys::KEY_F, Keys::KEY_G, Keys::KEY_H, Keys::KEY_I, Keys::KEY_J,
            Keys::KEY_K, Keys::KEY_L, Keys::KEY_M, Keys::KEY_N, Keys::KEY_O,
            Keys::KEY_P, Keys::KEY_Q, Keys::KEY_R, Keys::KEY_S, Keys::KEY_T,
            Keys::KEY_U, Keys::KEY_V, Keys::KEY_W, Keys::KEY_X, Keys::KEY_Y,
            Keys::KEY_Z
        };
        for (KeyCode kc : letter_codes) keys.push_back(kc);
    } else if (preset == "numbers" || preset == "digits") {
        static const KeyCode number_codes[] = {
            Keys::KEY_1, Keys::KEY_2, Keys::KEY_3, Keys::KEY_4, Keys::KEY_5,
            Keys::KEY_6, Keys::KEY_7, Keys::KEY_8, Keys::KEY_9, Keys::KEY_0
        };
        for (KeyCode kc : number_codes) keys.push_back(kc);
    } else if (preset == "symbols" || preset == "punctuation") {
        static const KeyCode symbol_codes[] = {
            Keys::KEY_MINUS, Keys::KEY_EQUAL, Keys::KEY_LEFTBRACE, Keys::KEY_RIGHTBRACE,
            Keys::KEY_SEMICOLON, Keys::KEY_APOSTROPHE, Keys::KEY_GRAVE, Keys::KEY_BACKSLASH,
            Keys::KEY_COMMA, Keys::KEY_DOT, Keys::KEY_SLASH
        };
        for (KeyCode kc : symbol_codes) keys.push_back(kc);
    } else if (preset == "all") {
        addAutoShiftPreset("letters", keys);
        addAutoShiftPreset("numbers", keys);
        addAutoShiftPreset("symbols", keys);
    }
}

bool parseAutoShiftKeyItem(const std::string& item, std::vector<KeyCode>& out_keys, std::string& err_msg) {
    std::string s = trim(item);
    if (s.empty()) return true;
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
        s = trim(s.substr(1, s.size() - 2));
    }
    if (s == "letters" || s == "alpha" || s == "numbers" || s == "digits" ||
        s == "symbols" || s == "punctuation" || s == "all") {
        addAutoShiftPreset(s, out_keys);
        return true;
    }
    KeyCode kc = 0;
    if (!wordToKeyCode(s, kc, err_msg)) {
        err_msg = "unknown key or preset in auto_shift keys: " + s;
        return false;
    }
    out_keys.push_back(kc);
    return true;
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
    config = Config{};
    std::istringstream iss(yaml_str);
    std::string line;
    bool in_combos = false;
    bool in_tap_hold = false;
    bool in_layers = false;
    bool in_one_shot = false;
    bool in_leader = false;
    bool in_leader_sequences = false;
    bool in_auto_shift = false;
    bool in_auto_shift_keys_list = false;
    bool in_mouse = false;
    std::string current_keys;
    std::string current_outkeys;
    std::string current_text;
    bool has_text = false;
    std::string current_toggle_layer;
    bool has_toggle_layer = false;
    MouseAction current_mouse;
    bool has_mouse = false;
    bool has_combos_tag = false;
    bool has_tap_hold_tag = false;
    bool has_layers_tag = false;
    bool has_one_shot_tag = false;
    bool has_leader_tag = false;
    bool has_auto_shift_tag = false;
    bool has_mouse_tag = false;
    size_t combos_base_indent = 0;
    size_t tap_hold_base_indent = 0;
    size_t layers_base_indent = 0;
    size_t one_shot_base_indent = 0;
    size_t leader_base_indent = 0;
    size_t leader_sequences_indent = 0;
    size_t auto_shift_base_indent = 0;
    size_t auto_shift_keys_list_indent = 0;
    size_t mouse_base_indent = 0;
    std::string current_leader;
    size_t leader_indent = 0;
    std::string current_layer_name;
    size_t current_layer_indent = 0;

    // For multi-line tap_hold definitions
    TapHoldKey pending_th;
    bool has_pending_th = false;
    size_t pending_th_indent = 0;

    auto flush_pending_th = [&]() -> bool {
        if (has_pending_th) {
            if (pending_th.key != 0) {
                if ((pending_th.tap_key == 0 && pending_th.tap_one_shot_modifier == 0 && pending_th.tap_one_shot_layer.empty() && !pending_th.tap_leader) ||
                    (pending_th.hold_key == 0 && pending_th.hold_layer.empty())) {
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

    // For multi-line one_shot definitions
    OneShotKey pending_os;
    bool has_pending_os = false;
    size_t pending_os_indent = 0;

    auto flush_pending_os = [&]() -> bool {
        if (has_pending_os) {
            if (pending_os.key != 0) {
                if (pending_os.modifier == 0 && pending_os.layer.empty()) {
                    if (isModifier(pending_os.key)) {
                        pending_os.modifier = pending_os.key;
                    } else {
                        err_msg = "one_shot key '" + keyCodeToWord(pending_os.key) + "' requires a modifier or layer";
                        return false;
                    }
                }
                config.one_shot_keys.push_back(pending_os);
            }
            pending_os = OneShotKey{};
            has_pending_os = false;
        }
        return true;
    };

    // For multi-line leader sequence definitions
    LeaderSequence pending_lseq;
    bool has_pending_lseq = false;
    size_t pending_lseq_indent = 0;

    auto flush_pending_lseq = [&]() -> bool {
        if (has_pending_lseq) {
            if (pending_lseq.keys.empty()) {
                err_msg = "leader sequence missing keys";
                return false;
            }
            if (pending_lseq.out_keys.empty() && pending_lseq.text.empty() && pending_lseq.toggle_layer.empty()) {
                err_msg = "leader sequence requires an action (text, keys, or toggle_layer)";
                return false;
            }
            config.leader.sequences.push_back(pending_lseq);
            pending_lseq = LeaderSequence{};
            has_pending_lseq = false;
        }
        return true;
    };

    while (std::getline(iss, line)) {
        // Strip inline comment if any (preserving '#' inside quoted strings)
        std::string clean_line = stripComment(line);
        std::string t = trim(clean_line);
        if (t.empty()) continue;

        size_t current_indent = line.find_first_not_of(" \t");

        if (t.rfind("combos:", 0) == 0) {
            if (!flush_pending_th()) return false;
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_combos = true;
            in_tap_hold = false;
            in_layers = false;
            in_one_shot = false;
            in_leader = false;
            in_leader_sequences = false;
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
            in_mouse = false;
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
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_tap_hold = true;
            in_combos = false;
            in_layers = false;
            in_one_shot = false;
            in_leader = false;
            in_leader_sequences = false;
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
            in_mouse = false;
            has_tap_hold_tag = true;
            tap_hold_base_indent = current_indent;
            continue;
        }

        if (t.rfind("layers:", 0) == 0) {
            if (!flush_pending_th()) return false;
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_layers = true;
            in_combos = false;
            in_tap_hold = false;
            in_one_shot = false;
            in_leader = false;
            in_leader_sequences = false;
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
            in_mouse = false;
            has_layers_tag = true;
            layers_base_indent = current_indent;
            current_layer_name.clear();
            continue;
        } else if (t.rfind("layers", 0) == 0 && t.find(':') == std::string::npos) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (t.rfind("one_shot:", 0) == 0) {
            if (!flush_pending_th()) return false;
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_one_shot = true;
            in_combos = false;
            in_tap_hold = false;
            in_layers = false;
            in_leader = false;
            in_leader_sequences = false;
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
            in_mouse = false;
            has_one_shot_tag = true;
            one_shot_base_indent = current_indent;
            continue;
        } else if (t.rfind("one_shot", 0) == 0 && t.find(':') == std::string::npos) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (t.rfind("leader:", 0) == 0) {
            if (!flush_pending_th()) return false;
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_leader = true;
            in_combos = false;
            in_tap_hold = false;
            in_layers = false;
            in_one_shot = false;
            in_leader_sequences = false;
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
            in_mouse = false;
            has_leader_tag = true;
            leader_base_indent = current_indent;
            continue;
        } else if (t.rfind("leader", 0) == 0 && t.find(':') == std::string::npos) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (t.rfind("auto_shift:", 0) == 0) {
            if (!flush_pending_th()) return false;
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_auto_shift = true;
            in_combos = false;
            in_tap_hold = false;
            in_layers = false;
            in_one_shot = false;
            in_leader = false;
            in_leader_sequences = false;
            in_mouse = false;
            has_auto_shift_tag = true;
            auto_shift_base_indent = current_indent;
            config.auto_shift.enabled = true;
            continue;
        } else if (t.rfind("auto_shift", 0) == 0 && t.find(':') == std::string::npos) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (t.rfind("mouse:", 0) == 0 && (!in_layers || current_indent <= layers_base_indent)) {
            if (!flush_pending_th()) return false;
            if (!flush_pending_os()) return false;
            if (!flush_pending_lseq()) return false;
            in_mouse = true;
            in_combos = false;
            in_tap_hold = false;
            in_layers = false;
            in_one_shot = false;
            in_leader = false;
            in_leader_sequences = false;
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
            has_mouse_tag = true;
            mouse_base_indent = current_indent;
            continue;
        } else if (t.rfind("mouse", 0) == 0 && t.find(':') == std::string::npos && (!in_layers || current_indent <= layers_base_indent)) {
            err_msg = "mapping values are not allowed in this context";
            return false;
        }

        if (in_combos && current_indent <= combos_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            in_combos = false;
            current_leader.clear();
        }

        if (in_tap_hold && current_indent <= tap_hold_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            if (!flush_pending_th()) return false;
            in_tap_hold = false;
        }

        if (in_layers && current_indent <= layers_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            in_layers = false;
            current_layer_name.clear();
        }

        if (in_one_shot && current_indent <= one_shot_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            if (!flush_pending_os()) return false;
            in_one_shot = false;
        }

        if (in_leader && current_indent <= leader_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            if (!flush_pending_lseq()) return false;
            in_leader = false;
            in_leader_sequences = false;
        }

        if (in_auto_shift && current_indent <= auto_shift_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            in_auto_shift = false;
            in_auto_shift_keys_list = false;
        }

        if (in_mouse && current_indent <= mouse_base_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
            in_mouse = false;
        }

        if (in_mouse) {
            auto colon = t.find(':');
            if (colon == std::string::npos) continue;
            std::string k = trim(t.substr(0, colon));
            std::string v = trim(t.substr(colon + 1));
            if (k == "speed" || k == "move_speed" || k == "mouse_speed") {
                try {
                    int spd = std::stoi(v);
                    if (spd < 1 || spd > 1000) {
                        err_msg = "mouse speed must be between 1 and 1000";
                        return false;
                    }
                    config.mouse.move_speed = static_cast<int16_t>(spd);
                } catch (...) {
                    err_msg = "invalid integer for mouse speed";
                    return false;
                }
            } else if (k == "wheel_step" || k == "wheel_speed" || k == "scroll_step") {
                try {
                    int ws = std::stoi(v);
                    if (ws < 1 || ws > 100) {
                        err_msg = "mouse wheel_step must be between 1 and 100";
                        return false;
                    }
                    config.mouse.wheel_step = static_cast<int16_t>(ws);
                } catch (...) {
                    err_msg = "invalid integer for mouse wheel_step";
                    return false;
                }
            } else {
                err_msg = "unknown field '" + k + "' in mouse section";
                return false;
            }
            continue;
        }

        if (in_layers) {
            auto colon = t.find(':');
            if (colon == std::string::npos) continue;

            std::string key_part = trim(t.substr(0, colon));
            std::string val_part = trim(t.substr(colon + 1));

            if (current_layer_name.empty() || current_indent <= current_layer_indent) {
                // Layer header, e.g. "nav:"
                if (key_part.empty()) {
                    err_msg = "empty layer name";
                    return false;
                }
                for (const auto& lyr : config.layers) {
                    if (lyr.name == key_part) {
                        err_msg = "duplicate layer definition: " + key_part;
                        return false;
                    }
                }
                current_layer_name = key_part;
                current_layer_indent = current_indent;
                Layer new_layer;
                new_layer.name = current_layer_name;
                config.layers.push_back(new_layer);
                continue;
            }

            // Key mapping inside current layer
            KeyCode in_code = 0;
            if (!wordToKeyCode(key_part, in_code, err_msg)) {
                return false;
            }

            if (config.layers.back().mappings.find(in_code) != config.layers.back().mappings.end()) {
                err_msg = "duplicate mapping for key '" + key_part + "' in layer '" + current_layer_name + "'";
                return false;
            }

            if (val_part.empty()) {
                err_msg = "empty mapping for key '" + key_part + "' in layer '" + current_layer_name + "'";
                return false;
            }

            std::string snippet;
            std::string toggle_layer_name;
            std::string toggle_err;
            MouseAction mouse_act;
            std::string mouse_err;
            if (isTextSnippet(val_part, snippet)) {
                if (snippet.empty()) {
                    err_msg = "empty text snippet is not allowed";
                    return false;
                }
                for (char ch : snippet) {
                    KeyCode kc = 0;
                    bool shift = false;
                    if (!asciiToKeyStroke(ch, kc, shift)) {
                        err_msg = "unsupported character in text snippet: '" + std::string(1, ch) + "'";
                        return false;
                    }
                }
                LayerAction act;
                act.text = snippet;
                config.layers.back().mappings[in_code] = act;
            } else if (parseToggleLayerTarget(val_part, toggle_layer_name, toggle_err)) {
                LayerAction act;
                act.toggle_layer = toggle_layer_name;
                config.layers.back().mappings[in_code] = act;
            } else if (!toggle_err.empty()) {
                err_msg = toggle_err;
                return false;
            } else if (parseMouseAction(val_part, mouse_act, mouse_err)) {
                LayerAction act;
                if (mouse_act.isButton()) {
                    KeyCode btn_code = (mouse_act.type == MouseActionType::BtnRight) ? Keys::BTN_RIGHT :
                                       (mouse_act.type == MouseActionType::BtnMiddle) ? Keys::BTN_MIDDLE :
                                       (mouse_act.type == MouseActionType::BtnSide) ? Keys::BTN_SIDE :
                                       (mouse_act.type == MouseActionType::BtnExtra) ? Keys::BTN_EXTRA :
                                       Keys::BTN_LEFT;
                    act.out_keys = {btn_code};
                } else {
                    act.mouse = mouse_act;
                }
                config.layers.back().mappings[in_code] = act;
            } else if (!mouse_err.empty()) {
                err_msg = mouse_err;
                return false;
            } else {
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
                    LayerAction act;
                    act.out_keys = out_codes;
                    config.layers.back().mappings[in_code] = act;
                }
            continue;
        }

        if (in_tap_hold) {
            std::string line_content = t;
            if (line_content.rfind("- key:", 0) == 0) {
                line_content = trim(line_content.substr(2)); // "key: ..."
            } else if (line_content.rfind("- ", 0) == 0 && line_content.find(':') != std::string::npos) {
                line_content = trim(line_content.substr(2));
            }

            auto colon = line_content.find(':');
            if (colon == std::string::npos) continue;

            std::string key_part = trim(line_content.substr(0, colon));
            std::string val_part = trim(line_content.substr(colon + 1));

            // Check if this is a new "- key: space" list item
            if (key_part == "key") {
                if (!flush_pending_th()) return false;
                KeyCode th_code = 0;
                if (!wordToKeyCode(val_part, th_code, err_msg)) {
                    return false;
                }
                has_pending_th = true;
                pending_th_indent = current_indent;
                pending_th.key = th_code;
                pending_th.timeout_us = 200000LL;
                continue;
            }

            // Check if this is a sub-property of pending_th (e.g. "tap: esc", "hold: super", "layer: nav", "timeout_ms: 200")
            if (has_pending_th && current_indent > pending_th_indent) {
                if (key_part == "tap") {
                    std::string toggle_layer_name;
                    std::string toggle_err;
                    if (parseToggleLayerTarget(val_part, toggle_layer_name, toggle_err)) {
                        pending_th.tap_toggle_layer = toggle_layer_name;
                    } else if (!toggle_err.empty()) {
                        err_msg = toggle_err;
                        return false;
                    } else if (val_part == "leader") {
                        pending_th.tap_leader = true;
                    } else if (isOneShotPattern(val_part)) {
                        if (!parseOneShotTarget(val_part, pending_th.tap_one_shot_modifier, pending_th.tap_one_shot_layer, err_msg)) {
                            return false;
                        }
                    } else if (!wordToKeyCode(val_part, pending_th.tap_key, err_msg)) {
                        return false;
                    }
                } else if (key_part == "toggle_layer" || key_part == "tap_toggle_layer" || key_part == "tg") {
                    pending_th.tap_toggle_layer = val_part;
                } else if (key_part == "hold") {
                    std::string dummy_err;
                    if (wordToKeyCode(val_part, pending_th.hold_key, dummy_err)) {
                        // valid key code
                    } else {
                        // layer name (validated at EOF)
                        pending_th.hold_layer = val_part;
                    }
                } else if (key_part == "layer" || key_part == "hold_layer") {
                    pending_th.hold_layer = val_part;
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
                // Inline compact format: "capslock: [esc, super]" or "space: [space, nav, 200]"
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
                std::string toggle_layer_name;
                std::string toggle_err;
                if (parseToggleLayerTarget(parts[0], toggle_layer_name, toggle_err)) {
                    thk.tap_toggle_layer = toggle_layer_name;
                } else if (!toggle_err.empty()) {
                    err_msg = toggle_err;
                    return false;
                } else if (parts[0] == "leader") {
                    thk.tap_leader = true;
                } else if (isOneShotPattern(parts[0])) {
                    if (!parseOneShotTarget(parts[0], thk.tap_one_shot_modifier, thk.tap_one_shot_layer, err_msg)) {
                        return false;
                    }
                } else if (!wordToKeyCode(parts[0], thk.tap_key, err_msg)) {
                    return false;
                }

                std::string hold_target = parts[1];
                if (hold_target.rfind("layer:", 0) == 0) {
                    thk.hold_layer = hold_target.substr(6);
                } else {
                    std::string dummy_err;
                    if (wordToKeyCode(hold_target, thk.hold_key, dummy_err)) {
                        // hold_key set
                    } else {
                        thk.hold_layer = hold_target;
                    }
                }
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

        if (in_one_shot) {
            std::string line_content = t;
            if (line_content.rfind("- key:", 0) == 0) {
                line_content = trim(line_content.substr(2)); // "key: ..."
            } else if (line_content.rfind("- ", 0) == 0 && line_content.find(':') != std::string::npos) {
                line_content = trim(line_content.substr(2));
            }

            auto colon = line_content.find(':');
            if (colon == std::string::npos) continue;

            std::string key_part = trim(line_content.substr(0, colon));
            std::string val_part = trim(line_content.substr(colon + 1));

            // Check if this is a new "- key: space" list item
            if (key_part == "key") {
                if (!flush_pending_os()) return false;
                KeyCode os_code = 0;
                if (!wordToKeyCode(val_part, os_code, err_msg)) {
                    return false;
                }
                has_pending_os = true;
                pending_os_indent = current_indent;
                pending_os.key = os_code;
                pending_os.timeout_us = 1500000LL;
                continue;
            }

            // Check if this is a sub-property of pending_os (e.g. "modifier: shift", "layer: nav", "timeout_ms: 1500")
            if (has_pending_os && current_indent > pending_os_indent) {
                if (key_part == "modifier" || key_part == "mod") {
                    if (!wordToKeyCode(val_part, pending_os.modifier, err_msg)) {
                        return false;
                    }
                    if (!isModifier(pending_os.modifier)) {
                        err_msg = "invalid modifier key '" + val_part + "' in one_shot";
                        return false;
                    }
                } else if (key_part == "layer") {
                    pending_os.layer = val_part;
                } else if (key_part == "timeout_ms" || key_part == "timeout") {
                    try {
                        long long val = std::stoll(val_part);
                        if (val <= 0) {
                            err_msg = "timeout_ms must be positive: " + val_part;
                            return false;
                        }
                        pending_os.timeout_us = val * 1000LL;
                    } catch (...) {
                        err_msg = "invalid timeout value: " + val_part;
                        return false;
                    }
                } else {
                    err_msg = "unknown one_shot property: " + key_part;
                    return false;
                }
                continue;
            }

            // New key under one_shot
            if (!flush_pending_os()) return false;

            KeyCode os_code = 0;
            if (!wordToKeyCode(key_part, os_code, err_msg)) {
                return false;
            }

            if (val_part.empty()) {
                // Multi-line property block, e.g. "leftshift:"
                has_pending_os = true;
                pending_os_indent = current_indent;
                pending_os.key = os_code;
                pending_os.timeout_us = 1500000LL;
            } else {
                // Inline compact format: "leftshift: 1500", "space: [nav, 1500]", "capslock: [shift, 1500]"
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

                OneShotKey osk;
                osk.key = os_code;
                osk.timeout_us = 1500000LL;

                if (parts.size() == 1) {
                    try {
                        long long val = std::stoll(parts[0]);
                        if (val <= 0) {
                            err_msg = "timeout_ms must be positive: " + parts[0];
                            return false;
                        }
                        osk.timeout_us = val * 1000LL;
                        if (isModifier(os_code)) {
                            osk.modifier = os_code;
                        }
                    } catch (...) {
                        std::string dummy_err;
                        KeyCode parsed_mod = 0;
                        if (wordToKeyCode(parts[0], parsed_mod, dummy_err) && isModifier(parsed_mod)) {
                            osk.modifier = parsed_mod;
                        } else {
                            osk.layer = parts[0];
                        }
                    }
                } else if (parts.size() >= 2) {
                    std::string dummy_err;
                    KeyCode parsed_mod = 0;
                    if (wordToKeyCode(parts[0], parsed_mod, dummy_err) && isModifier(parsed_mod)) {
                        osk.modifier = parsed_mod;
                    } else {
                        osk.layer = parts[0];
                    }
                    try {
                        long long val = std::stoll(parts[1]);
                        if (val <= 0) {
                            err_msg = "timeout_ms must be positive: " + parts[1];
                            return false;
                        }
                        osk.timeout_us = val * 1000LL;
                    } catch (...) {
                        err_msg = "invalid timeout value: " + parts[1];
                        return false;
                    }
                }

                if (osk.modifier == 0 && osk.layer.empty()) {
                    if (isModifier(os_code)) {
                        osk.modifier = os_code;
                    } else {
                        err_msg = "one_shot key '" + key_part + "' requires a modifier or layer";
                        return false;
                    }
                }
                config.one_shot_keys.push_back(osk);
            }
            continue;
        }

        if (in_leader) {
            if (in_leader_sequences) {
                if (current_indent <= leader_sequences_indent && t.find(':') != std::string::npos && t.rfind("-", 0) != 0) {
                    if (!flush_pending_lseq()) return false;
                    in_leader_sequences = false;
                }
            }

            if (!in_leader_sequences) {
                auto colon = t.find(':');
                if (colon == std::string::npos) continue;
                std::string prop = trim(t.substr(0, colon));
                std::string val = trim(t.substr(colon + 1));

                if (prop == "key") {
                    if (!flush_pending_lseq()) return false;
                    KeyCode k = 0;
                    if (!wordToKeyCode(val, k, err_msg)) return false;
                    config.leader.key = k;
                    continue;
                } else if (prop == "timeout_ms" || prop == "timeout") {
                    if (!flush_pending_lseq()) return false;
                    try {
                        long long v = std::stoll(val);
                        if (v <= 0) {
                            err_msg = "leader timeout_ms must be positive: " + val;
                            return false;
                        }
                        config.leader.timeout_us = v * 1000LL;
                    } catch (...) {
                        err_msg = "invalid leader timeout value: " + val;
                        return false;
                    }
                    continue;
                } else if (prop == "sequences") {
                    if (!flush_pending_lseq()) return false;
                    in_leader_sequences = true;
                    leader_sequences_indent = current_indent;
                    continue;
                } else {
                    err_msg = "unknown leader property: " + prop;
                    return false;
                }
            }

            std::string line_content = t;
            bool is_list_item = false;
            if (line_content.rfind("-", 0) == 0) {
                line_content = trim(line_content.substr(1));
                is_list_item = true;
            }

            size_t colon = std::string::npos;
            bool in_dquote = false;
            bool in_squote = false;
            for (size_t i = 0; i < line_content.size(); ++i) {
                char c = line_content[i];
                if (c == '"' && !in_squote) in_dquote = !in_dquote;
                else if (c == '\'' && !in_dquote) in_squote = !in_squote;
                else if (c == ':' && !in_dquote && !in_squote) {
                    colon = i;
                    break;
                }
            }

            if (colon == std::string::npos) continue;

            std::string key_part = trim(line_content.substr(0, colon));
            std::string val_part = trim(line_content.substr(colon + 1));

            auto parse_seq_keys = [&](const std::string& str, std::vector<KeyCode>& out_keys) -> bool {
                std::string s = trim(str);
                if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
                    s = trim(s.substr(1, s.size() - 2));
                }
                if (!s.empty() && s.front() == '[') {
                    if (s.back() == ']') s.pop_back();
                    s = s.substr(1);
                    for (char& ch : s) if (ch == ',') ch = ' ';
                }
                auto key_tokens = fields(s);
                if (key_tokens.empty()) {
                    err_msg = "leader sequence keys cannot be empty";
                    return false;
                }
                for (const auto& kt : key_tokens) {
                    KeyCode kc = 0;
                    if (!wordToKeyCode(kt, kc, err_msg)) return false;
                    out_keys.push_back(kc);
                }
                return true;
            };

            if (has_pending_lseq && current_indent > pending_lseq_indent) {
                if (key_part == "toggle_layer" || key_part == "tg") {
                    pending_lseq.toggle_layer = val_part;
                } else if (key_part == "text" || key_part == "type") {
                    pending_lseq.text = unquoteAndUnescape(val_part);
                } else if (key_part == "out" || key_part == "out_keys" || key_part == "keys_out" || key_part == "action") {
                    std::string toggle_layer_name;
                    std::string toggle_err;
                    MouseAction mouse_act;
                    std::string mouse_err;
                    if (parseToggleLayerTarget(val_part, toggle_layer_name, toggle_err)) {
                        pending_lseq.toggle_layer = toggle_layer_name;
                    } else if (!toggle_err.empty()) {
                        err_msg = toggle_err;
                        return false;
                    } else if (parseMouseAction(val_part, mouse_act, mouse_err)) {
                        pending_lseq.mouse = mouse_act;
                    } else if (!mouse_err.empty()) {
                        err_msg = mouse_err;
                        return false;
                    } else {
                        auto words = parseOutputWords(val_part);
                        for (const auto& w : words) {
                            KeyCode kc = 0;
                            if (!wordToKeyCode(w, kc, err_msg)) return false;
                            pending_lseq.out_keys.push_back(kc);
                        }
                    }
                } else {
                    err_msg = "unknown leader sequence property: " + key_part;
                    return false;
                }
                continue;
            }

            if (is_list_item && (key_part == "keys" || key_part == "seq" || key_part == "in")) {
                if (!flush_pending_lseq()) return false;
                std::vector<KeyCode> sk;
                if (!parse_seq_keys(val_part, sk)) return false;
                pending_lseq.keys = sk;
                has_pending_lseq = true;
                pending_lseq_indent = current_indent;
                continue;
            }

            if (!flush_pending_lseq()) return false;

            LeaderSequence seq;
            if (!parse_seq_keys(key_part, seq.keys)) return false;

            if (val_part.empty()) {
                err_msg = "leader sequence '" + key_part + "' requires an action";
                return false;
            }

            std::string snippet;
            std::string toggle_layer_name;
            std::string toggle_err;
            MouseAction mouse_act;
            std::string mouse_err;
            if (isTextSnippet(val_part, snippet)) {
                seq.text = snippet;
            } else if (parseToggleLayerTarget(val_part, toggle_layer_name, toggle_err)) {
                seq.toggle_layer = toggle_layer_name;
            } else if (!toggle_err.empty()) {
                err_msg = toggle_err;
                return false;
            } else if (parseMouseAction(val_part, mouse_act, mouse_err)) {
                seq.mouse = mouse_act;
            } else if (!mouse_err.empty()) {
                err_msg = mouse_err;
                return false;
            } else {
                auto words = parseOutputWords(val_part);
                for (const auto& w : words) {
                    KeyCode kc = 0;
                    if (!wordToKeyCode(w, kc, err_msg)) return false;
                    seq.out_keys.push_back(kc);
                }
            }
            config.leader.sequences.push_back(seq);
            continue;
        }

        if (in_auto_shift) {
            if (in_auto_shift_keys_list) {
                if (current_indent <= auto_shift_keys_list_indent && t.rfind("-", 0) != 0) {
                    in_auto_shift_keys_list = false;
                } else if (t.rfind("-", 0) == 0) {
                    std::string item = trim(t.substr(1));
                    if (!parseAutoShiftKeyItem(item, config.auto_shift.keys, err_msg)) {
                        return false;
                    }
                    continue;
                }
            }

            auto colon = t.find(':');
            if (colon == std::string::npos) {
                if (t.rfind("-", 0) == 0) {
                    std::string item = trim(t.substr(1));
                    if (!parseAutoShiftKeyItem(item, config.auto_shift.keys, err_msg)) {
                        return false;
                    }
                    continue;
                }
                continue;
            }

            std::string key_part = trim(t.substr(0, colon));
            std::string val_part = trim(t.substr(colon + 1));

            if (key_part == "enabled") {
                if (val_part == "true" || val_part == "yes" || val_part == "on" || val_part == "1") {
                    config.auto_shift.enabled = true;
                } else if (val_part == "false" || val_part == "no" || val_part == "off" || val_part == "0") {
                    config.auto_shift.enabled = false;
                } else {
                    err_msg = "invalid boolean for auto_shift enabled: " + val_part;
                    return false;
                }
            } else if (key_part == "timeout_ms" || key_part == "timeout") {
                try {
                    long long v = std::stoll(val_part);
                    if (v <= 0) {
                        err_msg = "auto_shift timeout_ms must be positive: " + val_part;
                        return false;
                    }
                    config.auto_shift.timeout_us = v * 1000LL;
                } catch (...) {
                    err_msg = "invalid auto_shift timeout value: " + val_part;
                    return false;
                }
            } else if (key_part == "keys") {
                if (val_part.empty()) {
                    in_auto_shift_keys_list = true;
                    auto_shift_keys_list_indent = current_indent;
                } else {
                    std::string vp = val_part;
                    if (!vp.empty() && vp.front() == '[') {
                        if (vp.back() == ']') vp.pop_back();
                        vp = vp.substr(1);
                        for (char& ch : vp) if (ch == ',') ch = ' ';
                    }
                    auto tokens = fields(vp);
                    for (const auto& tok : tokens) {
                        if (!parseAutoShiftKeyItem(tok, config.auto_shift.keys, err_msg)) {
                            return false;
                        }
                    }
                }
            } else {
                err_msg = "unknown auto_shift property: " + key_part;
                return false;
            }
            continue;
        }

        if (!in_combos) continue;

        // 1. Classic verbose format
        if (t.find("- keys:") != std::string::npos || t.find("- in:") != std::string::npos) {
            if (!current_keys.empty() && current_outkeys.empty() && !has_text && !has_toggle_layer) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            auto colon = t.find(':');
            current_keys = trim(t.substr(colon + 1));
            if (!current_keys.empty() && current_keys.front() == '[') {
                if (current_keys.back() == ']') current_keys.pop_back();
                current_keys = current_keys.substr(1);
                for (char& ch : current_keys) {
                    if (ch == ',') ch = ' ';
                }
                current_keys = trim(current_keys);
            }
            current_outkeys.clear();
            current_text.clear();
            current_toggle_layer.clear();
            has_text = false;
            has_toggle_layer = false;
            continue;
        } else if (t.find("- outKeys:") != std::string::npos || t.find("- out:") != std::string::npos) {
            if (!current_keys.empty() && current_outkeys.empty() && !has_text && !has_toggle_layer) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }
            err_msg = "empty list in 'keys' is not allowed";
            return false;
        } else if (t.find("outKeys:") != std::string::npos || t.rfind("out:", 0) == 0 ||
                   t.rfind("text:", 0) == 0 || t.rfind("type:", 0) == 0 ||
                   t.rfind("toggle_layer:", 0) == 0 || t.rfind("tg:", 0) == 0 ||
                   t.rfind("action:", 0) == 0) {
            auto colon = t.find(':');
            std::string prop = trim(t.substr(0, colon));
            std::string val_part = trim(t.substr(colon + 1));

            std::string toggle_layer_name;
            std::string toggle_err;
            MouseAction mouse_act;
            std::string mouse_err;
            std::string snippet;
            if (prop == "toggle_layer" || prop == "tg") {
                has_toggle_layer = true;
                current_toggle_layer = val_part;
                if (current_toggle_layer.empty()) {
                    err_msg = "empty layer name in toggle_layer";
                    return false;
                }
            } else if (prop == "text" || prop == "type") {
                has_text = true;
                current_text = unquoteAndUnescape(val_part);
                if (current_text.empty()) {
                    err_msg = "empty text snippet is not allowed";
                    return false;
                }
            } else if (isTextSnippet(val_part, snippet)) {
                has_text = true;
                current_text = snippet;
                if (current_text.empty()) {
                    err_msg = "empty text snippet is not allowed";
                    return false;
                }
            } else if (parseToggleLayerTarget(val_part, toggle_layer_name, toggle_err)) {
                has_toggle_layer = true;
                current_toggle_layer = toggle_layer_name;
            } else if (!toggle_err.empty()) {
                err_msg = toggle_err;
                return false;
            } else if (parseMouseAction(val_part, mouse_act, mouse_err)) {
                has_mouse = true;
                current_mouse = mouse_act;
            } else if (!mouse_err.empty()) {
                err_msg = mouse_err;
                return false;
            } else {
                current_outkeys = val_part;
            }

            if (current_keys.empty()) {
                err_msg = "empty list in 'keys' is not allowed";
                return false;
            }
            if (current_outkeys.empty() && !has_text && !has_toggle_layer && !has_mouse) {
                err_msg = "empty list in 'outKeys' is not allowed";
                return false;
            }

            bool is_symmetric = (current_keys.find('+') != std::string::npos);
            std::vector<std::string> chord_words;
            if (is_symmetric) {
                auto raw_parts = split(current_keys, '+');
                for (const auto& p : raw_parts) {
                    std::string word = trim(p);
                    if (!word.empty()) chord_words.push_back(word);
                }
                if (chord_words.size() < 2) {
                    err_msg = "symmetric combos with '+' require at least two keys";
                    return false;
                }
                if (chord_words.size() > 5) {
                    err_msg = "symmetric combo exceeds maximum supported chord size of 5 keys";
                    return false;
                }
            } else {
                chord_words = fields(current_keys);
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

            std::vector<KeyCode> out_codes;
            if (has_toggle_layer || has_mouse) {
                // No out codes needed
            } else if (has_text) {
                for (char ch : current_text) {
                    KeyCode kc = 0;
                    bool shift = false;
                    if (!asciiToKeyStroke(ch, kc, shift)) {
                        err_msg = "unsupported character in text snippet: '" + std::string(1, ch) + "'";
                        return false;
                    }
                }
            } else {
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
                    out_codes.push_back(code);
                }
            }

            if (is_symmetric) {
                for (size_t i = 0; i < chord_codes.size(); ++i) {
                    for (size_t j = i + 1; j < chord_codes.size(); ++j) {
                        if (chord_codes[i] == chord_codes[j]) {
                            err_msg = "duplicate key in symmetric combo: " + chord_words[i];
                            return false;
                        }
                    }
                }
                std::vector<KeyCode> perm = chord_codes;
                std::sort(perm.begin(), perm.end());
                do {
                    Combo c;
                    c.keys = perm;
                    if (has_toggle_layer) {
                        c.toggle_layer = current_toggle_layer;
                    } else if (has_mouse) {
                        c.mouse = current_mouse;
                    } else if (has_text) {
                        c.text = current_text;
                    } else {
                        c.out_keys = out_codes;
                    }
                    config.combos.push_back(c);
                } while (std::next_permutation(perm.begin(), perm.end()));
            } else {
                Combo c;
                c.keys = chord_codes;
                if (has_toggle_layer) {
                    c.toggle_layer = current_toggle_layer;
                } else if (has_mouse) {
                    c.mouse = current_mouse;
                } else if (has_text) {
                    c.text = current_text;
                } else {
                    c.out_keys = out_codes;
                }
                config.combos.push_back(c);
            }

            current_keys.clear();
            current_outkeys.clear();
            current_text.clear();
            current_toggle_layer.clear();
            current_mouse = MouseAction{};
            has_text = false;
            has_toggle_layer = false;
            has_mouse = false;
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
                if (chord_words.size() < 2) {
                    err_msg = "symmetric combos with '+' require at least two keys";
                    return false;
                }
                if (chord_words.size() > 5) {
                    err_msg = "symmetric combo exceeds maximum supported chord size of 5 keys";
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

            std::string snippet;
            bool snippet_mode = isTextSnippet(val_part, snippet);
            if (snippet_mode && snippet.empty()) {
                err_msg = "empty text snippet is not allowed";
                return false;
            }

            std::string toggle_layer_name;
            std::string toggle_err;
            bool toggle_mode = !snippet_mode && parseToggleLayerTarget(val_part, toggle_layer_name, toggle_err);
            if (!toggle_err.empty()) {
                err_msg = toggle_err;
                return false;
            }

            MouseAction mouse_act;
            std::string mouse_err;
            bool mouse_mode = !snippet_mode && !toggle_mode && parseMouseAction(val_part, mouse_act, mouse_err);
            if (!mouse_err.empty()) {
                err_msg = mouse_err;
                return false;
            }

            std::vector<KeyCode> out_codes;
            if (toggle_mode || mouse_mode) {
                // No out codes needed
            } else if (snippet_mode) {
                for (char ch : snippet) {
                    KeyCode kc = 0;
                    bool shift = false;
                    if (!asciiToKeyStroke(ch, kc, shift)) {
                        err_msg = "unsupported character in text snippet: '" + std::string(1, ch) + "'";
                        return false;
                    }
                }
            } else {
                auto out_words = parseOutputWords(val_part);
                if (out_words.empty()) {
                    err_msg = "empty list in 'outKeys' is not allowed";
                    return false;
                }
                for (const auto& w : out_words) {
                    KeyCode code = 0;
                    if (!wordToKeyCode(w, code, err_msg)) {
                        return false;
                    }
                    out_codes.push_back(code);
                }
            }

            if (is_symmetric) {
                for (size_t i = 0; i < chord_codes.size(); ++i) {
                    for (size_t j = i + 1; j < chord_codes.size(); ++j) {
                        if (chord_codes[i] == chord_codes[j]) {
                            err_msg = "duplicate key in symmetric combo: " + chord_words[i];
                            return false;
                        }
                    }
                }
                for (KeyCode lk : leader_codes) {
                    for (KeyCode ck : chord_codes) {
                        if (lk == ck) {
                            err_msg = "chord key '" + keyCodeToWord(ck) + "' conflicts with leader key";
                            return false;
                        }
                    }
                }
                std::vector<KeyCode> perm = chord_codes;
                std::sort(perm.begin(), perm.end());
                do {
                    Combo c;
                    c.keys = leader_codes;
                    c.keys.insert(c.keys.end(), perm.begin(), perm.end());
                    if (toggle_mode) {
                        c.toggle_layer = toggle_layer_name;
                    } else if (mouse_mode) {
                        c.mouse = mouse_act;
                    } else if (snippet_mode) {
                        c.text = snippet;
                    } else {
                        c.out_keys = out_codes;
                    }
                    config.combos.push_back(c);
                } while (std::next_permutation(perm.begin(), perm.end()));
            } else {
                Combo c;
                c.keys = leader_codes;
                c.keys.insert(c.keys.end(), chord_codes.begin(), chord_codes.end());
                if (toggle_mode) {
                    c.toggle_layer = toggle_layer_name;
                } else if (mouse_mode) {
                    c.mouse = mouse_act;
                } else if (snippet_mode) {
                    c.text = snippet;
                } else {
                    c.out_keys = out_codes;
                }
                config.combos.push_back(c);
            }
        }
    }

    if (!flush_pending_th()) return false;
    if (!flush_pending_os()) return false;
    if (!flush_pending_lseq()) return false;

    if (!current_keys.empty() && current_outkeys.empty() && !has_text && !has_toggle_layer && !has_mouse) {
        err_msg = "empty list in 'outKeys' is not allowed";
        return false;
    }

    if (!has_combos_tag && !has_tap_hold_tag && !has_layers_tag && !has_one_shot_tag && !has_leader_tag && !has_auto_shift_tag && !has_mouse_tag && !yaml_str.empty()) {
        err_msg = "missing combos section";
        return false;
    }

    for (const auto& th : config.tap_hold_keys) {
        if (!th.hold_layer.empty()) {
            bool found_layer = false;
            for (const auto& lyr : config.layers) {
                if (lyr.name == th.hold_layer) {
                    found_layer = true;
                    break;
                }
            }
            if (!found_layer) {
                err_msg = "unknown layer '" + th.hold_layer + "' referenced in tap_hold";
                return false;
            }
        }
        if (!th.tap_one_shot_layer.empty()) {
            bool found_layer = false;
            for (const auto& lyr : config.layers) {
                if (lyr.name == th.tap_one_shot_layer) {
                    found_layer = true;
                    break;
                }
            }
            if (!found_layer) {
                err_msg = "unknown layer '" + th.tap_one_shot_layer + "' referenced in tap_hold";
                return false;
            }
        }
        if (!th.tap_toggle_layer.empty()) {
            bool found_layer = false;
            for (const auto& lyr : config.layers) {
                if (lyr.name == th.tap_toggle_layer) {
                    found_layer = true;
                    break;
                }
            }
            if (!found_layer) {
                err_msg = "unknown layer '" + th.tap_toggle_layer + "' referenced in tap_hold toggle_layer";
                return false;
            }
        }
        if (th.tap_one_shot_modifier != 0 && !isModifier(th.tap_one_shot_modifier)) {
            err_msg = "invalid modifier key '" + keyCodeToWord(th.tap_one_shot_modifier) + "' in tap_hold osm";
            return false;
        }
        for (const auto& combo : config.combos) {
            for (KeyCode k : combo.keys) {
                if (k == th.key) {
                    err_msg = "key '" + keyCodeToWord(th.key) + "' cannot be used in both combos and tap_hold";
                    return false;
                }
            }
        }
    }

    for (const auto& combo : config.combos) {
        if (!combo.toggle_layer.empty()) {
            bool found_layer = false;
            for (const auto& lyr : config.layers) {
                if (lyr.name == combo.toggle_layer) {
                    found_layer = true;
                    break;
                }
            }
            if (!found_layer) {
                err_msg = "unknown layer '" + combo.toggle_layer + "' referenced in combo toggle_layer";
                return false;
            }
        }
    }

    for (const auto& lyr : config.layers) {
        for (const auto& kv : lyr.mappings) {
            if (!kv.second.toggle_layer.empty()) {
                bool found_layer = false;
                for (const auto& l : config.layers) {
                    if (l.name == kv.second.toggle_layer) {
                        found_layer = true;
                        break;
                    }
                }
                if (!found_layer) {
                    err_msg = "unknown layer '" + kv.second.toggle_layer + "' referenced in layer '" + lyr.name + "' toggle_layer";
                    return false;
                }
            }
        }
    }

    std::vector<KeyCode> seen_one_shot;
    for (const auto& osk : config.one_shot_keys) {
        if (!osk.layer.empty()) {
            bool found_layer = false;
            for (const auto& lyr : config.layers) {
                if (lyr.name == osk.layer) {
                    found_layer = true;
                    break;
                }
            }
            if (!found_layer) {
                err_msg = "unknown layer '" + osk.layer + "' referenced in one_shot";
                return false;
            }
        }
        if (osk.modifier != 0 && !isModifier(osk.modifier)) {
            err_msg = "invalid modifier key '" + keyCodeToWord(osk.modifier) + "' in one_shot";
            return false;
        }
        if (std::find(seen_one_shot.begin(), seen_one_shot.end(), osk.key) != seen_one_shot.end()) {
            err_msg = "duplicate one_shot key: " + keyCodeToWord(osk.key);
            return false;
        }
        seen_one_shot.push_back(osk.key);
        for (const auto& th : config.tap_hold_keys) {
            if (th.key == osk.key) {
                err_msg = "key '" + keyCodeToWord(osk.key) + "' cannot be used in both tap_hold and one_shot";
                return false;
            }
        }
        for (const auto& combo : config.combos) {
            for (KeyCode k : combo.keys) {
                if (k == osk.key) {
                    err_msg = "key '" + keyCodeToWord(osk.key) + "' cannot be used in both combos and one_shot";
                    return false;
                }
            }
        }
    }

    for (const auto& th : config.tap_hold_keys) {
        if (th.tap_leader && config.leader.key == 0) {
            config.leader.key = th.key;
        }
    }

    if (has_leader_tag || !config.leader.sequences.empty()) {
        if (config.leader.timeout_us <= 0) {
            err_msg = "leader timeout must be positive";
            return false;
        }
        std::vector<std::vector<KeyCode>> seen_seqs;
        for (const auto& seq : config.leader.sequences) {
            if (seq.keys.empty()) {
                err_msg = "leader sequence cannot have empty keys";
                return false;
            }
            if (seq.out_keys.empty() && seq.text.empty() && seq.toggle_layer.empty()) {
                err_msg = "leader sequence requires an output action (keys, text, or toggle_layer)";
                return false;
            }
            if (!seq.toggle_layer.empty()) {
                bool found_layer = false;
                for (const auto& lyr : config.layers) {
                    if (lyr.name == seq.toggle_layer) {
                        found_layer = true;
                        break;
                    }
                }
                if (!found_layer) {
                    err_msg = "unknown layer '" + seq.toggle_layer + "' referenced in leader toggle_layer";
                    return false;
                }
            }
            if (!seq.text.empty()) {
                for (char ch : seq.text) {
                    KeyCode kc = 0;
                    bool shift = false;
                    if (!asciiToKeyStroke(ch, kc, shift)) {
                        err_msg = "unsupported character in leader text snippet: '" + std::string(1, ch) + "'";
                        return false;
                    }
                }
            }
            for (const auto& prev : seen_seqs) {
                if (seq.keys == prev) {
                    err_msg = "duplicate leader sequence";
                    return false;
                }
                if (prev.size() < seq.keys.size() &&
                    std::equal(prev.begin(), prev.end(), seq.keys.begin())) {
                    err_msg = "leader sequence is a prefix of another sequence";
                    return false;
                }
                if (seq.keys.size() < prev.size() &&
                    std::equal(seq.keys.begin(), seq.keys.end(), prev.begin())) {
                    err_msg = "leader sequence is a prefix of another sequence";
                    return false;
                }
            }
            seen_seqs.push_back(seq.keys);
        }

        if (config.leader.key != 0) {
            for (const auto& combo : config.combos) {
                for (KeyCode k : combo.keys) {
                    if (k == config.leader.key) {
                        err_msg = "key '" + keyCodeToWord(config.leader.key) + "' cannot be used in both combos and leader";
                        return false;
                    }
                }
            }
        }
    }

    if (has_auto_shift_tag) {
        if (config.auto_shift.keys.empty()) {
            addAutoShiftPreset("letters", config.auto_shift.keys);
        }
        std::sort(config.auto_shift.keys.begin(), config.auto_shift.keys.end());
        config.auto_shift.keys.erase(
            std::unique(config.auto_shift.keys.begin(), config.auto_shift.keys.end()),
            config.auto_shift.keys.end());
        if (config.auto_shift.timeout_us <= 0) {
            err_msg = "auto_shift timeout_ms must be positive";
            return false;
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
