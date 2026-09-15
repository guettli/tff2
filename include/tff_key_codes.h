#ifndef TFF_KEY_CODES_H
#define TFF_KEY_CODES_H

#include <string>
#include <cstdint>
#include "tff_types.h"

namespace tff {

// Standard Linux Key Codes commonly used in TFF
namespace Keys {
    constexpr KeyCode KEY_RESERVED   = 0;
    constexpr KeyCode KEY_ESC        = 1;
    constexpr KeyCode KEY_1          = 2;
    constexpr KeyCode KEY_2          = 3;
    constexpr KeyCode KEY_3          = 4;
    constexpr KeyCode KEY_4          = 5;
    constexpr KeyCode KEY_5          = 6;
    constexpr KeyCode KEY_6          = 7;
    constexpr KeyCode KEY_7          = 8;
    constexpr KeyCode KEY_8          = 9;
    constexpr KeyCode KEY_9          = 10;
    constexpr KeyCode KEY_0          = 11;
    constexpr KeyCode KEY_MINUS      = 12;
    constexpr KeyCode KEY_EQUAL      = 13;
    constexpr KeyCode KEY_BACKSPACE  = 14;
    constexpr KeyCode KEY_TAB        = 15;
    constexpr KeyCode KEY_Q          = 16;
    constexpr KeyCode KEY_W          = 17;
    constexpr KeyCode KEY_E          = 18;
    constexpr KeyCode KEY_R          = 19;
    constexpr KeyCode KEY_T          = 20;
    constexpr KeyCode KEY_Y          = 21;
    constexpr KeyCode KEY_U          = 22;
    constexpr KeyCode KEY_I          = 23;
    constexpr KeyCode KEY_O          = 24;
    constexpr KeyCode KEY_P          = 25;
    constexpr KeyCode KEY_LEFTBRACE  = 26;
    constexpr KeyCode KEY_RIGHTBRACE = 27;
    constexpr KeyCode KEY_ENTER      = 28;
    constexpr KeyCode KEY_LEFTCTRL   = 29;
    constexpr KeyCode KEY_A          = 30;
    constexpr KeyCode KEY_S          = 31;
    constexpr KeyCode KEY_D          = 32;
    constexpr KeyCode KEY_F          = 33;
    constexpr KeyCode KEY_G          = 34;
    constexpr KeyCode KEY_H          = 35;
    constexpr KeyCode KEY_J          = 36;
    constexpr KeyCode KEY_K          = 37;
    constexpr KeyCode KEY_L          = 38;
    constexpr KeyCode KEY_SEMICOLON  = 39;
    constexpr KeyCode KEY_APOSTROPHE = 40;
    constexpr KeyCode KEY_GRAVE      = 41;
    constexpr KeyCode KEY_LEFTSHIFT  = 42;
    constexpr KeyCode KEY_BACKSLASH  = 43;
    constexpr KeyCode KEY_Z          = 44;
    constexpr KeyCode KEY_X          = 45;
    constexpr KeyCode KEY_C          = 46;
    constexpr KeyCode KEY_V          = 47;
    constexpr KeyCode KEY_B          = 48;
    constexpr KeyCode KEY_N          = 49;
    constexpr KeyCode KEY_M          = 50;
    constexpr KeyCode KEY_COMMA      = 51;
    constexpr KeyCode KEY_DOT        = 52;
    constexpr KeyCode KEY_SLASH      = 53;
    constexpr KeyCode KEY_RIGHTSHIFT = 54;
    constexpr KeyCode KEY_LEFTALT    = 56;
    constexpr KeyCode KEY_SPACE      = 57;
    constexpr KeyCode KEY_CAPSLOCK   = 58;
    constexpr KeyCode KEY_HOME       = 102;
    constexpr KeyCode KEY_UP         = 103;
    constexpr KeyCode KEY_PAGEUP     = 104;
    constexpr KeyCode KEY_LEFT       = 105;
    constexpr KeyCode KEY_RIGHT      = 106;
    constexpr KeyCode KEY_END        = 107;
    constexpr KeyCode KEY_DOWN       = 108;
    constexpr KeyCode KEY_PAGEDOWN   = 109;
    constexpr KeyCode KEY_DELETE     = 111;
    constexpr KeyCode KEY_LEFTMETA   = 125;
    constexpr KeyCode KEY_RFKILL     = 247;
}

/**
 * @brief Converts a word (lowercase, e.g. "f", "capslock", "backspace") to KeyCode.
 * Follows the Go tff rules:
 * - Only lowercase allowed; uppercase gives "only lower case characters are allowed".
 * - Unknown keys give "failed to get key \"...\": unknown key".
 */
bool wordToKeyCode(const std::string& word, KeyCode& out_code, std::string& err_msg);

/**
 * @brief Converts a KeyCode to lowercase word string ("f", "capslock", "backspace").
 */
std::string keyCodeToWord(KeyCode code);

/**
 * @brief Converts a KeyCode to uppercase short name ("F", "CAPSLOCK", "BACKSPACE", "1").
 */
std::string keyCodeToShortName(KeyCode code);

/**
 * @brief Converts (type, code) to evdev code name ("KEY_F", "SYN_REPORT", "MSC_SCAN").
 */
std::string codeName(uint16_t type, uint16_t code);

/**
 * @brief Converts type to evdev type name ("EV_KEY", "EV_SYN", "EV_MSC").
 */
std::string typeName(uint16_t type);

/**
 * @brief Parses type name string ("EV_KEY", "EV_SYN", "EV_MSC") to EvType integer.
 */
bool parseTypeName(const std::string& name, uint16_t& out_type);

/**
 * @brief Parses code name string ("KEY_F", "SYN_REPORT", "MSC_SCAN") to EvCode integer.
 */
bool parseCodeName(uint16_t type, const std::string& name, uint16_t& out_code);

/**
 * @brief Translates a printable ASCII character (or \t, \n) into an evdev KeyCode and Shift flag.
 * @param c The input ASCII character.
 * @param code Output evdev keycode.
 * @param shift Output boolean set to true if LeftShift modifier is required.
 * @return True if character was successfully mapped, false if unsupported.
 */
bool asciiToKeyStroke(char c, KeyCode& code, bool& shift);

} // namespace tff

#endif // TFF_KEY_CODES_H
