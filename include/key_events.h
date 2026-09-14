#ifndef KEY_EVENTS_H
#define KEY_EVENTS_H

#include <cstdint>

/**
 * @brief Represents a detected key combination
 */
struct KeyCombination {
    uint32_t first_key;     // First key pressed
    uint32_t second_key;    // Second key pressed
    uint32_t time_diff;     // Time difference between presses

    KeyCombination() : first_key(0), second_key(0), time_diff(0) {}
    KeyCombination(uint32_t first, uint32_t second, uint32_t diff)
        : first_key(first), second_key(second), time_diff(diff) {}

    bool isEmpty() const { return first_key == 0 && second_key == 0; }
    bool isValid() const { return first_key != 0 && second_key != 0; }
};

/**
 * @brief Represents a key event
 */
struct KeyEvent {
    uint32_t key_code;
    uint32_t timestamp_ms;
    bool is_pressed;

    KeyEvent() : key_code(0), timestamp_ms(0), is_pressed(false) {}
    KeyEvent(uint32_t code, uint32_t time, bool pressed)
        : key_code(code), timestamp_ms(time), is_pressed(pressed) {}
};

// Common key codes (platform-independent representations)
namespace KeyCodes {
    constexpr uint32_t F_KEY = 0x09;
    constexpr uint32_t J_KEY = 0x0A;
    constexpr uint32_t SPACE_KEY = 0x2C;
    constexpr uint32_t ONE = 0x1E;
    constexpr uint32_t TWO = 0x1F;
    constexpr uint32_t THREE = 0x20;
    constexpr uint32_t FOUR = 0x21;
    constexpr uint32_t LEFT_ARROW = 0x50;
    constexpr uint32_t RIGHT_ARROW = 0x4F;
    constexpr uint32_t UP_ARROW = 0x52;
    constexpr uint32_t DOWN_ARROW = 0x51;
    constexpr uint32_t ENTER = 0x28;
    constexpr uint32_t BACKSPACE = 0x2A;
    constexpr uint32_t DELETE = 0x4C;
    constexpr uint32_t ESCAPE = 0x29;
    constexpr uint32_t CONTROL = 0xE0;
    constexpr uint32_t SHIFT = 0xE1;
    constexpr uint32_t ALT = 0xE2;
    constexpr uint32_t HOME = 0x4A;
    constexpr uint32_t END = 0x4D;
    constexpr uint32_t PAGE_UP = 0x4B;
    constexpr uint32_t PAGE_DOWN = 0x4E;
    constexpr uint32_t SEMICOLON = 0x33;
    constexpr uint32_t A = 0x04;
    constexpr uint32_t N = 0x11;
    constexpr uint32_t U = 0x18;
    constexpr uint32_t M = 0x10;
    constexpr uint32_t K = 0x0E;
    constexpr uint32_t L = 0x0F;
    constexpr uint32_t I = 0x0C;
    constexpr uint32_t COMMA = 0x36;
    constexpr uint32_t G = 0x1A;  // Changed from 0x0A to avoid conflict with J_KEY
    constexpr uint32_t H = 0x0B;
    constexpr uint32_t D = 0x07;
}

#endif // KEY_EVENTS_H