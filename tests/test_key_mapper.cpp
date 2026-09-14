#include "../include/key_mapper.h"
#include "../include/key_events.h"
#include <cassert>
#include <iostream>

void test_basic_mapping() {
    std::cout << "Testing basic KeyMapper functionality...\n";

    KeyMapper mapper;

    // Add a simple mapping
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});

    // Test the mapping
    KeyCombination combo(KeyCodes::F_KEY, KeyCodes::J_KEY, 50);
    auto output = mapper.getMappedKeys(combo);

    assert(output.size() == 1);
    assert(output[0] == KeyCodes::ONE);

    std::cout << "Basic mapping test passed.\n\n";
}

void test_multiple_mappings() {
    std::cout << "Testing multiple key mappings...\n";

    KeyMapper mapper;

    // Add multiple mappings
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});
    mapper.addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE});

    // Test F+J -> 1
    KeyCombination combo1(KeyCodes::F_KEY, KeyCodes::J_KEY, 50);
    auto output1 = mapper.getMappedKeys(combo1);
    assert(output1.size() == 1);
    assert(output1[0] == KeyCodes::ONE);

    // Test J+F -> 2
    KeyCombination combo2(KeyCodes::J_KEY, KeyCodes::F_KEY, 30);
    auto output2 = mapper.getMappedKeys(combo2);
    assert(output2.size() == 1);
    assert(output2[0] == KeyCodes::TWO);

    // Test F+Space -> 3
    KeyCombination combo3(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, 75);
    auto output3 = mapper.getMappedKeys(combo3);
    assert(output3.size() == 1);
    assert(output3[0] == KeyCodes::THREE);

    std::cout << "Multiple mappings test passed.\n\n";
}

void test_no_mapping() {
    std::cout << "Testing unmapped combinations...\n";

    KeyMapper mapper;
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});

    // Test an unmapped combination
    KeyCombination combo(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, 50);
    auto output = mapper.getMappedKeys(combo);

    assert(output.empty());

    std::cout << "No mapping test passed.\n\n";
}

void test_clear_mappings() {
    std::cout << "Testing clear mappings functionality...\n";

    KeyMapper mapper;
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});

    assert(mapper.getMappingCount() == 1);

    mapper.clearMappings();

    assert(mapper.getMappingCount() == 0);

    // Test that mappings are really gone
    KeyCombination combo(KeyCodes::F_KEY, KeyCodes::J_KEY, 50);
    auto output = mapper.getMappedKeys(combo);
    assert(output.empty());

    std::cout << "Clear mappings test passed.\n\n";
}

int main() {
    std::cout << "Running KeyMapper unit tests...\n\n";

    test_basic_mapping();
    test_multiple_mappings();
    test_no_mapping();
    test_clear_mappings();

    std::cout << "All KeyMapper tests passed!\n";
    return 0;
}