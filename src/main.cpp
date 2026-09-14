#include "../include/tff_app.h"
#include "../include/key_events.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "TFF-like Keyboard Remapping System\n";
    std::cout << "==================================\n\n";

    // Create the application
    TFFApp app(100); // 100ms overlap threshold

    // Load configuration (hardcoded for now)
    app.loadConfiguration("config.yaml");

    // Add some sample mappings for demonstration
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});
    app.getKeyMapper().addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE});

    std::cout << "Processing sample key events...\n\n";

    // Simulate some key events
    uint32_t timestamp = 0;

    // Event 1: Press F
    std::cout << "Pressing F...\n";
    auto output = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    if (!output.empty()) {
        std::cout << "  Output keys: ";
        for (auto key : output) {
            std::cout << key << " ";
        }
        std::cout << "\n";
    }

    timestamp += 50; // 50ms later

    // Event 2: Press J (overlapping with F)
    std::cout << "Pressing J (overlapping with F)...\n";
    output = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    if (!output.empty()) {
        std::cout << "  Output keys: ";
        for (auto key : output) {
            std::cout << key << " ";
        }
        std::cout << "\n";
    } else {
        std::cout << "  No combination detected\n";
    }

    timestamp += 50; // 50ms later

    // Event 3: Release F
    std::cout << "Releasing F...\n";
    output = app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    if (!output.empty()) {
        std::cout << "  Output keys: ";
        for (auto key : output) {
            std::cout << key << " ";
        }
        std::cout << "\n";
    }

    timestamp += 50; // 50ms later

    // Event 4: Release J
    std::cout << "Releasing J...\n";
    output = app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    if (!output.empty()) {
        std::cout << "  Output keys: ";
        for (auto key : output) {
            std::cout << key << " ";
        }
        std::cout << "\n";
    }

    std::cout << "\nSimulation complete.\n";

    return 0;
}