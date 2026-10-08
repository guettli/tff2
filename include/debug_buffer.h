#ifndef TFF_DEBUG_BUFFER_H
#define TFF_DEBUG_BUFFER_H

#include "tff_key_codes.h"
#include "tff_types.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tff {

enum class DebugEventType : uint8_t {
    IN_RAW_REPORT = 0,  // Raw HID report from host keyboard (modifiers + keys[6])
    IN_KEY_EVENT = 1,   // Translated KeyCode fed into TFFEngine (code, down/up)
    TIMER_EXPIRED = 2,  // Engine timer expired (timestamp)
    OUT_KEY_EVENT = 3,  // KeyCode emitted by TFFEngine (code, down/up)
    OUT_RAW_REPORT = 4  // Raw HID report sent to host PC (modifiers + keys[6])
};

struct DebugEntry {
    uint32_t timestamp_ms = 0;
    DebugEventType type = DebugEventType::IN_RAW_REPORT;
    uint8_t modifiers = 0;
    uint8_t val = 0;            // 1 = down, 0 = up
    uint16_t keycode = 0;       // Linux KeyCode or USB keycode
    uint8_t raw_keys[6] = {0};  // Raw USB keycodes
};

/**
 * @brief Bounded, zero-heap event ring buffer for RP2040 diagnostics.
 *
 * Keeps the last CAPACITY events in static memory so users can trigger
 * a diagnostic dump of recent input/output transitions after observing
 * unexpected behavior.
 */
class DebugBuffer {
public:
    static constexpr size_t CAPACITY = 64;

    DebugBuffer() = default;

    // Recording functions
    void recordInRawReport(uint32_t ts_ms, uint8_t modifiers, const uint8_t* keys,
                           size_t key_count);
    void recordInKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed);
    void recordTimerExpired(uint32_t ts_ms);
    void recordOutKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed);
    void recordOutRawReport(uint32_t ts_ms, uint8_t modifiers, const uint8_t keys[6]);

    // Query & Formatting
    void clear();
    size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    std::vector<DebugEntry> getEntries() const;

    // Formats a human-readable text dump
    std::string formatDump(uint32_t current_ts_ms) const;

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
