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

void TFFEngine::setTapHoldKeys(const std::vector<TapHoldKey>& keys) {
    tap_hold_keys_ = keys;
}

bool TFFEngine::hasActiveTimer() const {
    if (fake_active_timer_next_time_ < TimeVal::maxTime()) {
        return true;
    }
    for (const auto& ath : active_tap_holds_) {
        if (!ath.hold_emitted) {
            return true;
        }
    }
    return false;
}

TimeVal TFFEngine::getActiveTimerTime() const {
    TimeVal earliest = fake_active_timer_next_time_;
    for (const auto& ath : active_tap_holds_) {
        if (!ath.hold_emitted) {
            TimeVal th_time = TimeVal::fromMicros(ath.down_time.toMicros() + ath.config.timeout_us);
            if (th_time < earliest) {
                earliest = th_time;
            }
        }
    }
    return earliest;
}

void TFFEngine::reset() {
    for (const auto& ath : active_tap_holds_) {
        if (ath.hold_emitted && out_dev_) {
            writeKey(ath.config.hold_key, KEY_VAL_UP, ath.hold_down_time);
        }
    }
    active_tap_holds_.clear();
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

    if (fake_active_timer_) {
        while (hasActiveTimer() && getActiveTimerTime() <= ev.time) {
            TimeVal t = getActiveTimerTime();
            onTimer(t);
            if (hasActiveTimer() && getActiveTimerTime() <= t) {
                break;
            }
        }
    }

    if (ev.type != EV_KEY) {
        if (out_dev_) {
            out_dev_->writeOne(ev);
        }
        return true;
    }

    if (ev.value == KEY_VAL_REPEAT) {
        // skip repeats
        return true;
    }

    // Check if this key is configured as a tap-hold key
    const TapHoldKey* thk = nullptr;
    for (const auto& k : tap_hold_keys_) {
        if (k.key == ev.code) {
            thk = &k;
            break;
        }
    }

    auto ath_it = std::find_if(active_tap_holds_.begin(), active_tap_holds_.end(),
        [&](const ActiveTapHold& a) { return a.config.key == ev.code; });

    if (ev.value == KEY_VAL_DOWN) {
        // Any other key going DOWN while a tap-hold key is pending immediately promotes it to HOLD
        for (auto& ath : active_tap_holds_) {
            if (!ath.hold_emitted && ath.config.key != ev.code) {
                writeKey(ath.config.hold_key, KEY_VAL_DOWN, ev.time);
                ath.hold_down_time = ev.time;
                ath.hold_emitted = true;
            }
        }

        if (thk != nullptr) {
            if (ath_it == active_tap_holds_.end()) {
                ActiveTapHold ath;
                ath.config = *thk;
                ath.down_time = ev.time;
                ath.hold_emitted = false;
                active_tap_holds_.push_back(ath);
            }
            return true;
        }

        return handleDownChar(ev);
    } else if (ev.value == KEY_VAL_UP) {
        if (ath_it != active_tap_holds_.end()) {
            if (ath_it->hold_emitted) {
                writeKey(ath_it->config.hold_key, KEY_VAL_UP, ev.time);
            } else {
                writeKey(ath_it->config.tap_key, KEY_VAL_DOWN, ev.time);
                writeKey(ath_it->config.tap_key, KEY_VAL_UP, ev.time);
            }
            active_tap_holds_.erase(ath_it);
            return true;
        }

        return handleUpChar(ev);
    }

    return false;
}

void TFFEngine::finish() {
    for (const auto& ath : active_tap_holds_) {
        if (ath.hold_emitted) {
            writeKey(ath.config.hold_key, KEY_VAL_UP, ath.hold_down_time);
        } else {
            writeKey(ath.config.tap_key, KEY_VAL_DOWN, ath.down_time);
            writeKey(ath.config.tap_key, KEY_VAL_UP, ath.down_time);
        }
    }
    active_tap_holds_.clear();
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
    for (auto& ath : active_tap_holds_) {
        if (!ath.hold_emitted) {
            int64_t expire_us = ath.down_time.toMicros() + ath.config.timeout_us;
            if (expire_us <= time.toMicros()) {
                TimeVal expire_tv = TimeVal::fromMicros(expire_us);
                writeKey(ath.config.hold_key, KEY_VAL_DOWN, expire_tv);
                ath.hold_down_time = expire_tv;
                ath.hold_emitted = true;
            }
        }
    }

    if (fake_active_timer_next_time_ <= time) {
        fake_active_timer_next_time_ = TimeVal::maxTime();
        eval(time, "timer");
    }
}

bool TFFEngine::eval(TimeVal curr_time, const std::string& /*reason*/) {
    // 1. Check for swallowed key releases (held keys from finished combos)
    if (!swallow_keys_.empty()) {
        bool swallowed_any = false;
        for (auto it = swallow_keys_.begin(); it != swallow_keys_.end(); ) {
            KeyCode sw_key = *it;
            bool has_down = false;
            bool has_up = false;
            for (const auto& ev : buf_) {
                if (ev.code == sw_key) {
                    if (ev.value == KEY_VAL_DOWN) has_down = true;
                    else if (ev.value == KEY_VAL_UP) has_up = true;
                }
            }
            if (has_down && has_up) {
                buf_.erase(std::remove_if(buf_.begin(), buf_.end(),
                    [sw_key](const Event& e) { return e.code == sw_key; }), buf_.end());
                it = swallow_keys_.erase(it);
                swallowed_any = true;
            } else {
                ++it;
            }
        }
        if (swallowed_any) {
            bool all_swallow = !buf_.empty();
            for (const auto& ev : buf_) {
                if (ev.value != KEY_VAL_DOWN ||
                    std::find(swallow_keys_.begin(), swallow_keys_.end(), ev.code) == swallow_keys_.end()) {
                    all_swallow = false;
                    break;
                }
            }
            if (all_swallow || buf_.empty()) {
                return true;
            }
        }
    }

    // 2. Single character down-up check
    if (buf_.size() == 2 &&
        buf_[0].code == buf_[1].code &&
        buf_[0].value == KEY_VAL_DOWN && buf_[1].value == KEY_VAL_UP) {
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
            if (!seen_up.empty()) {
                msg = "Key released before all combo keys were pressed";
                return EvalResult::NoMatch;
            }
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
        if (!combo.text.empty()) {
            emitText(combo.text, buf_[0].time);
        } else {
            writeCombo(combo, buf_[0].time, KEY_VAL_DOWN);
        }
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
    for (KeyCode mu : missing_up) {
        if (std::find(swallow_keys_.begin(), swallow_keys_.end(), mu) == swallow_keys_.end()) {
            swallow_keys_.push_back(mu);
        }
    }

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
        if (combo.text.empty()) {
            writeCombo(combo, buf_[0].time, KEY_VAL_UP);
        }
    }
    buf_ = std::move(new_buf);
}

void TFFEngine::emitText(const std::string& text, TimeVal base_time) {
    TimeVal curr_time = base_time;
    for (char c : text) {
        KeyCode code = 0;
        bool shift = false;
        if (!asciiToKeyStroke(c, code, shift)) {
            continue;
        }

        if (shift) {
            Event s_down{curr_time, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN};
            writeEvent(s_down, "TextSnippet");
            curr_time = TimeVal::fromMicros(curr_time.toMicros() + 100);

            Event k_down{curr_time, EV_KEY, code, KEY_VAL_DOWN};
            writeEvent(k_down, "TextSnippet");
            curr_time = TimeVal::fromMicros(curr_time.toMicros() + 100);

            Event k_up{curr_time, EV_KEY, code, KEY_VAL_UP};
            writeEvent(k_up, "TextSnippet");
            curr_time = TimeVal::fromMicros(curr_time.toMicros() + 100);

            Event s_up{curr_time, EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_UP};
            writeEvent(s_up, "TextSnippet");
            curr_time = TimeVal::fromMicros(curr_time.toMicros() + 100);
        } else {
            Event k_down{curr_time, EV_KEY, code, KEY_VAL_DOWN};
            writeEvent(k_down, "TextSnippet");
            curr_time = TimeVal::fromMicros(curr_time.toMicros() + 100);

            Event k_up{curr_time, EV_KEY, code, KEY_VAL_UP};
            writeEvent(k_up, "TextSnippet");
            curr_time = TimeVal::fromMicros(curr_time.toMicros() + 100);
        }
    }
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

void TFFEngine::writeKey(KeyCode code, int32_t value, TimeVal time) {
    Event ev;
    ev.time = time;
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;
    writeEvent(ev, "TapHold");
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
        auto it = std::find(swallow_keys_.begin(), swallow_keys_.end(), ev.code);
        if (it != swallow_keys_.end()) {
            swallow_keys_.erase(it);
        }
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
