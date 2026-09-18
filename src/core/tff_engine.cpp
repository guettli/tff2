#include "tff_engine.h"
#include "tff_key_codes.h"
#include <algorithm>
#include <iostream>
#include <map>

namespace tff {

TFFEngine::TFFEngine(EventWriter* out_dev, const std::vector<Combo>& combos)
    : out_dev_(out_dev), all_combos_(combos), fake_active_timer_next_time_(TimeVal::maxTime()) {}

void TFFEngine::setCombos(const std::vector<Combo>& combos) {
    all_combos_ = combos;
}

void TFFEngine::setTapHoldKeys(const std::vector<TapHoldKey>& keys) {
    tap_hold_keys_ = keys;
}

void TFFEngine::setLayers(const std::vector<Layer>& layers) {
    layers_ = layers;
}

void TFFEngine::setOneShotKeys(const std::vector<OneShotKey>& keys) {
    one_shot_keys_ = keys;
}

void TFFEngine::setLeaderConfig(const LeaderConfig& config) {
    leader_config_ = config;
}

void TFFEngine::setAutoShiftConfig(const AutoShiftConfig& config) {
    auto_shift_ = config;
    std::sort(auto_shift_.keys.begin(), auto_shift_.keys.end());
    auto_shift_.keys.erase(std::unique(auto_shift_.keys.begin(), auto_shift_.keys.end()),
                           auto_shift_.keys.end());
}

bool TFFEngine::isAutoShiftKey(KeyCode code) const {
    return std::binary_search(auto_shift_.keys.begin(), auto_shift_.keys.end(), code);
}

bool TFFEngine::isModifierActive() const {
    if (!active_modifiers_.empty() || !active_one_shot_modifiers_down_.empty() ||
        !armed_one_shot_modifiers_.empty()) {
        return true;
    }
    // Check dual-role tap-hold keys promoted to hold
    for (const auto& ath : active_tap_holds_) {
        if (ath.hold_emitted && isModifier(ath.config.hold_key)) {
            return true;
        }
    }
    // Check active layer remaps holding modifier keys
    for (const auto& kv : held_layer_remaps_) {
        for (KeyCode k : kv.second.out_keys) {
            if (isModifier(k)) {
                return true;
            }
        }
    }
    if (pending_auto_shift_.shifted_emitted) {
        return true;
    }
    for (const auto& h : auto_shift_held_) {
        if (h.shifted)
            return true;
    }
    return false;
}

void TFFEngine::setSettings(const Settings& settings) {
    settings_ = settings;
    min_overlap_us_ = settings.combo_timeout_ms * 1000LL;
}

void TFFEngine::setComboTimeoutMs(int64_t ms) {
    settings_.combo_timeout_ms = ms;
    min_overlap_us_ = ms * 1000LL;
}

void TFFEngine::setConfig(const Config& config) {
    setCombos(config.combos);
    setTapHoldKeys(config.tap_hold_keys);
    setLayers(config.layers);
    setOneShotKeys(config.one_shot_keys);
    setLeaderConfig(config.leader);
    setAutoShiftConfig(config.auto_shift);
    setMouseConfig(config.mouse);
    setSettings(config.settings);
}

Config TFFEngine::getConfig() const {
    Config cfg;
    cfg.combos = all_combos_;
    cfg.tap_hold_keys = tap_hold_keys_;
    cfg.layers = layers_;
    cfg.one_shot_keys = one_shot_keys_;
    cfg.leader = leader_config_;
    cfg.auto_shift = auto_shift_;
    cfg.mouse = mouse_config_;
    cfg.settings = settings_;
    return cfg;
}

void TFFEngine::emitMouseAction(const MouseAction& action, TimeVal time) {
    if (!action.isRelative() && !action.isButton())
        return;

    int16_t speed = (action.delta != 0) ? action.delta : mouse_config_.move_speed;
    int16_t wheel = (action.delta != 0) ? action.delta : mouse_config_.wheel_step;

    switch (action.type) {
        case MouseActionType::MoveLeft:
            writeRel(RelCodes::REL_X, -speed, time);
            break;
        case MouseActionType::MoveRight:
            writeRel(RelCodes::REL_X, speed, time);
            break;
        case MouseActionType::MoveUp:
            writeRel(RelCodes::REL_Y, -speed, time);
            break;
        case MouseActionType::MoveDown:
            writeRel(RelCodes::REL_Y, speed, time);
            break;
        case MouseActionType::WheelUp:
            writeRel(RelCodes::REL_WHEEL, wheel, time);
            break;
        case MouseActionType::WheelDown:
            writeRel(RelCodes::REL_WHEEL, -wheel, time);
            break;
        case MouseActionType::WheelLeft:
            writeRel(RelCodes::REL_HWHEEL, -wheel, time);
            break;
        case MouseActionType::WheelRight:
            writeRel(RelCodes::REL_HWHEEL, wheel, time);
            break;
        case MouseActionType::BtnLeft:
            writeKey(Keys::BTN_LEFT, KEY_VAL_DOWN, time);
            writeKey(Keys::BTN_LEFT, KEY_VAL_UP, time);
            break;
        case MouseActionType::BtnRight:
            writeKey(Keys::BTN_RIGHT, KEY_VAL_DOWN, time);
            writeKey(Keys::BTN_RIGHT, KEY_VAL_UP, time);
            break;
        case MouseActionType::BtnMiddle:
            writeKey(Keys::BTN_MIDDLE, KEY_VAL_DOWN, time);
            writeKey(Keys::BTN_MIDDLE, KEY_VAL_UP, time);
            break;
        case MouseActionType::BtnSide:
            writeKey(Keys::BTN_SIDE, KEY_VAL_DOWN, time);
            writeKey(Keys::BTN_SIDE, KEY_VAL_UP, time);
            break;
        case MouseActionType::BtnExtra:
            writeKey(Keys::BTN_EXTRA, KEY_VAL_DOWN, time);
            writeKey(Keys::BTN_EXTRA, KEY_VAL_UP, time);
            break;
        default:
            break;
    }
}

void TFFEngine::writeRel(uint16_t code, int32_t value, TimeVal time) {
    std::string axis_name = (code == RelCodes::REL_X)        ? "REL_X"
                            : (code == RelCodes::REL_Y)      ? "REL_Y"
                            : (code == RelCodes::REL_WHEEL)  ? "REL_WHEEL"
                            : (code == RelCodes::REL_HWHEEL) ? "REL_HWHEEL"
                                                             : std::to_string(code);
    if (trace_callback_) {
        trace(TraceEvent::Kind::EmitMouse, "EMIT REL: " + axis_name + " " + std::to_string(value));
    }
    if (out_dev_ == nullptr)
        return;
    Event ev_rel;
    ev_rel.time = time;
    ev_rel.type = EV_REL;
    ev_rel.code = code;
    ev_rel.value = value;
    out_dev_->writeOne(ev_rel);

    Event ev_syn;
    ev_syn.time = time;
    ev_syn.type = EV_SYN;
    ev_syn.code = SYN_REPORT;
    ev_syn.value = 0;
    out_dev_->writeOne(ev_syn);
}

void TFFEngine::activateLeader(TimeVal time) {
    leader_active_ = true;
    int64_t timeout = (leader_config_.timeout_us > 0) ? leader_config_.timeout_us : 1000000LL;
    leader_expire_time_ = TimeVal::fromMicros(time.toMicros() + timeout);
    leader_buffer_.clear();
    leader_raw_events_.clear();
    trace(TraceEvent::Kind::LeaderActive,
          "LEADER: active (timeout: " + std::to_string(timeout / 1000) + "ms)");
}

void TFFEngine::cancelLeader(TimeVal time) {
    (void)time;
    if (!leader_active_)
        return;
    leader_active_ = false;
    leader_buffer_.clear();
    auto to_replay = std::move(leader_raw_events_);
    leader_raw_events_.clear();
    trace(TraceEvent::Kind::LeaderCancel, "LEADER: cancelled");

    if (out_dev_) {
        for (const auto& raw_ev : to_replay) {
            out_dev_->writeOne(raw_ev);
            if (raw_ev.type == EV_KEY) {
                Event syn{raw_ev.time, EV_SYN, 0, 0};
                out_dev_->writeOne(syn);
            }
        }
    }
}

void TFFEngine::armOneShotModifier(KeyCode mod, TimeVal time, int64_t timeout_us) {
    trace(TraceEvent::Kind::OneShotArmed, "ONE-SHOT: armed osm(" + keyCodeToWord(mod) + ")");
    TimeVal expire = TimeVal::fromMicros(time.toMicros() + timeout_us);
    for (auto& armed : armed_one_shot_modifiers_) {
        if (armed.modifier == mod) {
            armed.expire_time = expire;
            return;
        }
    }
    armed_one_shot_modifiers_.push_back({mod, expire});
}

void TFFEngine::armOneShotLayer(const std::string& layer, TimeVal time, int64_t timeout_us) {
    trace(TraceEvent::Kind::OneShotArmed, "ONE-SHOT: armed osl(" + layer + ")");
    TimeVal expire = TimeVal::fromMicros(time.toMicros() + timeout_us);
    for (auto& armed : armed_one_shot_layers_) {
        if (armed.layer == layer) {
            armed.expire_time = expire;
            return;
        }
    }
    armed_one_shot_layers_.push_back({layer, expire});
    activateLayer(layer);
}

bool TFFEngine::isOneShotModifierArmed(KeyCode mod) const {
    for (const auto& armed : armed_one_shot_modifiers_) {
        if (armed.modifier == mod)
            return true;
    }
    return false;
}

bool TFFEngine::isOneShotLayerArmed(const std::string& layer) const {
    for (const auto& armed : armed_one_shot_layers_) {
        if (armed.layer == layer)
            return true;
    }
    return false;
}

void TFFEngine::disengageOneShots(TimeVal time) {
    if (out_dev_) {
        for (auto it = active_one_shot_modifiers_down_.rbegin();
             it != active_one_shot_modifiers_down_.rend(); ++it) {
            writeKey(*it, KEY_VAL_UP, time);
        }
    }
    active_one_shot_modifiers_down_.clear();

    for (const auto& lyr : active_one_shot_layers_deactivate_) {
        deactivateLayer(lyr);
    }
    active_one_shot_layers_deactivate_.clear();
    disengaging_trigger_key_ = 0;
}

void TFFEngine::activateLayer(const std::string& name) {
    if (active_layer_stack_.empty() || active_layer_stack_.back() != name) {
        active_layer_stack_.push_back(name);
        trace(TraceEvent::Kind::LayerActive, "LAYER: " + name + " (activated)");
    }
}

void TFFEngine::deactivateLayer(const std::string& name) {
    for (auto it = active_layer_stack_.rbegin(); it != active_layer_stack_.rend(); ++it) {
        if (*it == name) {
            active_layer_stack_.erase(std::next(it).base());
            trace(TraceEvent::Kind::LayerInactive, "LAYER: " + name + " (deactivated)");
            break;
        }
    }
}

void TFFEngine::toggleLayer(const std::string& name) {
    auto it = std::find(active_layer_stack_.begin(), active_layer_stack_.end(), name);
    if (it != active_layer_stack_.end()) {
        active_layer_stack_.erase(it);
        trace(TraceEvent::Kind::LayerInactive, "LAYER TOGGLE: " + name + " (off)");
    } else {
        active_layer_stack_.push_back(name);
        trace(TraceEvent::Kind::LayerActive, "LAYER TOGGLE: " + name + " (on)");
    }
}

bool TFFEngine::isLayerActive(const std::string& name) const {
    return std::find(active_layer_stack_.begin(), active_layer_stack_.end(), name) !=
           active_layer_stack_.end();
}

const LayerAction* TFFEngine::findLayerAction(KeyCode code) const {
    for (auto it = active_layer_stack_.rbegin(); it != active_layer_stack_.rend(); ++it) {
        const std::string& layer_name = *it;
        for (const auto& layer : layers_) {
            if (layer.name == layer_name) {
                auto m_it = layer.mappings.find(code);
                if (m_it != layer.mappings.end()) {
                    return &m_it->second;
                }
                break;
            }
        }
    }
    return nullptr;
}

void TFFEngine::releaseHeldLayerRemaps(TimeVal time) {
    if (out_dev_) {
        for (const auto& kv : held_layer_remaps_) {
            if (!kv.second.mouse.isRelative() && !kv.second.is_text) {
                for (auto it = kv.second.out_keys.rbegin(); it != kv.second.out_keys.rend(); ++it) {
                    writeKey(*it, KEY_VAL_UP, time);
                }
            }
        }
    }
    held_layer_remaps_.clear();
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
    if (!armed_one_shot_modifiers_.empty() || !armed_one_shot_layers_.empty()) {
        return true;
    }
    if (leader_active_) {
        return true;
    }
    if (auto_shift_.enabled && pending_auto_shift_.key != 0 &&
        !pending_auto_shift_.shifted_emitted) {
        return true;
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
    for (const auto& armed : armed_one_shot_modifiers_) {
        if (armed.expire_time < earliest) {
            earliest = armed.expire_time;
        }
    }
    for (const auto& armed : armed_one_shot_layers_) {
        if (armed.expire_time < earliest) {
            earliest = armed.expire_time;
        }
    }
    if (leader_active_ && leader_expire_time_ < earliest) {
        earliest = leader_expire_time_;
    }
    if (auto_shift_.enabled && pending_auto_shift_.key != 0 &&
        !pending_auto_shift_.shifted_emitted) {
        if (pending_auto_shift_.expire_time < earliest) {
            earliest = pending_auto_shift_.expire_time;
        }
    }
    return earliest;
}

void TFFEngine::reset() {
    for (const auto& ath : active_tap_holds_) {
        if (ath.hold_emitted && out_dev_) {
            if (ath.config.hold_key != 0) {
                writeKey(ath.config.hold_key, KEY_VAL_UP, ath.hold_down_time);
            }
        }
    }
    active_tap_holds_.clear();
    disengageOneShots(TimeVal{});
    for (const auto& armed : armed_one_shot_layers_) {
        deactivateLayer(armed.layer);
    }
    armed_one_shot_layers_.clear();
    armed_one_shot_modifiers_.clear();
    releaseHeldLayerRemaps(TimeVal{});
    active_layer_stack_.clear();
    leader_active_ = false;
    leader_buffer_.clear();
    leader_raw_events_.clear();
    leader_pending_releases_.clear();
    if (pending_auto_shift_.key != 0) {
        if (pending_auto_shift_.shifted_emitted) {
            writeKey(pending_auto_shift_.key, KEY_VAL_UP, TimeVal{});
            writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_UP, TimeVal{});
        }
        pending_auto_shift_ = {};
    }
    for (const auto& h : auto_shift_held_) {
        writeKey(h.key, KEY_VAL_UP, TimeVal{});
        if (h.shifted) {
            writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_UP, TimeVal{});
        }
    }
    auto_shift_held_.clear();
    active_modifiers_.clear();
    auto combos_to_release = down_keys_written_;
    for (const auto& combo : combos_to_release) {
        writeComboUpKeys(combo);
    }
    down_keys_written_.clear();
    buf_.clear();
    swallow_keys_.clear();
    physical_keys_down_.clear();
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

    if (ev.value == KEY_VAL_DOWN) {
        if (!physical_keys_down_.insert(ev.code).second) {
            // Drop duplicate down events (switch bounce / chatter)
            return true;
        }
    } else if (ev.value == KEY_VAL_UP) {
        if (physical_keys_down_.erase(ev.code) == 0) {
            // Drop spurious up events for keys not physically down
            return true;
        }
    }

    if (ev.value == KEY_VAL_REPEAT) {
        auto remap_it = held_layer_remaps_.find(ev.code);
        if (remap_it != held_layer_remaps_.end() && remap_it->second.mouse.isRelative()) {
            emitMouseAction(remap_it->second.mouse, ev.time);
            return true;
        }
        // skip repeats
        return true;
    }

    if (isModifier(ev.code)) {
        if (ev.value == KEY_VAL_DOWN) {
            if (std::find(active_modifiers_.begin(), active_modifiers_.end(), ev.code) ==
                active_modifiers_.end()) {
                active_modifiers_.push_back(ev.code);
            }
            if (pending_auto_shift_.key != 0) {
                commitPendingAutoShiftUnshifted(ev.time);
            }
        } else if (ev.value == KEY_VAL_UP) {
            auto it = std::find(active_modifiers_.begin(), active_modifiers_.end(), ev.code);
            if (it != active_modifiers_.end()) {
                active_modifiers_.erase(it);
            }
        }
    }

    if (ev.value == KEY_VAL_UP) {
        auto it =
            std::find(leader_pending_releases_.begin(), leader_pending_releases_.end(), ev.code);
        if (it != leader_pending_releases_.end()) {
            leader_pending_releases_.erase(it);
            return true;
        }
    }

    if (leader_active_ && ev.time >= leader_expire_time_) {
        cancelLeader(ev.time);
    }

    // Check if this key is configured as a tap-hold key or one-shot key
    TapHoldKey synthesized_th;
    const TapHoldKey* thk = nullptr;
    for (const auto& k : tap_hold_keys_) {
        if (k.key == ev.code) {
            thk = &k;
            break;
        }
    }
    if (thk == nullptr) {
        for (const auto& osk : one_shot_keys_) {
            if (osk.key == ev.code) {
                synthesized_th.key = osk.key;
                synthesized_th.tap_one_shot_modifier =
                    osk.modifier != 0 ? osk.modifier : (osk.layer.empty() ? osk.key : 0);
                synthesized_th.tap_one_shot_layer = osk.layer;
                synthesized_th.tap_one_shot_timeout_us = osk.timeout_us;
                synthesized_th.hold_key =
                    osk.layer.empty() ? synthesized_th.tap_one_shot_modifier : 0;
                synthesized_th.hold_layer = osk.layer;
                synthesized_th.timeout_us = 200000LL;
                thk = &synthesized_th;
                break;
            }
        }
    }

    if (leader_active_) {
        // Dedicated leader key pressed while leader is active -> cancel leader mode
        if (leader_config_.key != 0 && ev.code == leader_config_.key &&
            (thk == nullptr || !thk->tap_leader)) {
            if (ev.value == KEY_VAL_DOWN)
                return true;
            if (ev.value == KEY_VAL_UP) {
                cancelLeader(ev.time);
                return true;
            }
        }
        if (thk != nullptr && thk->tap_leader) {
            // Let tap-hold logic below process it (ath_it will cancel on tap or emit hold if held)
        } else if (ev.value == KEY_VAL_DOWN) {
            std::vector<KeyCode> candidate = leader_buffer_;
            candidate.push_back(ev.code);

            const LeaderSequence* matched_seq = nullptr;
            for (const auto& seq : leader_config_.sequences) {
                if (seq.keys == candidate) {
                    matched_seq = &seq;
                    break;
                }
            }

            bool is_prefix = false;
            for (const auto& seq : leader_config_.sequences) {
                if (seq.keys.size() > candidate.size() &&
                    std::equal(candidate.begin(), candidate.end(), seq.keys.begin())) {
                    is_prefix = true;
                    break;
                }
            }

            if (matched_seq != nullptr) {
                std::string seq_str;
                for (size_t k = 0; k < matched_seq->keys.size(); ++k) {
                    seq_str += (k > 0 ? " + " : "") + keyCodeToWord(matched_seq->keys[k]);
                }
                std::string act_str =
                    !matched_seq->text.empty() ? ("\"" + matched_seq->text + "\"")
                    : !matched_seq->toggle_layer.empty()
                        ? ("toggle_layer(" + matched_seq->toggle_layer + ")")
                    : (matched_seq->mouse.isRelative() || matched_seq->mouse.isButton())
                        ? mouseActionToWord(matched_seq->mouse)
                        : keyCodeToWord(matched_seq->out_keys.empty() ? 0
                                                                      : matched_seq->out_keys[0]);
                trace(TraceEvent::Kind::LeaderTrigger,
                      "LEADER TRIGGER: " + seq_str + " -> " + act_str);

                if (matched_seq->mouse.isRelative() || matched_seq->mouse.isButton()) {
                    emitMouseAction(matched_seq->mouse, ev.time);
                } else if (!matched_seq->toggle_layer.empty()) {
                    toggleLayer(matched_seq->toggle_layer);
                } else if (!matched_seq->text.empty()) {
                    emitText(matched_seq->text, ev.time);
                } else if (!matched_seq->out_keys.empty()) {
                    for (KeyCode out_k : matched_seq->out_keys) {
                        writeKey(out_k, KEY_VAL_DOWN, ev.time);
                    }
                    for (auto it = matched_seq->out_keys.rbegin();
                         it != matched_seq->out_keys.rend(); ++it) {
                        writeKey(*it, KEY_VAL_UP, ev.time);
                    }
                }

                std::map<KeyCode, int> press_counts;
                for (const auto& raw_ev : leader_raw_events_) {
                    if (raw_ev.value == KEY_VAL_DOWN)
                        press_counts[raw_ev.code]++;
                    else if (raw_ev.value == KEY_VAL_UP)
                        press_counts[raw_ev.code]--;
                }
                press_counts[ev.code]++;
                for (const auto& kv : press_counts) {
                    if (kv.second > 0) {
                        leader_pending_releases_.push_back(kv.first);
                    }
                }

                leader_active_ = false;
                leader_buffer_.clear();
                leader_raw_events_.clear();
                return true;
            } else if (is_prefix) {
                std::string cand_str;
                for (size_t k = 0; k < candidate.size(); ++k) {
                    cand_str += (k > 0 ? " " : "") + keyCodeToWord(candidate[k]);
                }
                trace(TraceEvent::Kind::LeaderCandidate, "LEADER CANDIDATE: " + cand_str);
                leader_buffer_.push_back(ev.code);
                leader_raw_events_.push_back(ev);
                int64_t timeout =
                    (leader_config_.timeout_us > 0) ? leader_config_.timeout_us : 1000000LL;
                leader_expire_time_ = TimeVal::fromMicros(ev.time.toMicros() + timeout);
                return true;
            } else {
                // Mismatch: cancel leader and replay buffered keys
                cancelLeader(ev.time);
                // Fall through to normal processing for ev!
            }
        } else if (ev.value == KEY_VAL_UP) {
            bool was_down_in_leader = false;
            for (const auto& raw : leader_raw_events_) {
                if (raw.code == ev.code && raw.value == KEY_VAL_DOWN) {
                    was_down_in_leader = true;
                    break;
                }
            }
            if (was_down_in_leader) {
                leader_raw_events_.push_back(ev);
                int64_t timeout =
                    (leader_config_.timeout_us > 0) ? leader_config_.timeout_us : 1000000LL;
                leader_expire_time_ = TimeVal::fromMicros(ev.time.toMicros() + timeout);
                return true;
            }
        }
    }

    auto ath_it = std::find_if(active_tap_holds_.begin(), active_tap_holds_.end(),
                               [&](const ActiveTapHold& a) { return a.config.key == ev.code; });

    if (ev.value == KEY_VAL_DOWN) {
        // Any other key going DOWN while a tap-hold key is pending immediately promotes it to HOLD
        for (auto& ath : active_tap_holds_) {
            if (!ath.hold_emitted && ath.config.key != ev.code) {
                std::string hold_desc = !ath.config.hold_layer.empty()
                                            ? ("layer(" + ath.config.hold_layer + ")")
                                            : keyCodeToWord(ath.config.hold_key);
                trace(TraceEvent::Kind::TapHoldHold, "TAP-HOLD: hold '" + hold_desc + "'");
                if (!ath.config.hold_layer.empty()) {
                    activateLayer(ath.config.hold_layer);
                } else if (ath.config.hold_key != 0) {
                    writeKey(ath.config.hold_key, KEY_VAL_DOWN, ev.time);
                }
                ath.hold_down_time = ev.time;
                ath.hold_emitted = true;
            }
        }

        // Dedicated leader key (not configured as tap_hold)
        if (leader_config_.key != 0 && ev.code == leader_config_.key &&
            (thk == nullptr || !thk->tap_leader)) {
            return true;
        }

        if (thk != nullptr) {
            if (ath_it == active_tap_holds_.end()) {
                ActiveTapHold ath;
                ath.config = *thk;
                ath.down_time = ev.time;
                ath.hold_emitted = false;
                active_tap_holds_.push_back(ath);
                trace(TraceEvent::Kind::TapHoldWait,
                      "TAP-HOLD: waiting (" + keyCodeToWord(thk->key) +
                          ", timeout: " + std::to_string(thk->timeout_us / 1000) + "ms)");
            }
            return true;
        }

        // Armed one-shots apply to this regular key
        if (!armed_one_shot_modifiers_.empty() || !armed_one_shot_layers_.empty()) {
            disengaging_trigger_key_ = ev.code;
            for (const auto& armed : armed_one_shot_modifiers_) {
                writeKey(armed.modifier, KEY_VAL_DOWN, ev.time);
                active_one_shot_modifiers_down_.push_back(armed.modifier);
            }
            armed_one_shot_modifiers_.clear();

            for (const auto& armed : armed_one_shot_layers_) {
                active_one_shot_layers_deactivate_.push_back(armed.layer);
            }
            armed_one_shot_layers_.clear();
        }

        // Check active layers
        if (!active_layer_stack_.empty()) {
            const LayerAction* action = findLayerAction(ev.code);
            if (action != nullptr) {
                HeldLayerRemap remap;
                remap.input_key = ev.code;
                if (action->mouse.isRelative()) {
                    remap.mouse = action->mouse;
                    emitMouseAction(action->mouse, ev.time);
                } else if (!action->toggle_layer.empty()) {
                    toggleLayer(action->toggle_layer);
                    remap.is_text = true;
                } else if (!action->text.empty()) {
                    remap.is_text = true;
                    emitText(action->text, ev.time);
                } else {
                    remap.is_text = false;
                    remap.out_keys = action->out_keys;
                    for (KeyCode out_k : action->out_keys) {
                        writeKey(out_k, KEY_VAL_DOWN, ev.time);
                    }
                }
                held_layer_remaps_[ev.code] = remap;
                return true;
            }
        }

        return handleDownChar(ev);
    } else if (ev.value == KEY_VAL_UP) {
        // Dedicated leader key (not configured as tap_hold)
        if (leader_config_.key != 0 && ev.code == leader_config_.key &&
            (thk == nullptr || !thk->tap_leader)) {
            activateLeader(ev.time);
            return true;
        }

        // Check if this key was remapped by an active layer when pressed
        auto remap_it = held_layer_remaps_.find(ev.code);
        if (remap_it != held_layer_remaps_.end()) {
            if (!remap_it->second.mouse.isRelative() && !remap_it->second.is_text) {
                for (auto it = remap_it->second.out_keys.rbegin();
                     it != remap_it->second.out_keys.rend(); ++it) {
                    writeKey(*it, KEY_VAL_UP, ev.time);
                }
            }
            held_layer_remaps_.erase(remap_it);
            if (disengaging_trigger_key_ == ev.code || disengaging_trigger_key_ == 0) {
                disengageOneShots(ev.time);
            }
            return true;
        }

        if (ath_it != active_tap_holds_.end()) {
            if (ath_it->hold_emitted) {
                trace(TraceEvent::Kind::KeySwallowed, "(swallowed)");
                if (!ath_it->config.hold_layer.empty()) {
                    deactivateLayer(ath_it->config.hold_layer);
                } else if (ath_it->config.hold_key != 0) {
                    writeKey(ath_it->config.hold_key, KEY_VAL_UP, ev.time);
                }
            } else {
                if (!ath_it->config.tap_toggle_layer.empty()) {
                    toggleLayer(ath_it->config.tap_toggle_layer);
                } else if (ath_it->config.tap_leader) {
                    if (leader_active_) {
                        cancelLeader(ev.time);
                    } else {
                        activateLeader(ev.time);
                    }
                } else if (ath_it->config.tap_one_shot_modifier != 0) {
                    armOneShotModifier(ath_it->config.tap_one_shot_modifier, ev.time,
                                       ath_it->config.tap_one_shot_timeout_us);
                } else if (!ath_it->config.tap_one_shot_layer.empty()) {
                    armOneShotLayer(ath_it->config.tap_one_shot_layer, ev.time,
                                    ath_it->config.tap_one_shot_timeout_us);
                } else if (ath_it->config.tap_key != 0) {
                    trace(TraceEvent::Kind::TapHoldTap,
                          "TAP-HOLD: tap '" + keyCodeToWord(ath_it->config.tap_key) + "'");
                    writeKey(ath_it->config.tap_key, KEY_VAL_DOWN, ev.time);
                    writeKey(ath_it->config.tap_key, KEY_VAL_UP, ev.time);
                }
            }
            active_tap_holds_.erase(ath_it);
            return true;
        }

        bool res = handleUpChar(ev);
        if (disengaging_trigger_key_ == ev.code || disengaging_trigger_key_ == 0) {
            disengageOneShots(ev.time);
        }
        return res;
    }

    return false;
}

void TFFEngine::finish() {
    for (const auto& ath : active_tap_holds_) {
        if (ath.hold_emitted) {
            if (ath.config.hold_key != 0) {
                writeKey(ath.config.hold_key, KEY_VAL_UP, ath.hold_down_time);
            }
        } else {
            if (!ath.config.tap_toggle_layer.empty()) {
                toggleLayer(ath.config.tap_toggle_layer);
            } else if (ath.config.tap_one_shot_modifier != 0) {
                armOneShotModifier(ath.config.tap_one_shot_modifier, ath.down_time,
                                   ath.config.tap_one_shot_timeout_us);
            } else if (!ath.config.tap_one_shot_layer.empty()) {
                armOneShotLayer(ath.config.tap_one_shot_layer, ath.down_time,
                                ath.config.tap_one_shot_timeout_us);
            } else if (ath.config.tap_key != 0) {
                writeKey(ath.config.tap_key, KEY_VAL_DOWN, ath.down_time);
                writeKey(ath.config.tap_key, KEY_VAL_UP, ath.down_time);
            }
        }
    }
    active_tap_holds_.clear();
    disengageOneShots(TimeVal{});
    for (const auto& armed : armed_one_shot_layers_) {
        deactivateLayer(armed.layer);
    }
    armed_one_shot_layers_.clear();
    armed_one_shot_modifiers_.clear();
    if (leader_active_) {
        cancelLeader(TimeVal{});
    }
    leader_pending_releases_.clear();
    releaseHeldLayerRemaps(TimeVal{});
    active_layer_stack_.clear();
    flushBuffer("EOF");
    if (pending_auto_shift_.key != 0) {
        if (pending_auto_shift_.shifted_emitted) {
            writeKey(pending_auto_shift_.key, KEY_VAL_UP, pending_auto_shift_.down_time);
            writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_UP, pending_auto_shift_.down_time);
        } else {
            writeKey(pending_auto_shift_.key, KEY_VAL_DOWN, pending_auto_shift_.down_time);
            writeKey(pending_auto_shift_.key, KEY_VAL_UP, pending_auto_shift_.down_time);
        }
        pending_auto_shift_ = {};
    }
    for (const auto& h : auto_shift_held_) {
        writeKey(h.key, KEY_VAL_UP, TimeVal{});
        if (h.shifted) {
            writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_UP, TimeVal{});
        }
    }
    auto_shift_held_.clear();
    auto combos_to_release = down_keys_written_;
    for (const auto& combo : combos_to_release) {
        writeComboUpKeys(combo);
    }
    down_keys_written_.clear();
    active_modifiers_.clear();
    physical_keys_down_.clear();
    swallow_keys_.clear();
}

void TFFEngine::evictOldestBufferedEvent() {
    if (buf_.empty()) {
        return;
    }
    Event oldest = buf_.front();
    buf_.erase(buf_.begin());
    handleAutoShiftOrWrite(oldest, "BufferOverflow>EvictOldest");
    auto it = std::find(swallow_keys_.begin(), swallow_keys_.end(), oldest.code);
    if (it != swallow_keys_.end()) {
        swallow_keys_.erase(it);
    }
}

bool TFFEngine::handleDownChar(const Event& ev) {
    if (fake_active_timer_) {
        fake_active_timer_next_time_ =
            TimeVal::fromMicros(ev.time.toMicros() + timeout_after_down_us_);
    }
    while (buf_.size() >= MAX_BUFFER_SIZE) {
        evictOldestBufferedEvent();
    }
    buf_.push_back(ev);
    return eval(ev.time, "down");
}

bool TFFEngine::handleUpChar(const Event& ev) {
    while (buf_.size() >= MAX_BUFFER_SIZE) {
        evictOldestBufferedEvent();
    }
    buf_.push_back(ev);
    return eval(ev.time, "up");
}

void TFFEngine::onTimer(TimeVal time) {
    if (auto_shift_.enabled && pending_auto_shift_.key != 0 &&
        !pending_auto_shift_.shifted_emitted) {
        if (time >= pending_auto_shift_.expire_time) {
            trace(
                TraceEvent::Kind::AutoShiftHold,
                "AUTO-SHIFT: hold '" + keyCodeToWord(pending_auto_shift_.key) + "' (capitalized)");
            writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN, pending_auto_shift_.expire_time);
            writeKey(pending_auto_shift_.key, KEY_VAL_DOWN, pending_auto_shift_.expire_time);
            pending_auto_shift_.shifted_emitted = true;
        }
    }
    for (auto& ath : active_tap_holds_) {
        if (!ath.hold_emitted) {
            int64_t expire_us = ath.down_time.toMicros() + ath.config.timeout_us;
            if (expire_us <= time.toMicros()) {
                TimeVal expire_tv = TimeVal::fromMicros(expire_us);
                std::string hold_desc = !ath.config.hold_layer.empty()
                                            ? ("layer(" + ath.config.hold_layer + ")")
                                            : keyCodeToWord(ath.config.hold_key);
                trace(TraceEvent::Kind::TapHoldHold, "TAP-HOLD: hold '" + hold_desc + "'");
                if (!ath.config.hold_layer.empty()) {
                    activateLayer(ath.config.hold_layer);
                } else if (ath.config.hold_key != 0) {
                    writeKey(ath.config.hold_key, KEY_VAL_DOWN, expire_tv);
                }
                ath.hold_down_time = expire_tv;
                ath.hold_emitted = true;
            }
        }
    }

    for (auto it = armed_one_shot_modifiers_.begin(); it != armed_one_shot_modifiers_.end();) {
        if (time >= it->expire_time) {
            it = armed_one_shot_modifiers_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = armed_one_shot_layers_.begin(); it != armed_one_shot_layers_.end();) {
        if (time >= it->expire_time) {
            deactivateLayer(it->layer);
            it = armed_one_shot_layers_.erase(it);
        } else {
            ++it;
        }
    }

    if (leader_active_ && time >= leader_expire_time_) {
        cancelLeader(time);
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
        for (auto it = swallow_keys_.begin(); it != swallow_keys_.end();) {
            KeyCode sw_key = *it;
            bool has_down = false;
            bool has_up = false;
            for (const auto& ev : buf_) {
                if (ev.code == sw_key) {
                    if (ev.value == KEY_VAL_DOWN)
                        has_down = true;
                    else if (ev.value == KEY_VAL_UP)
                        has_up = true;
                }
            }
            if (has_down && has_up) {
                trace(TraceEvent::Kind::KeySwallowed, "(swallowed)");
                buf_.erase(std::remove_if(buf_.begin(), buf_.end(),
                                          [sw_key](const Event& e) { return e.code == sw_key; }),
                           buf_.end());
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
                    std::find(swallow_keys_.begin(), swallow_keys_.end(), ev.code) ==
                        swallow_keys_.end()) {
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
    if (buf_.size() == 2 && buf_[0].code == buf_[1].code && buf_[0].value == KEY_VAL_DOWN &&
        buf_[1].value == KEY_VAL_UP) {
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

    auto formatComboTrigger = [](const Combo& c) {
        std::string desc;
        for (size_t k = 0; k < c.keys.size(); ++k) {
            desc += (k > 0 ? " + " : "") + keyCodeToWord(c.keys[k]);
        }
        desc += " -> ";
        if (!c.text.empty()) {
            desc += "\"" + c.text + "\"";
        } else if (!c.toggle_layer.empty()) {
            desc += "toggle_layer(" + c.toggle_layer + ")";
        } else if (c.mouse.isRelative() || c.mouse.isButton()) {
            desc += mouseActionToWord(c.mouse);
        } else {
            for (size_t k = 0; k < c.out_keys.size(); ++k) {
                desc += (k > 0 ? "+" : "") + keyCodeToWord(c.out_keys[k]);
            }
        }
        return desc;
    };

    // 3. Handle WriteUpKeys first
    bool found = false;
    for (size_t i = 0; i < codes.size(); ++i) {
        if (codes[i] != EvalResult::WriteUpKeys) {
            continue;
        }
        found = true;
        bool already_down = false;
        for (const auto& c : down_keys_written_) {
            if (c == all_combos_[i]) {
                already_down = true;
                break;
            }
        }
        if (!already_down && trace_callback_) {
            trace(TraceEvent::Kind::TriggerCombo,
                  "TRIGGER COMBO: " + formatComboTrigger(all_combos_[i]));
        }
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
        if (trace_callback_) {
            trace(TraceEvent::Kind::TriggerCombo,
                  "TRIGGER COMBO: " + formatComboTrigger(all_combos_[i]));
        }
        writeComboDownKeys(all_combos_[i]);
        down_keys_written_.push_back(all_combos_[i]);
    }
    if (found) {
        return true;
    }

    // 5. Handle ComboNotFinished and AllDownKeysSeenAndAlreadyWritten
    bool already_written = false;
    bool has_candidate = false;
    for (EvalResult code : codes) {
        if (code == EvalResult::AllDownKeysSeenAndAlreadyWritten) {
            already_written = true;
        } else if (code == EvalResult::ComboNotFinished) {
            has_candidate = true;
        }
    }
    if (already_written) {
        return true;
    }
    if (has_candidate) {
        if (trace_callback_) {
            std::vector<KeyCode> down_keys;
            for (const auto& ev : buf_) {
                if (ev.value == KEY_VAL_DOWN &&
                    std::find(down_keys.begin(), down_keys.end(), ev.code) == down_keys.end()) {
                    down_keys.push_back(ev.code);
                }
            }
            if (down_keys.size() >= 2) {
                std::string candidate_keys;
                for (size_t k = 0; k < down_keys.size(); ++k) {
                    candidate_keys += (k > 0 ? " + " : "") + keyCodeToWord(down_keys[k]);
                }
                trace(TraceEvent::Kind::ChordCandidate, "CHORD CANDIDATE: " + candidate_keys);
            }
        }
        return true;
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
    if (first_up_event && last_down_event && last_down_event->time < first_up_event->time &&
        last_down_event->code != first_up_event->code) {
        int64_t overlap = timeSubMicros(last_down_event->time, first_up_event->time);
        if (overlap < min_overlap_us_) {
            msg = "Overlap too short";
            return EvalResult::NoMatch;
        }
    }

    if (!seen_up.empty()) {
        msg = "WriteUpKeys";
        return EvalResult::WriteUpKeys;
    }

    if (last_down_event) {
        std::string too_young_msg = tooYoung(*last_down_event, curr_time);
        if (!too_young_msg.empty()) {
            msg = too_young_msg;
            return EvalResult::ComboNotFinished;
        }
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
    TimeVal event_time = !buf_.empty() ? buf_[0].time : TimeVal{};
    if (combo.mouse.isButton()) {
        KeyCode btn_code = (combo.mouse.type == MouseActionType::BtnRight)    ? Keys::BTN_RIGHT
                           : (combo.mouse.type == MouseActionType::BtnMiddle) ? Keys::BTN_MIDDLE
                           : (combo.mouse.type == MouseActionType::BtnSide)   ? Keys::BTN_SIDE
                           : (combo.mouse.type == MouseActionType::BtnExtra)  ? Keys::BTN_EXTRA
                                                                              : Keys::BTN_LEFT;
        writeKey(btn_code, KEY_VAL_DOWN, event_time);
    } else if (combo.mouse.isRelative()) {
        emitMouseAction(combo.mouse, event_time);
    } else if (!combo.toggle_layer.empty()) {
        toggleLayer(combo.toggle_layer);
    } else {
        if (!combo.text.empty()) {
            emitText(combo.text, event_time);
        } else {
            writeCombo(combo, event_time, KEY_VAL_DOWN);
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
    for (size_t k = 0; k < seen_up.size(); ++k) {
        trace(TraceEvent::Kind::KeySwallowed, "(swallowed)");
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

    TimeVal event_time = !buf_.empty() ? buf_[0].time : TimeVal{};
    if (combo.mouse.isButton()) {
        KeyCode btn_code = (combo.mouse.type == MouseActionType::BtnRight)    ? Keys::BTN_RIGHT
                           : (combo.mouse.type == MouseActionType::BtnMiddle) ? Keys::BTN_MIDDLE
                           : (combo.mouse.type == MouseActionType::BtnSide)   ? Keys::BTN_SIDE
                           : (combo.mouse.type == MouseActionType::BtnExtra)  ? Keys::BTN_EXTRA
                                                                              : Keys::BTN_LEFT;
        writeKey(btn_code, KEY_VAL_UP, event_time);
    } else if (combo.toggle_layer.empty() && combo.text.empty() && !combo.mouse.isRelative()) {
        writeCombo(combo, event_time, KEY_VAL_UP);
    }
    if (disengaging_trigger_key_ != 0) {
        disengageOneShots(event_time);
    }
    buf_ = std::move(new_buf);
}

void TFFEngine::emitText(const std::string& text, TimeVal base_time) {
    if (trace_callback_) {
        trace(TraceEvent::Kind::EmitText, "EMIT TEXT: \"" + text + "\"");
    }
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
    if (value == KEY_VAL_UP) {
        for (auto it = combo.out_keys.rbegin(); it != combo.out_keys.rend(); ++it) {
            Event ev;
            ev.time = time;
            ev.type = EV_KEY;
            ev.code = *it;
            ev.value = value;
            writeEvent(ev, "WriteCombo");
        }
    } else {
        for (KeyCode out_key : combo.out_keys) {
            Event ev;
            ev.time = time;
            ev.type = EV_KEY;
            ev.code = out_key;
            ev.value = value;
            writeEvent(ev, "WriteCombo");
        }
    }
}

void TFFEngine::writeKey(KeyCode code, int32_t value, TimeVal time) {
    Event ev;
    ev.time = time;
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;
    writeEventDirect(ev, "Key");
}

void TFFEngine::writeEventDirect(const Event& ev, const std::string& /*reason*/) {
    if (trace_callback_ && ev.type == EV_KEY) {
        trace(TraceEvent::Kind::EmitKey,
              "EMIT: " + keyCodeToWord(ev.code) + " (code: " + std::to_string(ev.code) + ", " +
                  (ev.value == KEY_VAL_DOWN ? "DOWN" : (ev.value == KEY_VAL_UP ? "UP" : "REPEAT")) +
                  ")",
              ev);
    }
    if (out_dev_) {
        out_dev_->writeOne(ev);
        Event syn;
        syn.time = ev.time;
        syn.type = EV_SYN;
        syn.code = SYN_REPORT;
        syn.value = 0;
        out_dev_->writeOne(syn);
    }
}

void TFFEngine::writeEvent(const Event& ev, const std::string& reason) {
    writeEventDirect(ev, reason);
}

void TFFEngine::commitPendingAutoShiftUnshifted(TimeVal time) {
    (void)time;
    if (pending_auto_shift_.key == 0)
        return;
    if (trace_callback_) {
        trace(TraceEvent::Kind::AutoShiftTap,
              "AUTO-SHIFT: tap '" + keyCodeToWord(pending_auto_shift_.key) + "'");
    }
    if (pending_auto_shift_.shifted_emitted) {
        auto_shift_held_.push_back({pending_auto_shift_.key, true});
    } else {
        writeKey(pending_auto_shift_.key, KEY_VAL_DOWN, pending_auto_shift_.down_time);
        auto_shift_held_.push_back({pending_auto_shift_.key, false});
    }
    pending_auto_shift_ = {};
}

void TFFEngine::handleAutoShiftOrWrite(const Event& ev, const std::string& reason) {
    if (!auto_shift_.enabled || ev.type != EV_KEY) {
        writeEventDirect(ev, reason);
        return;
    }

    if (ev.value == KEY_VAL_DOWN) {
        if (isModifierActive() || !isAutoShiftKey(ev.code)) {
            if (pending_auto_shift_.key != 0) {
                commitPendingAutoShiftUnshifted(ev.time);
            }
            writeEventDirect(ev, reason);
            return;
        }

        if (pending_auto_shift_.key != 0) {
            commitPendingAutoShiftUnshifted(ev.time);
        }
        pending_auto_shift_.key = ev.code;
        pending_auto_shift_.down_time = ev.time;
        pending_auto_shift_.expire_time =
            TimeVal::fromMicros(ev.time.toMicros() + auto_shift_.timeout_us);
        pending_auto_shift_.shifted_emitted = false;
        return;
    } else if (ev.value == KEY_VAL_UP) {
        if (pending_auto_shift_.key == ev.code) {
            if (pending_auto_shift_.shifted_emitted) {
                writeKey(ev.code, KEY_VAL_UP, ev.time);
                writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_UP, ev.time);
                pending_auto_shift_ = {};
                return;
            } else {
                // Tap! Released before timeout and before another key
                writeKey(ev.code, KEY_VAL_DOWN, pending_auto_shift_.down_time);
                writeKey(ev.code, KEY_VAL_UP, ev.time);
                pending_auto_shift_ = {};
                return;
            }
        }

        auto it = std::find_if(auto_shift_held_.begin(), auto_shift_held_.end(),
                               [&](const HeldAutoShift& h) { return h.key == ev.code; });
        if (it != auto_shift_held_.end()) {
            writeKey(ev.code, KEY_VAL_UP, ev.time);
            if (it->shifted) {
                writeKey(Keys::KEY_LEFTSHIFT, KEY_VAL_UP, ev.time);
            }
            auto_shift_held_.erase(it);
            return;
        }

        writeEventDirect(ev, reason);
        return;
    } else {
        writeEventDirect(ev, reason);
    }
}

void TFFEngine::flushBuffer(const std::string& reason) {
    for (const auto& ev : buf_) {
        handleAutoShiftOrWrite(ev, reason + ">FlushBuffer");
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

}  // namespace tff
