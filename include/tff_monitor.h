#ifndef TFF_MONITOR_H
#define TFF_MONITOR_H

#include "tff_types.h"
#include "tff_engine.h"
#include <string>
#include <vector>
#include <cstdint>

namespace tff {

/**
 * @brief Options for formatting live monitor events
 */
struct MonitorOptions {
    bool color = true;              ///< Enable ANSI terminal colors
    bool show_deltas = true;        ///< Show (+Xms) timing deltas between events
    bool show_emitted = true;       ///< Show emitted virtual keys and macros
    int64_t max_delta_ms = 99999;   ///< Maximum delta to display before clamping
};

/**
 * @brief Formats real-time input events and engine evaluations for interactive debugging
 */
class EventMonitor {
public:
    explicit EventMonitor(const MonitorOptions& options = MonitorOptions{});

    void setOptions(const MonitorOptions& options) { options_ = options; }
    const MonitorOptions& getOptions() const { return options_; }

    /**
     * @brief Attach this monitor to a TFFEngine instance to receive evaluation traces
     */
    void attachToEngine(TFFEngine& engine);

    /**
     * @brief Record a trace event from the engine
     */
    void recordTrace(const TraceEvent& trace);

    /**
     * @brief Clear all currently buffered trace events
     */
    void clearTrace();

    /**
     * @brief Format an incoming hardware event along with any engine evaluations that occurred
     * @param ev The input event
     * @param device_label Optional device identifier (e.g. "event8")
     * @return Formatted line(s) ending with newline
     */
    std::string formatEvent(const Event& ev, const std::string& device_label = "");

    /**
     * @brief Format a timer expiration event and any resulting engine evaluations
     * @param time Timestamp of timer expiration
     * @return Formatted line(s) ending with newline, or empty string if nothing triggered
     */
    std::string formatTimer(TimeVal time);

    /**
     * @brief Reset timing delta tracking (e.g. after long pause)
     */
    void resetTiming() { prev_event_time_us_ = -1; }

    /**
     * @brief Format timestamp as [HH:MM:SS.mmm]
     */
    static std::string formatTimestamp(TimeVal time);

    /**
     * @brief Format key name and evdev key code: e.g. "'d' (code: 32)"
     */
    static std::string formatKey(KeyCode code);

    /**
     * @brief Format delta milliseconds: e.g. "(+25ms)"
     */
    static std::string formatDelta(int64_t delta_ms);

private:
    MonitorOptions options_;
    int64_t prev_event_time_us_ = -1;
    std::vector<TraceEvent> pending_traces_;
};

} // namespace tff

#endif // TFF_MONITOR_H
