#include "tff_app.h"
#include <iostream>

TFFApp::TFFApp(uint32_t overlap_threshold_ms)
    : key_detector_(std::make_unique<KeyDetector>(overlap_threshold_ms)),
      key_mapper_(std::make_unique<KeyMapper>()) {
}

std::vector<uint32_t> TFFApp::processKeyEvent(uint32_t key_code, uint32_t timestamp_ms, bool is_pressed) {
    // Process the key event through the detector
    key_detector_->processKeyEvent(key_code, timestamp_ms, is_pressed);

    // Check if we detected a combination
    KeyCombination combination = key_detector_->getLastCombination();
    if (combination.isValid()) {
        // Get the mapped output keys
        std::vector<uint32_t> output_keys = key_mapper_->getMappedKeys(combination);

        // Clear the combination so we don't process it again
        key_detector_->clearLastCombination();

        return output_keys;
    }

    // No combination detected or no mapping found
    return std::vector<uint32_t>();
}

bool TFFApp::loadConfiguration(const std::string& config_file) {
    return key_mapper_->loadConfiguration(config_file);
}

void TFFApp::reset() {
    key_detector_->reset();
    key_mapper_->clearMappings();
}