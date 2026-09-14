#ifndef TFF_APP_H
#define TFF_APP_H

#include "key_detector.h"
#include "key_mapper.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Main application class that ties together key detection and mapping
 *
 * This class represents the core logic of the TFF-like application, combining
 * key detection and mapping in a platform-independent way.
 */
class TFFApp {
public:
    /**
     * @brief Constructor
     * @param overlap_threshold_ms Time threshold for overlapping detection
     */
    explicit TFFApp(uint32_t overlap_threshold_ms = 100);

    /**
     * @brief Process a key event
     * @param key_code The key code
     * @param timestamp_ms Timestamp in milliseconds
     * @param is_pressed true if pressed, false if released
     * @return Vector of output key codes if a combination was detected and mapped, empty otherwise
     */
    std::vector<uint32_t> processKeyEvent(uint32_t key_code, uint32_t timestamp_ms, bool is_pressed);

    /**
     * @brief Load configuration from file
     * @param config_file Path to configuration file
     * @return true if successful
     */
    bool loadConfiguration(const std::string& config_file);

    /**
     * @brief Get the key detector instance
     */
    KeyDetector& getKeyDetector() { return *key_detector_; }

    /**
     * @brief Get the key mapper instance
     */
    KeyMapper& getKeyMapper() { return *key_mapper_; }

    /**
     * @brief Reset the application state
     */
    void reset();

private:
    std::unique_ptr<KeyDetector> key_detector_;
    std::unique_ptr<KeyMapper> key_mapper_;
};

#endif // TFF_APP_H