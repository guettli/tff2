#ifndef TFF_ENGINE_H
#define TFF_ENGINE_H

#include "tff_types.h"
#include <vector>
#include <string>

namespace tff {

enum class EvalResult {
    NoMatch,
    Error,
    ComboNotFinished,
    WriteUpKeys,
    AllDownKeysSeen,
    AllDownKeysSeenAndAlreadyWritten
};

class TFFEngine {
public:
    explicit TFFEngine(EventWriter* out_dev = nullptr, const std::vector<Combo>& combos = {});

    void setCombos(const std::vector<Combo>& combos);
    const std::vector<Combo>& getCombos() const { return all_combos_; }

    void setEventWriter(EventWriter* out_dev) { out_dev_ = out_dev; }

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
    bool hasActiveTimer() const {
        return fake_active_timer_next_time_ < TimeVal::maxTime();
    }
    TimeVal getActiveTimerTime() const {
        return fake_active_timer_next_time_;
    }

    // Get string representation of buffer and state (matching Go state.String())
    std::string toString() const;

private:
    EventWriter* out_dev_ = nullptr;
    std::vector<Combo> all_combos_;
    std::vector<Event> buf_;
    std::vector<Combo> down_keys_written_;
    std::vector<KeyCode> swallow_keys_;

    int64_t min_overlap_duration_us_ = 80000;    // 80ms
    int64_t timeout_after_down_us_   = 150000;   // 150ms
    int64_t min_age_us_              = 140000;   // 140ms
    int64_t min_overlap_us_          = 40000;    // 40ms

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
    void writeEvent(const Event& ev, const std::string& reason);

    void flushBuffer(const std::string& reason);
};

} // namespace tff

#endif // TFF_ENGINE_H
