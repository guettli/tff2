#include "rp2040_platform.h"
#include <iostream>
#include <cassert>
#include <algorithm>

static void test_initialization() {
    std::cout << "Test 1: RP2040Platform initialization with default config... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    const auto& config = platform.getConfig();
    assert(!config.combos.empty());
    assert(!config.tap_hold_keys.empty());

    // Verify default combos are present
    bool found_jf = false;
    for (const auto& c : config.combos) {
        if (c.keys.size() == 2 && c.keys[0] == tff::Keys::KEY_J && c.keys[1] == tff::Keys::KEY_F &&
            !c.out_keys.empty() && c.out_keys[0] == tff::Keys::KEY_BACKSPACE) {
            found_jf = true;
            break;
        }
    }
    assert(found_jf);

    // Verify tap-hold capslock is present
    bool found_caps = false;
    for (const auto& th : config.tap_hold_keys) {
        if (th.key == tff::Keys::KEY_CAPSLOCK && th.tap_key == tff::Keys::KEY_ESC &&
            th.hold_key == tff::Keys::KEY_LEFTMETA) {
            found_caps = true;
            break;
        }
    }
    assert(found_caps);

    std::cout << "PASSED (" << config.combos.size() << " combos, " << config.tap_hold_keys.size()
              << " tap-hold keys)\n";
}

static void test_keycode_conversion() {
    std::cout << "Test 2: USB HID <-> Linux KeyCode bidirectional conversion... ";

    // Letters
    assert(RP2040Platform::convertUsbToKeyCode(0x04) == tff::Keys::KEY_A);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_A) == 0x04);
    assert(RP2040Platform::convertUsbToKeyCode(0x09) == tff::Keys::KEY_F);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_F) == 0x09);
    assert(RP2040Platform::convertUsbToKeyCode(0x0D) == tff::Keys::KEY_J);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_J) == 0x0D);

    // Common keys
    assert(RP2040Platform::convertUsbToKeyCode(0x2C) == tff::Keys::KEY_SPACE);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_SPACE) == 0x2C);
    assert(RP2040Platform::convertUsbToKeyCode(0x29) == tff::Keys::KEY_ESC);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_ESC) == 0x29);
    assert(RP2040Platform::convertUsbToKeyCode(0x2A) == tff::Keys::KEY_BACKSPACE);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_BACKSPACE) == 0x2A);
    assert(RP2040Platform::convertUsbToKeyCode(0x4C) == tff::Keys::KEY_DELETE);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_DELETE) == 0x4C);
    assert(RP2040Platform::convertUsbToKeyCode(0x39) == tff::Keys::KEY_CAPSLOCK);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_CAPSLOCK) == 0x39);

    // Modifiers (all 8)
    assert(RP2040Platform::convertUsbToKeyCode(0xE0) == tff::Keys::KEY_LEFTCTRL);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_LEFTCTRL) == 0xE0);
    assert(RP2040Platform::convertUsbToKeyCode(0xE1) == tff::Keys::KEY_LEFTSHIFT);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_LEFTSHIFT) == 0xE1);
    assert(RP2040Platform::convertUsbToKeyCode(0xE2) == tff::Keys::KEY_LEFTALT);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_LEFTALT) == 0xE2);
    assert(RP2040Platform::convertUsbToKeyCode(0xE3) == tff::Keys::KEY_LEFTMETA);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_LEFTMETA) == 0xE3);
    assert(RP2040Platform::convertUsbToKeyCode(0xE4) == tff::Keys::KEY_RIGHTCTRL);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_RIGHTCTRL) == 0xE4);
    assert(RP2040Platform::convertUsbToKeyCode(0xE5) == tff::Keys::KEY_RIGHTSHIFT);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_RIGHTSHIFT) == 0xE5);
    assert(RP2040Platform::convertUsbToKeyCode(0xE6) == tff::Keys::KEY_RIGHTALT);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_RIGHTALT) == 0xE6);
    assert(RP2040Platform::convertUsbToKeyCode(0xE7) == tff::Keys::KEY_RIGHTMETA);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_RIGHTMETA) == 0xE7);

    // Function keys (F1 - F12)
    assert(RP2040Platform::convertUsbToKeyCode(0x3A) == tff::Keys::KEY_F1);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_F1) == 0x3A);
    assert(RP2040Platform::convertUsbToKeyCode(0x43) == tff::Keys::KEY_F10);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_F10) == 0x43);
    assert(RP2040Platform::convertUsbToKeyCode(0x44) == tff::Keys::KEY_F11);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_F11) == 0x44);
    assert(RP2040Platform::convertUsbToKeyCode(0x45) == tff::Keys::KEY_F12);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_F12) == 0x45);

    // Navigation and editing
    assert(RP2040Platform::convertUsbToKeyCode(0x49) == tff::Keys::KEY_INSERT);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_INSERT) == 0x49);
    assert(RP2040Platform::convertUsbToKeyCode(0x4A) == tff::Keys::KEY_HOME);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_HOME) == 0x4A);
    assert(RP2040Platform::convertUsbToKeyCode(0x4D) == tff::Keys::KEY_END);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_END) == 0x4D);

    // Keypad keys
    assert(RP2040Platform::convertUsbToKeyCode(0x53) == tff::Keys::KEY_NUMLOCK);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_NUMLOCK) == 0x53);
    assert(RP2040Platform::convertUsbToKeyCode(0x58) == tff::Keys::KEY_KPENTER);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_KPENTER) == 0x58);
    assert(RP2040Platform::convertUsbToKeyCode(0x59) == tff::Keys::KEY_KP1);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_KP1) == 0x59);
    assert(RP2040Platform::convertUsbToKeyCode(0x62) == tff::Keys::KEY_KP0);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_KP0) == 0x62);
    assert(RP2040Platform::convertUsbToKeyCode(0x63) == tff::Keys::KEY_KPDOT);
    assert(RP2040Platform::convertKeyCodeToUsb(tff::Keys::KEY_KPDOT) == 0x63);

    // Unmapped codes return 0
    assert(RP2040Platform::convertUsbToKeyCode(0x00) == 0);
    assert(RP2040Platform::convertUsbToKeyCode(0xA5) == 0);
    assert(RP2040Platform::convertKeyCodeToUsb(0) == 0);
    assert(RP2040Platform::convertKeyCodeToUsb(9999) == 0);

    // Backward-compatible static helper check
    assert(RP2040Platform::convertKeyCode(0x09) == tff::Keys::KEY_F);
    assert(RP2040Platform::convertToUsbKeyCode(tff::Keys::KEY_F) == 0x09);

    std::cout << "PASSED\n";
}

static void test_combos_processing() {
    std::cout << "Test 3: Home-row combos processing on RP2040... ";
    RP2040Platform platform;
    platform.initialize();

    // 1) Test j f -> backspace
    platform.clearEmittedKeys();
    platform.setTimestamp(100);
    platform.processHostKeyEvent(0x0D, true);  // USB J down
    platform.setTimestamp(120);
    platform.processHostKeyEvent(0x09, true);   // USB F down (chord!)
    platform.setTimestamp(180);                 // Overlap > 40ms
    platform.processHostKeyEvent(0x0D, false);  // USB J up
    platform.setTimestamp(200);
    platform.processHostKeyEvent(0x09, false);  // USB F up

    const auto& emitted1 = platform.getEmittedKeys();
    assert(!emitted1.empty());
    assert(emitted1.back() == tff::Keys::KEY_BACKSPACE);

    // 2) Test f j -> delete
    platform.clearEmittedKeys();
    platform.setTimestamp(500);
    platform.processHostKeyEvent(0x09, true);  // USB F down
    platform.setTimestamp(520);
    platform.processHostKeyEvent(0x0D, true);  // USB J down
    platform.setTimestamp(580);                // Overlap > 40ms
    platform.processHostKeyEvent(0x09, false);
    platform.setTimestamp(600);
    platform.processHostKeyEvent(0x0D, false);

    const auto& emitted2 = platform.getEmittedKeys();
    assert(!emitted2.empty());
    assert(emitted2.back() == tff::Keys::KEY_DELETE);

    std::cout << "PASSED\n";
}

static void test_triple_combos() {
    std::cout << "Test 4: Triple combos (d + f + j -> esc) on RP2040... ";
    RP2040Platform platform;
    platform.initialize();

    platform.clearEmittedKeys();
    platform.setTimestamp(1000);
    platform.processHostKeyEvent(0x07, true);  // USB D down
    platform.setTimestamp(1020);
    platform.processHostKeyEvent(0x09, true);  // USB F down
    platform.setTimestamp(1040);
    platform.processHostKeyEvent(0x0D, true);  // USB J down (triple chord!)
    platform.setTimestamp(1100);               // Overlap > 40ms
    platform.processHostKeyEvent(0x07, false);
    platform.setTimestamp(1120);
    platform.processHostKeyEvent(0x09, false);
    platform.setTimestamp(1140);
    platform.processHostKeyEvent(0x0D, false);

    const auto& emitted = platform.getEmittedKeys();
    assert(!emitted.empty());
    assert(emitted.back() == tff::Keys::KEY_ESC);

    std::cout << "PASSED\n";
}

static void test_tap_hold() {
    std::cout << "Test 5: Tap-vs-Hold CapsLock (Esc / Super) on RP2040... ";
    RP2040Platform platform;
    platform.initialize();

    // 1) Quick tap -> should emit Esc
    platform.clearEmittedKeys();
    platform.setTimestamp(2000);
    platform.processHostKeyEvent(0x39, true);   // USB CapsLock down
    platform.setTimestamp(2050);                // 50ms later (< 200ms)
    platform.processHostKeyEvent(0x39, false);  // USB CapsLock up -> TAP!

    const auto& emitted_tap = platform.getEmittedKeys();
    assert(!emitted_tap.empty());
    assert(emitted_tap.back() == tff::Keys::KEY_ESC);

    // 2) Fast chording: CapsLock held while another key is pressed -> promote to Super
    platform.clearEmittedKeys();
    platform.setTimestamp(3000);
    platform.processHostKeyEvent(0x39, true);  // USB CapsLock down
    platform.setTimestamp(3050);
    platform.processHostKeyEvent(0x2C, true);  // USB Space down -> triggers hold promotion!

    const auto& emitted_chord = platform.getEmittedKeys();
    assert(!emitted_chord.empty());
    assert(emitted_chord[0] == tff::Keys::KEY_LEFTMETA);  // Super down

    platform.setTimestamp(3080);
    platform.processHostKeyEvent(0x2C, false);
    platform.processHostKeyEvent(0x39, false);

    // 3) Hold timeout: CapsLock held past 200ms -> promote to Super
    platform.clearEmittedKeys();
    platform.setTimestamp(4000);
    platform.processHostKeyEvent(0x39, true);  // USB CapsLock down
    platform.setTimestamp(4250);               // 250ms later (> 200ms)
    platform.checkTimers();

    const auto& emitted_hold = platform.getEmittedKeys();
    assert(!emitted_hold.empty());
    assert(emitted_hold[0] == tff::Keys::KEY_LEFTMETA);  // Super down

    platform.setTimestamp(4300);
    platform.processHostKeyEvent(0x39, false);

    std::cout << "PASSED\n";
}

static void test_modal_layers() {
    std::cout << "Test 6: Modal layers on RP2040... ";
    RP2040Platform platform;
    platform.initialize();

    // Load custom configuration with a navigation layer
    std::string custom_yaml = R"(
tap_hold:
  space:
    tap: space
    layer: nav
    timeout_ms: 200

layers:
  nav:
    h: left
    j: down
    k: up
    l: right
)";
    bool ok = platform.loadConfiguration(custom_yaml);
    assert(ok);

    // Hold Space (0x2C) and press J (0x0D) -> should emit Down arrow
    platform.clearEmittedKeys();
    platform.setTimestamp(5000);
    platform.processHostKeyEvent(0x2C, true);  // USB Space down
    platform.setTimestamp(5040);
    platform.processHostKeyEvent(0x0D, true);  // USB J down

    const auto& emitted = platform.getEmittedKeys();
    assert(!emitted.empty());
    assert(emitted.back() == tff::Keys::KEY_DOWN);

    platform.setTimestamp(5080);
    platform.processHostKeyEvent(0x0D, false);
    platform.processHostKeyEvent(0x2C, false);

    std::cout << "PASSED\n";
}

static void test_text_snippets() {
    std::cout << "Test 7: Text snippets on RP2040... ";
    RP2040Platform platform;
    platform.initialize();

    std::string snippet_yaml = R"(
combos:
  j + k: "hi"
)";
    bool ok = platform.loadConfiguration(snippet_yaml);
    assert(ok);

    platform.clearEmittedKeys();
    platform.setTimestamp(6000);
    platform.processHostKeyEvent(0x0D, true);  // USB J down
    platform.setTimestamp(6020);
    platform.processHostKeyEvent(0x0E, true);  // USB K down
    platform.setTimestamp(6080);               // Overlap > 40ms
    platform.processHostKeyEvent(0x0D, false);
    platform.setTimestamp(6100);
    platform.processHostKeyEvent(0x0E, false);

    const auto& emitted = platform.getEmittedKeys();
    // Emitted keys should contain 'h' and 'i'
    assert(emitted.size() >= 2);
    assert(std::find(emitted.begin(), emitted.end(), tff::Keys::KEY_H) != emitted.end());
    assert(std::find(emitted.begin(), emitted.end(), tff::Keys::KEY_I) != emitted.end());

    std::cout << "PASSED\n";
}

static void test_device_keys_and_cleanup() {
    std::cout << "Test 8: sendDeviceKeys and cleanup... ";
    RP2040Platform platform;
    platform.initialize();

    platform.clearEmittedKeys();
    std::vector<uint32_t> keys = {tff::Keys::KEY_A, tff::Keys::KEY_B};
    bool ok = platform.sendDeviceKeys(keys);
    assert(ok);

    const auto& emitted = platform.getEmittedKeys();
    assert(emitted.size() == 2);
    assert(emitted[0] == tff::Keys::KEY_A);
    assert(emitted[1] == tff::Keys::KEY_B);

    platform.cleanup();
    std::cout << "PASSED\n";
}

static void test_unmapped_keys_and_auto_init() {
    std::cout << "Test 9: getEngine auto-initialization & unmapped key drop... ";
    RP2040Platform platform;

    // platform not initialized yet; calling getEngine() should auto-initialize
    const tff::TFFEngine& engine = platform.getEngine();
    assert(platform.getConfig().combos.size() > 0);
    (void)engine;

    // Feeding an unmapped USB scancode (0xA5) or >0xFF should drop without crashing
    platform.clearEmittedKeys();
    platform.setTimestamp(7000);
    platform.processHostKeyEvent(0xA5, true);
    platform.processHostKeyEvent(0xA5, false);
    platform.processHostKeyEvent(0x100, true);
    assert(platform.getEmittedKeys().empty());

    // Feeding F1 (0x3A) should NOT be confused with CapsLock
    platform.clearEmittedKeys();
    platform.setTimestamp(7100);
    platform.processHostKeyEvent(0x3A, true);  // F1 down
    platform.setTimestamp(7150);
    platform.processHostKeyEvent(0x3A, false);  // F1 up
    const auto& emitted = platform.getEmittedKeys();
    assert(emitted.size() == 1);
    assert(emitted[0] == tff::Keys::KEY_F1);
    assert(emitted[0] != tff::Keys::KEY_CAPSLOCK);

    std::cout << "PASSED\n";
}

static void test_one_shot_keys() {
    std::cout << "Test 10: One-shot modifiers and layers on RP2040... ";
    RP2040Platform platform;
    std::string config_yaml = R"(
one_shot:
  leftshift: 1500
  space: [nav, 1500]
layers:
  nav:
    k: up
)";
    bool ok = platform.loadConfiguration(config_yaml);
    assert(ok);
    assert(platform.getConfig().one_shot_keys.size() == 2);

    // Tap LeftShift (USB 0xE1)
    platform.clearEmittedKeys();
    platform.setTimestamp(8000);
    platform.processHostKeyEvent(0xE1, true);
    platform.setTimestamp(8040);
    platform.processHostKeyEvent(0xE1, false);
    // Shift is not emitted yet (it is armed as one-shot modifier)
    assert(platform.getEmittedKeys().empty());

    // Press Key A (USB 0x04)
    platform.setTimestamp(8100);
    platform.processHostKeyEvent(0x04, true);
    // When Key A is pressed, LeftShift and Key A are emitted!
    const auto& emitted1 = platform.getEmittedKeys();
    assert(emitted1.size() == 2);
    assert(emitted1[0] == tff::Keys::KEY_LEFTSHIFT);
    assert(emitted1[1] == tff::Keys::KEY_A);

    // Release Key A
    platform.setTimestamp(8140);
    platform.processHostKeyEvent(0x04, false);

    // Tap Space (USB 0x2C) -> arms OSL "nav"
    platform.clearEmittedKeys();
    platform.setTimestamp(8200);
    platform.processHostKeyEvent(0x2C, true);
    platform.setTimestamp(8240);
    platform.processHostKeyEvent(0x2C, false);
    assert(platform.getEmittedKeys().empty());

    // Press Key K (USB 0x0E) -> maps to KEY_UP!
    platform.setTimestamp(8300);
    platform.processHostKeyEvent(0x0E, true);
    const auto& emitted2 = platform.getEmittedKeys();
    assert(emitted2.size() == 1);
    assert(emitted2[0] == tff::Keys::KEY_UP);

    platform.setTimestamp(8340);
    platform.processHostKeyEvent(0x0E, false);

    // Next Key K is regular K
    platform.clearEmittedKeys();
    platform.setTimestamp(8400);
    platform.processHostKeyEvent(0x0E, true);
    const auto& emitted3 = platform.getEmittedKeys();
    assert(emitted3.size() == 1);
    assert(emitted3[0] == tff::Keys::KEY_K);

    platform.setTimestamp(8440);
    platform.processHostKeyEvent(0x0E, false);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "================================================\n";
    std::cout << "Testing RP2040 Platform with Unified TFFEngine\n";
    std::cout << "================================================\n";

    test_initialization();
    test_keycode_conversion();
    test_combos_processing();
    test_triple_combos();
    test_tap_hold();
    test_modal_layers();
    test_text_snippets();
    test_device_keys_and_cleanup();
    test_unmapped_keys_and_auto_init();
    test_one_shot_keys();

    std::cout << "\nAll RP2040 platform unified engine tests passed!\n";
    return 0;
}