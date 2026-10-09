#include "debug_buffer.h"
#include <cstdio>
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

void DebugBuffer::recordInKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.keycode = static_cast<uint16_t>(code);
    entry.val = pressed ? 1 : 0;
    entry.dir = DebugDirection::IN;
    push(entry);
}

void DebugBuffer::recordOutKeyEvent(uint32_t ts_ms, KeyCode code, bool pressed) {
    DebugEntry entry;
    entry.timestamp_ms = ts_ms;
    entry.keycode = static_cast<uint16_t>(code);
    entry.val = pressed ? 1 : 0;
    entry.dir = DebugDirection::OUT;
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

std::string DebugBuffer::formatStateString(DebugDirection dir) const {
    auto entries = getEntries();
    std::string res;
    bool first = true;
    uint32_t prev_ts = 0;
    for (const auto& ev : entries) {
        if (ev.dir != dir) {
            continue;
        }
        std::string word = keyCodeToWord(ev.keycode);
        if (word == "unknown") {
            continue;
        }
        if (!first) {
            uint32_t delta_ms = (ev.timestamp_ms >= prev_ts) ? (ev.timestamp_ms - prev_ts) : 0;
            res += "(" + std::to_string(delta_ms) + "ms) ";
        }
        res += word + (ev.val ? "_" : "/") + " ";
        prev_ts = ev.timestamp_ms;
        first = false;
    }
    if (!res.empty() && res.back() == ' ') {
        res.pop_back();
    }
    return res;
}

std::string DebugBuffer::formatDump(uint32_t current_ts_ms) const {
    (void)current_ts_ms;
    std::string in_str = formatStateString(DebugDirection::IN);
    std::string out_str = formatStateString(DebugDirection::OUT);
    std::string res;
    res += "IN: " + in_str + "\n";
    res += "OUT: " + out_str + "\n";
    return res;
}

}  // namespace tff
