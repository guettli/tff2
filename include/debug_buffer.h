#ifndef TFF_DEBUG_BUFFER_H
#define TFF_DEBUG_BUFFER_H

#include "tff_key_codes.h"
#include "tff_types.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tff {

enum class DebugDirection : uint8_t { IN = 0, OUT = 1 };

struct DebugEntry {
    uint32_t timestamp_ms = 0;
    uint16_t keycode = 0;
    uint8_t val = 0;  // 1 = down, 0 = up
    DebugDirection dir = DebugDirection::IN;
};

/**
 * @brief Bounded, zero-heap event ring buffer for RP2040 diagnostics.
 *
 * Keeps the last CAPACITY events in static memory so users can trigger
 * a diagnostic dump of recent input/output transitions after observing
 * unexpected behavior. Formats output directly as test-compatible state strings.
 */
class DebugBuffer {
public:
    static constexpr size_t CAPACITY = 64;

    DebugBuffer() = default;

    // Recording functions
    void recordInKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed);
    void recordOutKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed);

    // Query & Formatting
    void clear();
    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    std::vector<DebugEntry> getEntries() const;

    // Formats a human-readable, test-compatible pure text dump:
    // IN: <state_str>
    // OUT: <state_str>
    std::string formatDump(uint32_t current_ts_ms = 0) const;

    // Helper to format just IN or OUT events as a state string
    std::string formatStateString(DebugDirection dir) const;

    // Pause recording (used while typing out the dump so it does not overwrite the buffer)
    void setPaused(bool paused) { paused_ = paused; }
    bool isPaused() const { return paused_; }

private:
    DebugEntry buffer_[CAPACITY];
    size_t head_ = 0;   // Next write index
    size_t count_ = 0;  // Stored entry count
    bool paused_ = false;

    void push(const DebugEntry& entry);
};

}  // namespace tff

#endif  // TFF_DEBUG_BUFFER_H
