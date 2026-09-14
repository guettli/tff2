#include "../include/key_detector.h"
#include <cassert>
#include <iostream>

void test_basic_functionality() {
    std::cout << "Testing basic KeyDetector functionality...\n";

    KeyDetector detector(100); // 100ms threshold

    // Test initial state
    assert(detector.getLastCombination().isEmpty());

    // Test single key press
    detector.processKeyEvent(1, 0, true); // Press key 1 at time 0
    assert(detector.getLastCombination().isEmpty()); // No combination yet

    // Test single key release
    detector.processKeyEvent(1, 50, false); // Release key 1 at time 50
    assert(detector.getLastCombination().isEmpty());

    std::cout << "Basic functionality test passed.\n\n";
}

void test_overlapping_detection() {
    std::cout << "Testing overlapping key detection...\n";

    KeyDetector detector(100); // 100ms threshold

    // Press first key
    detector.processKeyEvent(KeyCodes::F_KEY, 0, true);
    assert(detector.getLastCombination().isEmpty());

    // Press second key within threshold
    detector.processKeyEvent(KeyCodes::J_KEY, 50, true);

    // Should detect the combination
    KeyCombination combo = detector.getLastCombination();
    assert(combo.isValid());
    assert(combo.first_key == KeyCodes::F_KEY);
    assert(combo.second_key == KeyCodes::J_KEY);
    assert(combo.time_diff == 50);

    // Clear the combination
    detector.clearLastCombination();
    assert(detector.getLastCombination().isEmpty());

    std::cout << "Overlapping detection test passed.\n\n";
}

void test_non_overlapping() {
    std::cout << "Testing non-overlapping key detection...\n";

    KeyDetector detector(100); // 100ms threshold

    // Press first key
    detector.processKeyEvent(KeyCodes::F_KEY, 0, true);
    detector.processKeyEvent(KeyCodes::F_KEY, 50, false); // Release first key

    // Press second key after threshold
    detector.processKeyEvent(KeyCodes::J_KEY, 150, true); // Way past threshold

    // Should NOT detect combination
    KeyCombination combo = detector.getLastCombination();
    assert(combo.isEmpty());

    std::cout << "Non-overlapping detection test passed.\n\n";
}

void test_reset_functionality() {
    std::cout << "Testing reset functionality...\n";

    KeyDetector detector(100);

    // Set up a combination
    detector.processKeyEvent(KeyCodes::F_KEY, 0, true);
    detector.processKeyEvent(KeyCodes::J_KEY, 50, true);

    assert(detector.getLastCombination().isValid());

    // Reset
    detector.reset();

    assert(detector.getLastCombination().isEmpty());

    std::cout << "Reset functionality test passed.\n\n";
}

int main() {
    std::cout << "Running KeyDetector unit tests...\n\n";

    test_basic_functionality();
    test_overlapping_detection();
    test_non_overlapping();
    test_reset_functionality();

    std::cout << "All KeyDetector tests passed!\n";
    return 0;
}