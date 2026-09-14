#include "tff_app.h"
#include "key_mapper.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing TFF Mappings Implementation\n";
    std::cout << "=====================================\n";

    // Create TFF application with 100ms threshold
    TFFApp app(100);

    // Load TFF configuration
    KeyMapper& mapper = app.getKeyMapper();
    mapper.clearMappings();

    // Add TFF mappings manually (these would normally be loaded from JSON)
    // j f -> backspace
    mapper.addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::BACKSPACE});

    // f j -> delete
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::DELETE});

    // semicolon a -> home
    mapper.addMapping(KeyCodes::SEMICOLON, KeyCodes::A, {KeyCodes::HOME});

    // a semicolon -> end
    mapper.addMapping(KeyCodes::A, KeyCodes::SEMICOLON, {KeyCodes::END});

    // f n -> down
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::N, {KeyCodes::DOWN_ARROW});

    // f u -> up
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::U, {KeyCodes::UP_ARROW});

    std::cout << "Loaded " << mapper.getMappingCount() << " TFF mappings\n\n";

    // Test j f -> backspace
    uint32_t timestamp = 0;
    auto result1 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    timestamp += 50; // Within 100ms threshold
    auto result2 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);

    assert(result2.size() == 1);
    assert(result2[0] == KeyCodes::BACKSPACE);
    std::cout << "✓ j f combination -> backspace\n";

    // Test f j -> delete
    timestamp = 0;
    auto result3 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 50; // Within 100ms threshold
    auto result4 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    assert(result4.size() == 1);
    assert(result4[0] == KeyCodes::DELETE);
    std::cout << "✓ f j combination -> delete\n";

    // Test semicolon a -> home
    timestamp = 0;
    auto result5 = app.processKeyEvent(KeyCodes::SEMICOLON, timestamp, true);
    timestamp += 50; // Within 100ms threshold
    auto result6 = app.processKeyEvent(KeyCodes::A, timestamp, true);

    assert(result6.size() == 1);
    assert(result6[0] == KeyCodes::HOME);
    std::cout << "✓ semicolon a combination -> home\n";

    // Test a semicolon -> end
    timestamp = 0;
    auto result7 = app.processKeyEvent(KeyCodes::A, timestamp, true);
    timestamp += 50; // Within 100ms threshold
    auto result8 = app.processKeyEvent(KeyCodes::SEMICOLON, timestamp, true);

    assert(result8.size() == 1);
    assert(result8[0] == KeyCodes::END);
    std::cout << "✓ a semicolon combination -> end\n";

    std::cout << "\nAll TFF mapping tests passed!\n";
    return 0;
}