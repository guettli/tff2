#include "tff_monitor.h"
#include "tff_key_codes.h"
#include <ctime>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace tff {

EventMonitor::EventMonitor(const MonitorOptions& options)
    : options_(options) {
}

void EventMonitor::attachToEngine(TFFEngine& engine) {
    engine.setTraceCallback([this](const TraceEvent& trace) {
        this->recordTrace(trace);
    });
}

void EventMonitor::recordTrace(const TraceEvent& trace) {
    pending_traces_.push_back(trace);
}

void EventMonitor::clearTrace() {
    pending_traces_.clear();
}

std::string EventMonitor::formatTimestamp(TimeVal time) {
    char buf[32];
    if (time.sec > 0) {
        std::time_t s = static_cast<std::time_t>(time.sec);
        struct tm tm_info;
        localtime_r(&s, &tm_info);
        std::snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d]",
                      tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
                      static_cast<int>((time.usec / 1000) % 1000));
    } else {
        int64_t total_ms = time.usec / 1000;
        int ms = static_cast<int>(total_ms % 1000);
        int total_sec = static_cast<int>(total_ms / 1000);
        int sec = total_sec % 60;
        int min = (total_sec / 60) % 60;
        int hour = total_sec / 3600;
        std::snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d]", hour, min, sec, ms);
    }
    return std::string(buf);
}

std::string EventMonitor::formatKey(KeyCode code) {
    std::string name = keyCodeToWord(code);
    if (name.empty() || name == "unknown") {
        return "(code: " + std::to_string(code) + ")";
    }
    return "'" + name + "' (code: " + std::to_string(code) + ")";
}

std::string EventMonitor::formatDelta(int64_t delta_ms) {
    if (delta_ms < 0) {
        return "";
    }
    if (delta_ms > 9999) {
        return "(>9.9s)";
    }
    return "(+" + std::to_string(delta_ms) + "ms)";
}

std::string EventMonitor::formatEvent(const Event& ev, const std::string& device_label) {
    if (ev.type != EV_KEY) {
        clearTrace();
        return "";
    }

    int64_t curr_us = ev.time.toMicros();
    int64_t delta_ms = -1;
    if (prev_event_time_us_ > 0 && curr_us >= prev_event_time_us_) {
        delta_ms = (curr_us - prev_event_time_us_) / 1000;
        if (delta_ms > options_.max_delta_ms) {
            delta_ms = -1; // Reset delta after long idle
        }
    }
    prev_event_time_us_ = curr_us;

    std::ostringstream oss;

    // 1. Timestamp
    std::string ts_plain = formatTimestamp(ev.time);
    if (options_.color) {
        oss << "\033[90m" << ts_plain << "\033[0m";
    } else {
        oss << ts_plain;
    }
    oss << "  ";

    // Optional device label
    if (!device_label.empty()) {
        if (options_.color) {
            oss << "\033[36m[" << device_label << "]\033[0m ";
        } else {
            oss << "[" << device_label << "] ";
        }
    }

    // 2. Action (DOWN, UP, REPEAT)
    std::string action_plain;
    if (ev.value == KEY_VAL_DOWN) action_plain = "DOWN";
    else if (ev.value == KEY_VAL_UP) action_plain = "UP";
    else action_plain = "REPEAT";

    if (options_.color) {
        if (ev.value == KEY_VAL_DOWN) {
            oss << "\033[1;32mDOWN\033[0m  ";
        } else if (ev.value == KEY_VAL_UP) {
            oss << "\033[33mUP\033[0m    ";
        } else {
            oss << "\033[35mREPEAT\033[0m";
        }
    } else {
        oss << std::left << std::setw(6) << action_plain;
    }
    oss << "  ";

    // 3. Key Name & Code: e.g. "'d' (code: 32)"
    std::string key_plain = formatKey(ev.code);
    std::string key_name = keyCodeToWord(ev.code);
    if (options_.color) {
        if (!key_name.empty() && key_name != "unknown") {
            oss << "'\033[1;37m" << key_name << "\033[0m' \033[90m(code: " << ev.code << ")\033[0m";
        } else {
            oss << "\033[90m(code: " << ev.code << ")\033[0m";
        }
    } else {
        oss << key_plain;
    }

    int key_width = 16;
    int pad = key_width - static_cast<int>(key_plain.size());
    if (pad < 0) pad = 0;
    oss << std::string(pad, ' ');
    oss << "  ";

    // 4. Delta ms: e.g. "(+25ms)"
    std::string delta_plain = formatDelta(delta_ms);
    if (!delta_plain.empty() && options_.show_deltas) {
        if (options_.color) {
            oss << "\033[35m" << delta_plain << "\033[0m";
        } else {
            oss << delta_plain;
        }
        int dpad = 8 - static_cast<int>(delta_plain.size());
        if (dpad < 0) dpad = 0;
        oss << std::string(dpad, ' ');
    } else {
        oss << "        "; // 8 spaces
    }
    oss << "  ";

    // Calculate indent width for secondary lines
    // ts (14) + 2 + [dev] + action (6) + 2 + key (key_width) + 2 + delta (8) + 2
    int base_indent = 14 + 2 + (device_label.empty() ? 0 : (static_cast<int>(device_label.size()) + 3)) + 6 + 2 + key_width + 2 + 8 + 2;

    // 5. Annotations
    std::vector<const TraceEvent*> annot_traces;
    std::vector<const TraceEvent*> emit_traces;

    for (const auto& tr : pending_traces_) {
        if (tr.kind == TraceEvent::Kind::EmitKey ||
            tr.kind == TraceEvent::Kind::EmitText ||
            tr.kind == TraceEvent::Kind::EmitMouse) {
            emit_traces.push_back(&tr);
        } else {
            annot_traces.push_back(&tr);
        }
    }

    std::string indent(base_indent, ' ');
    for (size_t i = 0; i < annot_traces.size(); ++i) {
        const auto* tr = annot_traces[i];
        if (i > 0) {
            oss << indent;
        }

        std::string tag = tr->text;
        bool is_swallowed = (tr->kind == TraceEvent::Kind::KeySwallowed);

        if (options_.color) {
            if (is_swallowed) {
                oss << "\033[90m" << tag << "\033[0m";
            } else if (tr->kind == TraceEvent::Kind::TriggerCombo) {
                oss << "\033[1;33m-> " << tag << "\033[0m";
            } else if (tr->kind == TraceEvent::Kind::ChordCandidate) {
                oss << "\033[34m-> " << tag << "\033[0m";
            } else if (tr->kind == TraceEvent::Kind::LayerActive ||
                       tr->kind == TraceEvent::Kind::LayerInactive) {
                oss << "\033[36m-> " << tag << "\033[0m";
            } else if (tr->kind == TraceEvent::Kind::LeaderTrigger ||
                       tr->kind == TraceEvent::Kind::LeaderCandidate ||
                       tr->kind == TraceEvent::Kind::LeaderActive ||
                       tr->kind == TraceEvent::Kind::LeaderCancel) {
                oss << "\033[35m-> " << tag << "\033[0m";
            } else if (tr->kind == TraceEvent::Kind::TapHoldHold ||
                       tr->kind == TraceEvent::Kind::TapHoldTap ||
                       tr->kind == TraceEvent::Kind::TapHoldWait) {
                oss << "\033[36m-> " << tag << "\033[0m";
            } else if (tr->kind == TraceEvent::Kind::AutoShiftHold ||
                       tr->kind == TraceEvent::Kind::AutoShiftTap) {
                oss << "\033[33m-> " << tag << "\033[0m";
            } else {
                oss << "-> " << tag;
            }
        } else {
            if (is_swallowed) {
                oss << tag;
            } else {
                oss << "-> " << tag;
            }
        }
        oss << "\n";
    }

    if (annot_traces.empty()) {
        oss << "\n";
    }

    // Secondary emitted lines
    if (options_.show_emitted) {
        for (const auto* tr : emit_traces) {
            oss << indent;
            if (options_.color) {
                oss << "\033[1;32m-> " << tr->text << "\033[0m\n";
            } else {
                oss << "-> " << tr->text << "\n";
            }
        }
    }

    clearTrace();
    return oss.str();
}

std::string EventMonitor::formatTimer(TimeVal time) {
    if (pending_traces_.empty()) {
        return "";
    }

    std::ostringstream oss;
    std::string ts_plain = formatTimestamp(time);
    if (options_.color) {
        oss << "\033[90m" << ts_plain << "\033[0m";
    } else {
        oss << ts_plain;
    }
    oss << "  ";

    if (options_.color) {
        oss << "\033[1;33mTIMER EXPIRED\033[0m";
    } else {
        oss << "TIMER EXPIRED";
    }

    // Indent to annotation column: 52 - 14 - 2 - 13 = 23
    constexpr int kTimerPad = 23;
    oss << std::string(kTimerPad, ' ');

    std::vector<const TraceEvent*> annot_traces;
    std::vector<const TraceEvent*> emit_traces;

    for (const auto& tr : pending_traces_) {
        if (tr.kind == TraceEvent::Kind::EmitKey ||
            tr.kind == TraceEvent::Kind::EmitText ||
            tr.kind == TraceEvent::Kind::EmitMouse) {
            emit_traces.push_back(&tr);
        } else {
            annot_traces.push_back(&tr);
        }
    }

    std::string indent(52, ' ');
    for (size_t i = 0; i < annot_traces.size(); ++i) {
        const auto* tr = annot_traces[i];
        if (i > 0) {
            oss << indent;
        }
        if (options_.color) {
            oss << "\033[36m-> " << tr->text << "\033[0m\n";
        } else {
            oss << "-> " << tr->text << "\n";
        }
    }

    if (annot_traces.empty()) {
        oss << "\n";
    }

    if (options_.show_emitted) {
        for (const auto* tr : emit_traces) {
            oss << indent;
            if (options_.color) {
                oss << "\033[1;32m-> " << tr->text << "\033[0m\n";
            } else {
                oss << "-> " << tr->text << "\n";
            }
        }
    }

    clearTrace();
    return oss.str();
}

} // namespace tff
