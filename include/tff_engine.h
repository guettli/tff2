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
        TapDanceTap,
        TapDanceHold,
        Reset,
        Info
    };
    Kind kind = Kind::Info;
    std::string text;
    Event event;
};

using TraceCallback = std::function<void(const TraceEvent&)>;
using LayerToggleCallback = std::function<void(const std::string& layer_name, bool active)>;

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
    bool isKeyInActiveCombo(KeyCode code) const;

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

    void setTapDances(const std::vector<TapDance>& tap_dances) { tap_dances_ = tap_dances; }
    const std::vector<TapDance>& getTapDances() const { return tap_dances_; }
    bool isTapDanceKey(KeyCode code) const;

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

    void setLayerToggleCallback(LayerToggleCallback cb) { layer_toggle_callback_ = std::move(cb); }
    const LayerToggleCallback& getLayerToggleCallback() const { return layer_toggle_callback_; }
    bool hasLayerToggleCallback() const { return static_cast<bool>(layer_toggle_callback_); }

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

    // Emergency state machine reset (automatically active when ESC is held continuously for 3.0s)
    static constexpr int64_t ESC_RESET_TIMEOUT_US = 3000000LL;
    bool isEscResetTriggered() const { return esc_reset_triggered_; }

    // Get string representation of buffer and state (matching Go state.String())
    std::string toString() const;

private:
    void triggerEscReset(TimeVal time);

    bool esc_hold_active_ = false;
    TimeVal esc_down_time_;
    bool esc_reset_triggered_ = false;
    bool esc_reset_swallow_release_ = false;

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
    // Keys of held combos (in down_keys_written_) that were taken out of buf_ because another
    // key was pressed or released meanwhile. Releasing one releases the combo output; the
    // releases of the other keys are swallowed.
    std::unordered_set<KeyCode> held_combo_keys_;
    std::vector<KeyCode> swallow_keys_;
    std::unordered_set<KeyCode> physical_keys_down_;
    // How often each output key is pressed by different sources (a combo, a tap-hold, a passed
    // through key). A key that is already down is released and pressed again instead of getting
    // a second DOWN; it goes up only when its last source releases it.
    std::unordered_map<KeyCode, int> output_press_count_;
    bool releasing_all_outputs_ = false;

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

    enum class TapDanceStage {
        IDLE,
        WAITING_FOR_RELEASE,   // Physical key DOWN, waiting for UP or hold timeout
        WAITING_FOR_NEXT_TAP,  // Physical key UP, waiting for subsequent DOWN or tap timeout
        HELD                   // Hold action emitted, waiting for physical key UP
    };

    struct ActiveTapDance {
        TapDance config;
        TapDanceStage stage = TapDanceStage::IDLE;
        int tap_count = 0;
        TimeVal start_time;
        TimeVal expire_time;
        std::vector<KeyCode> held_keys;
        std::string held_layer;
        MouseAction held_mouse;
    };

    std::vector<TapDance> tap_dances_;
    ActiveTapDance active_tap_dance_;

    const TapDance* findTapDance(KeyCode code) const;
    void executeTapDanceAction(const TapDanceAction& act, TimeVal time, bool is_hold);
    void releaseActiveTapDanceHold(TimeVal time);

    PendingAutoShift pending_auto_shift_;
    std::vector<HeldAutoShift> auto_shift_held_;
    std::vector<KeyCode> active_modifiers_;
    TraceCallback trace_callback_;
    LayerToggleCallback layer_toggle_callback_;

    int64_t timeout_after_down_us_ = 150000;  // 150ms
    int64_t min_age_us_ = 140000;             // 140ms
    int64_t min_overlap_us_ = 40000;          // 40ms

    bool fake_active_timer_ = true;
    TimeVal fake_active_timer_next_time_;

    // Permissive hold: while a tap-hold key is undecided, other key events wait here. Another
    // key pressed and released meanwhile makes it a hold; releasing the tap-hold key first (a
    // roll, e.g. typing "as" fast with home-row mods) makes it a tap. Then the events replay.
    std::vector<Event> deferred_events_;
    bool hasPendingTapHold() const;
    bool deferForPendingTapHold(const Event& ev);
    void promotePendingTapHolds(TimeVal time);
    void promoteExpiredTapHolds(TimeVal time);
    void emitTapHoldTap(const ActiveTapHold& ath, TimeVal time);
    void replayDeferredIfResolved();
    bool processEventNow(const Event& ev);

    bool handleDownChar(const Event& ev);
    bool handleUpChar(const Event& ev);

    bool eval(TimeVal curr_time, const std::string& reason);
    EvalResult evalCombo(const Combo& combo, TimeVal curr_time, std::string& msg);
    std::string tooYoung(const Event& last_down, TimeVal curr_time);

    void writeComboDownKeys(const Combo& combo);
    void writeComboUpKeys(const Combo& combo);
    void writeComboOutputUp(const Combo& combo, TimeVal time);
    void releaseCombosWithKey(KeyCode key, TimeVal time);
    void detachHeldCombos();
    bool releaseHeldComboKey(const Event& ev);
    void writeCombo(const Combo& combo, TimeVal time, int32_t value);
    void emitText(const std::string& text, TimeVal base_time);
    void writeKey(KeyCode code, int32_t value, TimeVal time);
    void writeEvent(const Event& ev, const std::string& reason);
    void writeEventDirect(const Event& ev, const std::string& reason);
    void writeKeyEvent(const Event& ev, const std::string& reason);

    void handleAutoShiftOrWrite(const Event& ev, const std::string& reason);
    void commitPendingAutoShiftUnshifted(TimeVal time);

    void flushBuffer(const std::string& reason);
    void emitBufferedEvent(const Event& ev, const std::string& reason);

    const LayerAction* findLayerAction(KeyCode code) const;
    void releaseHeldLayerRemaps(TimeVal time);
};

}  // namespace tff

#endif  // TFF_ENGINE_H
