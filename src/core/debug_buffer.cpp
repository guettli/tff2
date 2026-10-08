#include "debug_buffer.h"
#include <cstdio>
#include <iomanip>
#include <sstream>

namespace tff {

void DebugBuffer::push(const DebugEntry& entry) {
    if (paused_) {
        return;
    }
    buffer_[head_] = entry;
    head_ = (head_ + 1) % CAPACITY;
    if (count_ < CAPACITY) {
        count_++;
    }
}

void DebugBuffer::recordInRawReport(uint32_t ts_ms, uint8_t modifiers, const uint8_t* keys,
                                    size_t key_count) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.type = DebugEventType::IN_RAW_REPORT;
    entry.modifiers = modifiers;
    size_t limit = key_count > 6 ? 6 : key_count;
    for (size_t i = 0; i < limit; ++i) {
        entry.raw_keys[i] = keys[i];
    }
    push(entry);
}

void DebugBuffer::recordInKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.type = DebugEventType::IN_KEY_EVENT;
    entry.keycode = static_cast<uint16_t>(code);
    entry.val = pressed ? 1 : 0;
    push(entry);
}

void DebugBuffer::recordTimerExpired(uint32_t ts_ms) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.type = DebugEventType::TIMER_EXPIRED;
    push(entry);
}

void DebugBuffer::recordOutKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.type = DebugEventType::OUT_KEY_EVENT;
    entry.keycode = static_cast<uint16_t>(code);
    entry.val = pressed ? 1 : 0;
    push(entry);
}

void DebugBuffer::recordOutRawReport(uint32_t ts_ms, uint8_t modifiers, const uint8_t keys[6]) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.type = DebugEventType::OUT_RAW_REPORT;
    entry.modifiers = modifiers;
    if (keys) {
        for (size_t i = 0; i < 6; ++i) {
            entry.raw_keys[i] = keys[i];
        }
    }
    push(entry);
}

void DebugBuffer::clear() {
    head_ = 0;
    count_ = 0;
    paused_ = false;
}

std::vector<DebugEntry> DebugBuffer::getEntries() const {
    std::vector<DebugEntry> result;
    result.reserve(count_);
    if (count_ == 0) {
        return result;
    }
    size_t start = (count_ < CAPACITY) ? 0 : head_;
    for (size_t i = 0; i < count_; ++i) {
        result.push_back(buffer_[(start + i) % CAPACITY]);
    }
    return result;
}

std::string DebugBuffer::formatDump(uint32_t current_ts_ms) const {
    std::ostringstream oss;
    oss << "=== TFF RP2040 DEBUG DUMP ===\n";
    char header_buf[64];
    std::snprintf(header_buf, sizeof(header_buf), "Uptime: %u.%03us | Events: %zu\n",
                  current_ts_ms / 1000, current_ts_ms % 1000, count_);
    oss << header_buf;

    auto entries = getEntries();
    for (const auto& ev : entries) {
        char line_prefix[32];
        std::snprintf(line_prefix, sizeof(line_prefix), "[%4u.%03us] ", ev.timestamp_ms / 1000,
                      ev.timestamp_ms % 1000);
        oss << line_prefix;

        switch (ev.type) {
            case DebugEventType::IN_RAW_REPORT: {
                char raw_buf[64];
                std::snprintf(raw_buf, sizeof(raw_buf),
                              "IN_RAW : mod=%02X keys=[%02X %02X %02X %02X %02X %02X]\n",
                              ev.modifiers, ev.raw_keys[0], ev.raw_keys[1], ev.raw_keys[2],
                              ev.raw_keys[3], ev.raw_keys[4], ev.raw_keys[5]);
                oss << raw_buf;
                break;
            }
            case DebugEventType::IN_KEY_EVENT: {
                std::string k_name = codeName(EV_KEY, ev.keycode);
                oss << "IN_EV  : " << k_name << " " << (ev.val ? "DOWN" : "UP") << "\n";
                break;
            }
            case DebugEventType::TIMER_EXPIRED: {
                oss << "TIMER  : expired\n";
                break;
            }
            case DebugEventType::OUT_KEY_EVENT: {
                std::string k_name = codeName(EV_KEY, ev.keycode);
                oss << "OUT_EV : " << k_name << " " << (ev.val ? "DOWN" : "UP") << "\n";
                break;
            }
            case DebugEventType::OUT_RAW_REPORT: {
                char raw_buf[64];
                std::snprintf(raw_buf, sizeof(raw_buf),
                              "OUT_RAW: mod=%02X keys=[%02X %02X %02X %02X %02X %02X]\n",
                              ev.modifiers, ev.raw_keys[0], ev.raw_keys[1], ev.raw_keys[2],
                              ev.raw_keys[3], ev.raw_keys[4], ev.raw_keys[5]);
                oss << raw_buf;
                break;
            }
        }
    }
    oss << "=== END DUMP ===\n";
    return oss.str();
}

}  // namespace tff
