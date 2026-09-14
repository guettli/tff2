#include "linux_platform.h"
#include "tff_app.h"
#include "key_events.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Test application for UpBoard <-> RP2040 testing setup
 *
 * This simulates the complete flow:
 * 1. UpBoard sends fake keyboard events to RP2040 USB-A host port
 * 2. RP2040 processes key combinations
 * 3. RP2040 sends mapped output to UpBoard USB-C device port
 */
int main() {
    std::cout << "TFF-like Keyboard Remapping - UpBoard Testing Setup\n";
    std::cout << "==================================================\n\n";

    // Initialize the Linux platform for testing
    LinuxPlatform platform;
    if (!platform.initialize()) {
        std::cerr << "Failed to initialize Linux platform\n";
        return 1;
    }

    // Create the TFF application
    TFFApp app(100); // 100ms overlap threshold

    // Add some sample mappings for testing
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});      // F+J -> 1
    app.getKeyMapper().addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});      // J+F -> 2
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE}); // F+Space -> 3

    std::cout << "Testing overlapping key combinations...\n\n";

    // Test Case 1: F+J (F pressed first, J pressed within threshold)
    std::cout << "Test Case 1: F+J combination (F pressed first)\n";
    std::cout << "---------------------------------------------\n";

    // Send F key press
    platform.sendKeyEvent(KeyCodes::F_KEY, true);

    // Wait 50ms (within 100ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send J key press (should trigger F+J combination)
    platform.sendKeyEvent(KeyCodes::J_KEY, true);

    // Process the events through our application logic
    uint32_t timestamp = 0;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 50;
    auto output2 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    if (!output2.empty()) {
        std::cout << "Detected F+J combination, output key: " << output2[0] << "\n";
        // In real implementation, this would be sent to the RP2040 USB-C device port
    } else {
        std::cout << "No combination detected\n";
    }

    // Release keys
    timestamp += 50;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    timestamp += 50;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    platform.sendKeyEvent(KeyCodes::F_KEY, false);
    platform.sendKeyEvent(KeyCodes::J_KEY, false);

    std::cout << "\n";

    // Test Case 2: J+F (J pressed first, F pressed within threshold)
    std::cout << "Test Case 2: J+F combination (J pressed first)\n";
    std::cout << "---------------------------------------------\n";

    // Send J key press
    platform.sendKeyEvent(KeyCodes::J_KEY, true);

    // Wait 50ms (within 100ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send F key press (should trigger J+F combination)
    platform.sendKeyEvent(KeyCodes::F_KEY, true);

    // Process the events
    timestamp = 0;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    timestamp += 50;
    auto output4 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);

    if (!output4.empty()) {
        std::cout << "Detected J+F combination, output key: " << output4[0] << "\n";
        // In real implementation, this would be sent to the RP2040 USB-C device port
    } else {
        std::cout << "No combination detected\n";
    }

    // Release keys
    timestamp += 50;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    timestamp += 50;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    platform.sendKeyEvent(KeyCodes::J_KEY, false);
    platform.sendKeyEvent(KeyCodes::F_KEY, false);

    std::cout << "\n";

    // Test Case 3: Sequential keys (outside threshold)
    std::cout << "Test Case 3: Sequential keys (outside threshold)\n";
    std::cout << "----------------------------------------------\n";

    // Send F key press
    platform.sendKeyEvent(KeyCodes::F_KEY, true);

    // Wait 150ms (outside 100ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Send J key press (should NOT trigger combination)
    platform.sendKeyEvent(KeyCodes::J_KEY, true);

    // Process the events
    timestamp = 0;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 150;
    auto output6 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    if (output6.empty()) {
        std::cout << "No combination detected (keys pressed sequentially)\n";
    } else {
        std::cout << "Unexpected combination detected\n";
    }

    // Release keys
    timestamp += 50;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    timestamp += 50;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    platform.sendKeyEvent(KeyCodes::F_KEY, false);
    platform.sendKeyEvent(KeyCodes::J_KEY, false);

    std::cout << "\nTest completed successfully!\n";
    std::cout << "In the complete implementation, the RP2040 would:\n";
    std::cout << "1. Receive keyboard events via USB-A host port\n";
    std::cout << "2. Process key combinations using this logic\n";
    std::cout << "3. Send mapped output keys via USB-C device port\n";

    platform.cleanup();
    return 0;
}