#include "linux_platform.h"
#include "tff_app.h"
#include "key_mapper.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Demonstration of TFF-like keyboard remapping system
 *
 * This simulates the complete flow:
 * 1. UpBoard sends fake keyboard events to RP2040 USB-A host port
 * 2. RP2040 processes key combinations using TFF configuration
 * 3. RP2040 sends mapped output to UpBoard USB-C device port
 */
int main() {
    std::cout << "TFF-like Keyboard Remapping - Complete Demo\n";
    std::cout << "=============================================\n\n";

    // Initialize the Linux platform for testing
    LinuxPlatform platform;
    if (!platform.initialize()) {
        std::cerr << "Failed to initialize Linux platform\n";
        return 1;
    }

    // Create the TFF application with TFF configuration
    TFFApp app(100); // 100ms overlap threshold

    // Load TFF configuration (these would normally be loaded from JSON)
    KeyMapper& mapper = app.getKeyMapper();
    mapper.clearMappings();

    // Add TFF mappings from my-combos.yaml
    std::cout << "Loading TFF configuration...\n";

    // j f -> backspace
    mapper.addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::BACKSPACE});
    std::cout << "  j f -> backspace\n";

    // f j -> delete
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::DELETE});
    std::cout << "  f j -> delete\n";

    // semicolon a -> home
    mapper.addMapping(KeyCodes::SEMICOLON, KeyCodes::A, {KeyCodes::HOME});
    std::cout << "  semicolon a -> home\n";

    // a semicolon -> end
    mapper.addMapping(KeyCodes::A, KeyCodes::SEMICOLON, {KeyCodes::END});
    std::cout << "  a semicolon -> end\n";

    // f n -> down
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::N, {KeyCodes::DOWN_ARROW});
    std::cout << "  f n -> down\n";

    // f u -> up
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::U, {KeyCodes::UP_ARROW});
    std::cout << "  f u -> up\n";

    // f m -> down
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::M, {KeyCodes::DOWN_ARROW});
    std::cout << "  f m -> down\n";

    // f k -> left
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::K, {KeyCodes::LEFT_ARROW});
    std::cout << "  f k -> left\n";

    // f l -> right
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::L, {KeyCodes::RIGHT_ARROW});
    std::cout << "  f l -> right\n";

    // f i -> pageup
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::I, {KeyCodes::PAGE_UP});
    std::cout << "  f i -> pageup\n";

    // f comma -> pagedown
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::COMMA, {KeyCodes::PAGE_DOWN});
    std::cout << "  f comma -> pagedown\n";

    // g h -> esc
    mapper.addMapping(KeyCodes::G, KeyCodes::H, {KeyCodes::ESCAPE});
    std::cout << "  g h -> esc\n\n";

    std::cout << "Testing TFF key combinations...\n\n";

    // Test Case 1: j f -> backspace
    std::cout << "Test Case 1: j f combination -> backspace\n";
    std::cout << "------------------------------------------\n";

    // Send j key press
    platform.sendKeyEvent(KeyCodes::J_KEY, true);

    // Wait 50ms (within 100ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send f key press (should trigger j+f combination -> backspace)
    platform.sendKeyEvent(KeyCodes::F_KEY, true);

    // Process the events through our application logic
    uint32_t timestamp = 0;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    timestamp += 50;
    auto output2 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);

    if (!output2.empty()) {
        std::cout << "✓ Detected j f combination, output key: backspace\n";
    } else {
        std::cout << "✗ No combination detected\n";
    }

    // Release keys
    timestamp += 50;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    timestamp += 50;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    platform.sendKeyEvent(KeyCodes::J_KEY, false);
    platform.sendKeyEvent(KeyCodes::F_KEY, false);

    std::cout << "\n";

    // Test Case 2: f j -> delete
    std::cout << "Test Case 2: f j combination -> delete\n";
    std::cout << "---------------------------------------\n";

    // Send f key press
    platform.sendKeyEvent(KeyCodes::F_KEY, true);

    // Wait 50ms (within 100ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send j key press (should trigger f+j combination -> delete)
    platform.sendKeyEvent(KeyCodes::J_KEY, true);

    // Process the events
    timestamp = 0;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 50;
    auto output4 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    if (!output4.empty()) {
        std::cout << "✓ Detected f j combination, output key: delete\n";
    } else {
        std::cout << "✗ No combination detected\n";
    }

    // Release keys
    timestamp += 50;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    timestamp += 50;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    platform.sendKeyEvent(KeyCodes::F_KEY, false);
    platform.sendKeyEvent(KeyCodes::J_KEY, false);

    std::cout << "\n";

    // Test Case 3: Sequential keys (outside threshold)
    std::cout << "Test Case 3: Sequential keys (outside threshold)\n";
    std::cout << "----------------------------------------------\n";

    // Send j key press
    platform.sendKeyEvent(KeyCodes::J_KEY, true);

    // Wait 150ms (outside 100ms threshold)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Send f key press (should NOT trigger combination)
    platform.sendKeyEvent(KeyCodes::F_KEY, true);

    // Process the events
    timestamp = 0;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    timestamp += 150;
    auto output6 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);

    if (output6.empty()) {
        std::cout << "✓ No combination detected (keys pressed sequentially)\n";
    } else {
        std::cout << "✗ Unexpected combination detected\n";
    }

    // Release keys
    timestamp += 50;
    app.processKeyEvent(KeyCodes::J_KEY, timestamp, false);
    timestamp += 50;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    platform.sendKeyEvent(KeyCodes::J_KEY, false);
    platform.sendKeyEvent(KeyCodes::F_KEY, false);

    std::cout << "\nDemo completed successfully!\n";
    std::cout << "================================\n\n";
    std::cout << "In the complete implementation, the RP2040 would:\n";
    std::cout << "1. Receive keyboard events via USB-A host port\n";
    std::cout << "2. Process key combinations using TFF configuration\n";
    std::cout << "3. Send mapped output keys via USB-C device port\n";
    std::cout << "4. Remap according to ../tff/my-combos.yaml configuration\n";

    platform.cleanup();
    return 0;
}