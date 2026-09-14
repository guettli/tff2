#include "../include/tff_app.h"
#include "../include/key_events.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_app_integration() {
    std::cout << "Testing TFFApp integration...\n";

    TFFApp app(100); // 100ms threshold

    // Add mappings manually for testing
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});
    app.getKeyMapper().addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});

    // Test F+J combination
    uint32_t timestamp = 0;

    // Press F
    auto output1 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    assert(output1.empty()); // No output yet

    timestamp += 50;

    // Press J (should trigger combination)
    auto output2 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    assert(output2.size() == 1);
    assert(output2[0] == KeyCodes::ONE);

    std::cout << "App integration test passed.\n\n";
}

void test_app_reset() {
    std::cout << "Testing TFFApp reset functionality...\n";

    TFFApp app(100);
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});

    // Verify mapping exists
    assert(app.getKeyMapper().getMappingCount() > 0);

    // Reset the app
    app.reset();

    // Verify mappings are cleared
    assert(app.getKeyMapper().getMappingCount() == 0);

    std::cout << "App reset test passed.\n\n";
}

void test_sequential_vs_overlapping() {
    std::cout << "Testing sequential vs overlapping key processing...\n";

    TFFApp app(100);
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});

    uint32_t timestamp = 0;

    // Test sequential pressing (F, then J after threshold)
    // Press and release F
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 150; // Way past threshold
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, false);
    timestamp += 50;

    // Press J (should NOT trigger combination)
    auto output1 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    assert(output1.empty());

    // Reset for next test
    app.getKeyDetector().reset();

    // Test overlapping pressing (F, then J within threshold)
    timestamp = 0;
    app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 50; // Within threshold
    auto output2 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    // Should trigger combination
    assert(!output2.empty());
    assert(output2[0] == KeyCodes::ONE);

    std::cout << "Sequential vs overlapping test passed.\n\n";
}

int main() {
    std::cout << "Running TFFApp unit tests...\n\n";

    test_app_integration();
    test_app_reset();
    test_sequential_vs_overlapping();

    std::cout << "All TFFApp tests passed!\n";
    return 0;
}