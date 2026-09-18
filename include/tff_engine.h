#ifndef TFF_ENGINE_H
#define TFF_ENGINE_H

#include "tff_types.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace tff {

enum class EvalResult {
    NoMatch,
    Error,
    ComboNotFinished,
    WriteUpKeys,
    AllDownKeysSeen,
    AllDownKeysSeenAndAlreadyWritten
};

struct TraceEvent {
    enum class Kind {
        ChordCandidate,
        TriggerCombo,
        KeySwallowed,
        EmitKey,
        EmitText,
        EmitMouse,
        LayerActive,
        LayerInactive,
        TapHoldWait,
        TapHoldTap,
        TapHoldHold,
        LeaderActive,
        LeaderCandidate,
        LeaderTrigger,
        LeaderCancel,
        AutoShiftTap,
        AutoShiftHold,
        OneShotArmed,
        Info
    };
    Kind kind = Kind::Info;
    std::string text;
    Event event;
};

using TraceCallback = std::function<void(const TraceEvent&)>;

class TFFEngine {
public:
    explicit TFFEngine(EventWriter* out_dev = nullptr, const std::vector<Combo>& combos = {});

    void setCombos(const std::vector<Combo>& combos);
    const std::vector<Combo>& getCombos() const { return all_combos_; }

    void setTapHoldKeys(const std::vector<TapHoldKey>& keys);
    const std::vector<TapHoldKey>& getTapHoldKeys() const { return tap_hold_keys_; }

    void setLayers(const std::vector<Layer>& layers);
    const std::vector<Layer>& getLayers() const { return layers_; }
    const std::vector<std::string>& getActiveLayers() const { return active_layer_stack_; }
    bool isLayerActive(const std::string& name) const;

    void setOneShotKeys(const std::vector<OneShotKey>& keys);
    const std::vector<OneShotKey>& getOneShotKeys() const { return one_shot_keys_; }

    void setLeaderConfig(const LeaderConfig& config);
    const LeaderConfig& getLeaderConfig() const { return leader_config_; }
    void activateLeader(TimeVal time);
    void cancelLeader(TimeVal time);
    bool isLeaderActive() const { return leader_active_; }
    const std::vector<KeyCode>& getLeaderBuffer() const { return leader_buffer_; }

    void armOneShotModifier(KeyCode mod, TimeVal time, int64_t timeout_us = 1500000LL);
    void armOneShotLayer(const std::string& layer, TimeVal time, int64_t timeout_us = 1500000LL);
    void disengageOneShots(TimeVal time);
    bool isOneShotModifierArmed(KeyCode mod) const;
    bool isOneShotLayerArmed(const std::string& layer) const;

    void setConfig(const Config& config);
    Config getConfig() const;

    void activateLayer(const std::string& name);
    void deactivateLayer(const std::string& name);
    void toggleLayer(const std::string& name);

    void setAutoShiftConfig(const AutoShiftConfig& config);
    const AutoShiftConfig& getAutoShiftConfig() const { return auto_shift_; }
    bool isAutoShiftKey(KeyCode code) const;
    bool isModifierActive() const;

    void setMouseConfig(const MouseConfig& config) { mouse_config_ = config; }
    const MouseConfig& getMouseConfig() const { return mouse_config_; }
    void emitMouseAction(const MouseAction& action, TimeVal time);
    void writeRel(uint16_t code, int32_t value, TimeVal time);

    void setSettings(const Settings& settings);
    const Settings& getSettings() const { return settings_; }
    void setComboTimeoutMs(int64_t ms);
    int64_t getComboTimeoutMs() const { return settings_.combo_timeout_ms; }

    void setEventWriter(EventWriter* out_dev) { out_dev_ = out_dev; }

    void setTraceCallback(TraceCallback cb) { trace_callback_ = std::move(cb); }
    const TraceCallback& getTraceCallback() const { return trace_callback_; }
    bool hasTraceCallback() const { return static_cast<bool>(trace_callback_); }
    void trace(TraceEvent::Kind kind, const std::string& text, const Event& ev = Event{}) const {
        if (trace_callback_) {
            trace_callback_(TraceEvent{kind, text, ev});
        }
    }

    void setFakeActiveTimer(bool fake) { fake_active_timer_ = fake; }

    // Feeds an event into the engine. Returns false if RFKILL or fatal error encountered.
    bool processEvent(const Event& ev);

    // Call when input stream finishes (EOF)
    void finish();

    // Reset internal state
    void reset();

    // Trigger timer event
    void onTimer(TimeVal time);

    // Check if an active timer is pending and get its expiration time
    bool hasActiveTimer() const;
    TimeVal getActiveTimerTime() const;

    // Buffer size limit to bound memory usage and prevent buffer exhaustion
    static constexpr size_t MAX_BUFFER_SIZE = 64;

    // Inspect current buffer size
    size_t getBufferSize() const { return buf_.size(); }

    // Evict oldest buffered event to enforce buffer bounds
    void evictOldestBufferedEvent();

    // Get string representation of buffer and state (matching Go state.String())
    std::string toString() const;

private:
    struct ActiveTapHold {
        TapHoldKey config;
        TimeVal down_time;
        TimeVal hold_down_time;
        bool hold_emitted = false;
    };

    struct HeldLayerRemap {
        KeyCode input_key = 0;
        std::vector<KeyCode> out_keys;
        bool is_text = false;
        MouseAction mouse;
    };

    struct ArmedOneShotModifier {
        KeyCode modifier = 0;
        TimeVal expire_time;
    };

    struct ArmedOneShotLayer {
        std::string layer;
        TimeVal expire_time;
    };

    EventWriter* out_dev_ = nullptr;
    std::vector<Combo> all_combos_;
    std::vector<TapHoldKey> tap_hold_keys_;
    std::vector<Layer> layers_;
    std::vector<OneShotKey> one_shot_keys_;
    std::vector<std::string> active_layer_stack_;
    std::unordered_map<KeyCode, HeldLayerRemap> held_layer_remaps_;
    std::vector<ActiveTapHold> active_tap_holds_;
    std::vector<ArmedOneShotModifier> armed_one_shot_modifiers_;
    std::vector<ArmedOneShotLayer> armed_one_shot_layers_;
    std::vector<KeyCode> active_one_shot_modifiers_down_;
    std::vector<std::string> active_one_shot_layers_deactivate_;
    KeyCode disengaging_trigger_key_ = 0;
    LeaderConfig leader_config_;
    bool leader_active_ = false;
    TimeVal leader_expire_time_;
    std::vector<KeyCode> leader_buffer_;
    std::vector<Event> leader_raw_events_;
    std::vector<KeyCode> leader_pending_releases_;
    std::vector<Event> buf_;
    std::vector<Combo> down_keys_written_;
    std::vector<KeyCode> swallow_keys_;
    std::unordered_set<KeyCode> physical_keys_down_;

    struct PendingAutoShift {
        KeyCode key = 0;
        TimeVal down_time;
        TimeVal expire_time;
        bool shifted_emitted = false;
    };

    struct HeldAutoShift {
        KeyCode key = 0;
        bool shifted = false;
    };

    AutoShiftConfig auto_shift_;
    MouseConfig mouse_config_;
    Settings settings_;
    PendingAutoShift pending_auto_shift_;
    std::vector<HeldAutoShift> auto_shift_held_;
    std::vector<KeyCode> active_modifiers_;
    TraceCallback trace_callback_;

    int64_t timeout_after_down_us_ = 150000;  // 150ms
    int64_t min_age_us_ = 140000;             // 140ms
    int64_t min_overlap_us_ = 40000;          // 40ms

    bool fake_active_timer_ = true;
    TimeVal fake_active_timer_next_time_;

    bool handleDownChar(const Event& ev);
    bool handleUpChar(const Event& ev);

    bool eval(TimeVal curr_time, const std::string& reason);
    EvalResult evalCombo(const Combo& combo, TimeVal curr_time, std::string& msg);
    std::string tooYoung(const Event& last_down, TimeVal curr_time);

    void writeComboDownKeys(const Combo& combo);
    void writeComboUpKeys(const Combo& combo);
    void writeCombo(const Combo& combo, TimeVal time, int32_t value);
    void emitText(const std::string& text, TimeVal base_time);
    void writeKey(KeyCode code, int32_t value, TimeVal time);
    void writeEvent(const Event& ev, const std::string& reason);
    void writeEventDirect(const Event& ev, const std::string& reason);

    void handleAutoShiftOrWrite(const Event& ev, const std::string& reason);
    void commitPendingAutoShiftUnshifted(TimeVal time);

    void flushBuffer(const std::string& reason);

    const LayerAction* findLayerAction(KeyCode code) const;
    void releaseHeldLayerRemaps(TimeVal time);
};

}  // namespace tff

#endif  // TFF_ENGINE_H
