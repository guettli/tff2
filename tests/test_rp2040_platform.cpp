#include "rp2040_platform.h"
#include "tff_app.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Testing RP2040 Platform Implementation\n";
    std::cout << "========================================\n";

    // Test RP2040Platform initialization
    RP2040Platform platform;

    // Test initialization
    bool init_result = platform.initialize();
    assert(init_result);
    (void)init_result;
    std::cout << "✓ RP2040Platform initialization successful\n";

    // Test with TFFApp directly
    TFFApp app(100); // 100ms threshold

    // Add test mappings
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});      // F+J -> 1
    app.getKeyMapper().addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});      // J+F -> 2
    app.getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE}); // F+Space -> 3

    // Test F+J combination (F pressed first)
    uint32_t timestamp = 0;
    auto result1 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 50; // Within 100ms threshold
    auto result2 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    assert(result2.size() == 1);
    assert(result2[0] == KeyCodes::ONE);
    std::cout << "✓ F+J combination detection successful\n";

    // Test J+F combination (J pressed first)
    timestamp = 0;
    auto result3 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);
    timestamp += 50; // Within 100ms threshold
    auto result4 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);

    assert(result4.size() == 1);
    assert(result4[0] == KeyCodes::TWO);
    std::cout << "✓ J+F combination detection successful\n";

    // Test sequential keys (outside threshold)
    timestamp = 0;
    auto result5 = app.processKeyEvent(KeyCodes::F_KEY, timestamp, true);
    timestamp += 150; // Outside 100ms threshold
    auto result6 = app.processKeyEvent(KeyCodes::J_KEY, timestamp, true);

    assert(result6.empty());
    std::cout << "✓ Sequential keys (outside threshold) handled correctly\n";

    std::cout << "\nAll RP2040 platform tests passed!\n";
    return 0;
}