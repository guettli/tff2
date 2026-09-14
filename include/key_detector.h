#ifndef KEY_DETECTOR_H
#define KEY_DETECTOR_H

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include "key_events.h"

/**
 * @brief Detects overlapping key presses and identifies combinations
 *
 * This class is platform-independent and focuses purely on the logic
 * of detecting when keys are pressed in overlapping time windows.
 */
class KeyDetector {
public:
    /**
     * @brief Constructor
     * @param overlap_threshold_ms Time threshold for considering keys overlapping (default 100ms)
     */
    explicit KeyDetector(uint32_t overlap_threshold_ms = 100);

    /**
     * @brief Process a key event (press or release)
     * @param key_code The key code (platform-specific)
     * @param timestamp_ms Timestamp in milliseconds
     * @param is_pressed true if key is pressed, false if released
     */
    void processKeyEvent(uint32_t key_code, uint32_t timestamp_ms, bool is_pressed);

    /**
     * @brief Get the last detected key combination
     * @return The detected combination, or empty if none
     */
    KeyCombination getLastCombination() const;

    /**
     * @brief Clear the last detected combination
     */
    void clearLastCombination();

    /**
     * @brief Reset internal state (for testing)
     */
    void reset();

    /**
     * @brief Get current overlap threshold
     */
    uint32_t getOverlapThreshold() const { return overlap_threshold_ms_; }

private:
    struct KeyState {
        bool is_pressed;
        uint32_t press_timestamp;
        uint32_t release_timestamp;
    };

    uint32_t overlap_threshold_ms_;
    std::unordered_map<uint32_t, KeyState> key_states_;
    KeyCombination last_combination_;

    /**
     * @brief Check if any keys were pressed recently
     * @param current_key The currently pressed key
     * @param timestamp_ms Current timestamp
     * @return Pair of overlapping key and time difference, or (0,0) if none
     */
    std::pair<uint32_t, uint32_t> findOverlappingKeys(uint32_t current_key, uint32_t timestamp_ms) const;
};

#endif // KEY_DETECTOR_H