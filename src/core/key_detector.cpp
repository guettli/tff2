#include "key_detector.h"
#include <algorithm>
#include <iostream>

KeyDetector::KeyDetector(uint32_t overlap_threshold_ms)
    : overlap_threshold_ms_(overlap_threshold_ms) {
    key_states_.clear();
    last_combination_ = KeyCombination();
}

void KeyDetector::processKeyEvent(uint32_t key_code, uint32_t timestamp_ms, bool is_pressed) {
    // Update key state
    auto& state = key_states_[key_code];
    state.is_pressed = is_pressed;

    if (is_pressed) {
        state.press_timestamp = timestamp_ms;

        // Check for overlapping key presses
        auto overlapping = findOverlappingKeys(key_code, timestamp_ms);
        if (overlapping.first != 0) {
            // We found an overlapping key combination
            last_combination_ = KeyCombination(overlapping.first, key_code, overlapping.second);
        }
    } else {
        state.release_timestamp = timestamp_ms;
    }
}

KeyCombination KeyDetector::getLastCombination() const {
    return last_combination_;
}

void KeyDetector::clearLastCombination() {
    last_combination_ = KeyCombination();
}

void KeyDetector::reset() {
    key_states_.clear();
    last_combination_ = KeyCombination();
}

std::pair<uint32_t, uint32_t> KeyDetector::findOverlappingKeys(uint32_t current_key, uint32_t timestamp_ms) const {
    // Check if any other keys were pressed recently
    for (const auto& pair : key_states_) {
        uint32_t key = pair.first;
        const KeyState& state = pair.second;

        // Skip the current key and keys that aren't pressed
        if (key == current_key || !state.is_pressed) {
            continue;
        }

        // Check if this key was pressed within the overlap threshold
        uint32_t time_diff = timestamp_ms - state.press_timestamp;
        if (time_diff < overlap_threshold_ms_) {
            return std::make_pair(key, time_diff);
        }
    }

    return std::make_pair(0, 0); // No overlapping key found
}