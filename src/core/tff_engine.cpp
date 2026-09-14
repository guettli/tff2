#include "tff_engine.h"
#include "tff_key_codes.h"
#include <algorithm>
#include <iostream>

namespace tff {

TFFEngine::TFFEngine(EventWriter* out_dev, const std::vector<Combo>& combos)
    : out_dev_(out_dev),
      all_combos_(combos),
      fake_active_timer_next_time_(TimeVal::maxTime()) {
}

void TFFEngine::setCombos(const std::vector<Combo>& combos) {
    all_combos_ = combos;
}

void TFFEngine::reset() {
    buf_.clear();
    down_keys_written_.clear();
    swallow_keys_.clear();
    fake_active_timer_next_time_ = TimeVal::maxTime();
}

bool TFFEngine::processEvent(const Event& ev) {
    if (ev.code == Keys::KEY_RFKILL) {
        // Used in unit tests to stop immediately without final FlushBuffer
        return false;
    }

    if (fake_active_timer_ && fake_active_timer_next_time_ < ev.time) {
        onTimer(fake_active_timer_next_time_);
        fake_active_timer_next_time_ = TimeVal::maxTime();
    }

    if (ev.type != EV_KEY) {
        if (out_dev_) {
            out_dev_->writeOne(ev);
        }
        return true;
    }

    switch (ev.value) {
    case KEY_VAL_UP:
        return handleUpChar(ev);
    case KEY_VAL_DOWN:
        return handleDownChar(ev);
    case KEY_VAL_REPEAT:
        // skip repeats
        return true;
    default:
        return false;
    }
}

void TFFEngine::finish() {
    flushBuffer("EOF");
}

bool TFFEngine::handleDownChar(const Event& ev) {
    if (fake_active_timer_) {
        fake_active_timer_next_time_ = TimeVal::fromMicros(ev.time.toMicros() + timeout_after_down_us_);
    }
    buf_.push_back(ev);
    return eval(ev.time, "down");
}

bool TFFEngine::handleUpChar(const Event& ev) {
    buf_.push_back(ev);
    return eval(ev.time, "up");
}

void TFFEngine::onTimer(TimeVal time) {
    fake_active_timer_next_time_ = TimeVal::maxTime();
    eval(time, "timer");
}

bool TFFEngine::eval(TimeVal curr_time, const std::string& /*reason*/) {
    // 1. Single character down-up check
    if (buf_.size() == 2 &&
        buf_[0].code == buf_[1].code &&
        buf_[0].value == KEY_VAL_DOWN && buf_[1].value == KEY_VAL_UP) {
        std::vector<KeyCode> new_swallow;
        bool do_swallow = false;
        for (KeyCode key : swallow_keys_) {
            if (buf_[0].code == key && buf_[1].code == key) {
                do_swallow = true;
                continue;
            }
            new_swallow.push_back(key);
        }
        if (do_swallow) {
            swallow_keys_ = std::move(new_swallow);
            buf_.clear();
            return true;
        }
        flushBuffer("Eval>up-down-of-singlechar");
        return true;
    }

    // 2. Evaluate all combos
    std::vector<EvalResult> codes;
    codes.reserve(all_combos_.size());
    for (const auto& combo : all_combos_) {
        std::string msg;
        EvalResult code = evalCombo(combo, curr_time, msg);
        codes.push_back(code);
    }

    // 3. Handle WriteUpKeys first
    bool found = false;
    for (size_t i = 0; i < codes.size(); ++i) {
        if (codes[i] != EvalResult::WriteUpKeys) {
            continue;
        }
        found = true;
        writeComboDownKeys(all_combos_[i]);
        writeComboUpKeys(all_combos_[i]);
    }
    if (found) {
        return true;
    }

    // 4. Handle AllDownKeysSeen
    for (size_t i = 0; i < codes.size(); ++i) {
        if (codes[i] != EvalResult::AllDownKeysSeen) {
            continue;
        }
        found = true;
        bool already = false;
        for (const auto& c : down_keys_written_) {
            if (c == all_combos_[i]) {
                already = true;
                break;
            }
        }
        if (already) {
            continue;
        }
        writeComboDownKeys(all_combos_[i]);
        down_keys_written_.push_back(all_combos_[i]);
    }
    if (found) {
        return true;
    }

    // 5. Handle ComboNotFinished
    for (EvalResult code : codes) {
        if (code == EvalResult::ComboNotFinished) {
            return true;
        }
    }

    // 6. No match: flush buffer
    flushBuffer("Eval>No-match");
    return true;
}

EvalResult TFFEngine::evalCombo(const Combo& combo, TimeVal curr_time, std::string& msg) {
    std::vector<KeyCode> seen_down;
    std::vector<KeyCode> seen_up;
    const Event* last_down_event = nullptr;
    const Event* first_up_event = nullptr;
    bool has_unknown = false;

    for (const auto& ev : buf_) {
        bool in_combo = false;
        for (KeyCode k : combo.keys) {
            if (ev.code == k) {
                in_combo = true;
                break;
            }
        }
        if (!in_combo) {
            has_unknown = true;
            break;
        }
        if (ev.value == KEY_VAL_DOWN) {
            last_down_event = &ev;
            seen_down.push_back(ev.code);
        } else if (ev.value == KEY_VAL_UP) {
            if (!first_up_event) {
                first_up_event = &ev;
            }
            seen_up.push_back(ev.code);
        }
    }

    if (seen_down.empty()) {
        msg = "No down-keys seen";
        return EvalResult::NoMatch;
    }
    if (has_unknown) {
        msg = "Unknown key in buffer";
        return EvalResult::NoMatch;
    }

    for (size_t i = 0; i < combo.keys.size(); ++i) {
        if (i >= seen_down.size()) {
            msg = "Not all down-keys seen";
            return EvalResult::ComboNotFinished;
        }
        if (seen_down[i] != combo.keys[i]) {
            msg = "Order is wrong";
            return EvalResult::NoMatch;
        }
    }

    // All down keys seen
    if (first_up_event && last_down_event &&
        last_down_event->time < first_up_event->time &&
        last_down_event->code != first_up_event->code) {
        int64_t overlap = timeSubMicros(last_down_event->time, first_up_event->time);
        if (overlap < min_overlap_us_) {
            msg = "Overlap too short";
            return EvalResult::NoMatch;
        }
    }

    if (last_down_event) {
        std::string too_young_msg = tooYoung(*last_down_event, curr_time);
        if (!too_young_msg.empty()) {
            msg = too_young_msg;
            return EvalResult::ComboNotFinished;
        }
    }

    if (!seen_up.empty()) {
        msg = "WriteUpKeys";
        return EvalResult::WriteUpKeys;
    }

    for (const auto& c : down_keys_written_) {
        if (c == combo) {
            return EvalResult::AllDownKeysSeenAndAlreadyWritten;
        }
    }

    msg = "All down seen. Write the out-down-keys";
    return EvalResult::AllDownKeysSeen;
}

std::string TFFEngine::tooYoung(const Event& last_down, TimeVal curr_time) {
    if (buf_.size() > 1) {
        if (buf_[buf_.size() - 2].code == last_down.code) {
            return "";
        }
    }
    int64_t age = timeSubMicros(last_down.time, curr_time);
    if (age < min_age_us_) {
        return "All down seen, but too young";
    }
    return "";
}

void TFFEngine::writeComboDownKeys(const Combo& combo) {
    for (const auto& c : down_keys_written_) {
        if (c == combo) {
            return;
        }
    }
    if (!buf_.empty()) {
        writeCombo(combo, buf_[0].time, KEY_VAL_DOWN);
    }
}

void TFFEngine::writeComboUpKeys(const Combo& combo) {
    std::vector<Combo> new_written;
    for (const auto& c : down_keys_written_) {
        if (!(c == combo)) {
            new_written.push_back(c);
        }
    }
    down_keys_written_ = std::move(new_written);

    std::vector<KeyCode> seen_up;
    for (const auto& ev : buf_) {
        bool in_combo = false;
        for (KeyCode k : combo.keys) {
            if (ev.code == k) {
                in_combo = true;
                break;
            }
        }
        if (in_combo && ev.value == KEY_VAL_UP) {
            seen_up.push_back(ev.code);
        }
    }

    std::vector<KeyCode> missing_up;
    for (KeyCode k : combo.keys) {
        bool found = false;
        for (KeyCode su : seen_up) {
            if (k == su) {
                found = true;
                break;
            }
        }
        if (!found) {
            missing_up.push_back(k);
        }
    }
    swallow_keys_.insert(swallow_keys_.end(), missing_up.begin(), missing_up.end());

    std::vector<Event> new_buf;
    for (const auto& ev : buf_) {
        bool in_seen = false;
        for (KeyCode su : seen_up) {
            if (ev.code == su) {
                in_seen = true;
                break;
            }
        }
        if (!in_seen) {
            new_buf.push_back(ev);
        }
    }

    if (!buf_.empty()) {
        writeCombo(combo, buf_[0].time, KEY_VAL_UP);
    }
    buf_ = std::move(new_buf);
}

void TFFEngine::writeCombo(const Combo& combo, TimeVal time, int32_t value) {
    for (KeyCode out_key : combo.out_keys) {
        Event ev;
        ev.time = time;
        ev.type = EV_KEY;
        ev.code = out_key;
        ev.value = value;
        writeEvent(ev, "WriteCombo");
    }
}

void TFFEngine::writeEvent(const Event& ev, const std::string& /*reason*/) {
    if (out_dev_) {
        out_dev_->writeOne(ev);
        Event syn;
        syn.time = ev.time;
        syn.type = EV_SYN;
        syn.code = SYN_REPORT;
        syn.value = ev.value;
        out_dev_->writeOne(syn);
    }
}

void TFFEngine::flushBuffer(const std::string& reason) {
    for (const auto& ev : buf_) {
        writeEvent(ev, reason + ">FlushBuffer");
    }
    buf_.clear();
    fake_active_timer_next_time_ = TimeVal::maxTime();
}

std::string TFFEngine::toString() const {
    std::string s;
    for (const auto& ev : buf_) {
        s += keyCodeToWord(ev.code) + (ev.value == KEY_VAL_DOWN ? "_" : "/") + " ";
    }
    return s;
}

} // namespace tff
