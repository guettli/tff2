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

static void test_unmapped_keys() {
    std::cout << "Test 9: Initialized engine & unmapped key drop... ";
    RP2040Platform platform;
    bool init_ok = platform.initialize();
    assert(init_ok);

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
    bool init_ok = platform.initialize();
    assert(init_ok);
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

static void test_raw_keyboard_report() {
    std::cout << "Test 11: sendRawKeyboardReport API verification... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    uint8_t keys[6] = {0x04, 0x05, 0, 0, 0, 0};              // A, B
    bool sent = platform.sendRawKeyboardReport(0x02, keys);  // Shift + A + B
    assert(sent);

    std::cout << "PASSED\n";
}

static void test_host_keyboard_report_processing() {
    std::cout << "Test 12: processHostKeyboardReport diffing & combos... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    // 1. Single non-combo key press & release
    platform.clearEmittedKeys();
    platform.setTimestamp(100);
    const uint8_t report_c[6] = {0x06, 0, 0, 0, 0, 0};  // Key C
    platform.processHostKeyboardReport(0, report_c, 6);
    const auto& emitted1 = platform.getEmittedKeys();
    assert(emitted1.size() == 1);
    assert(emitted1[0] == tff::Keys::KEY_C);

    platform.setTimestamp(150);
    const uint8_t report_empty[6] = {0, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0, report_empty, 6);

    // 2. Modifiers and key rollover
    platform.clearEmittedKeys();
    platform.setTimestamp(200);
    // Press Left Shift (0x02) + Key B (0x05)
    const uint8_t report_shift_b[6] = {0x05, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0x02, report_shift_b, 6);
    const auto& emitted2 = platform.getEmittedKeys();
    assert(emitted2.size() == 2);
    assert(emitted2[0] == tff::Keys::KEY_LEFTSHIFT);
    assert(emitted2[1] == tff::Keys::KEY_B);

    platform.setTimestamp(250);
    platform.processHostKeyboardReport(0, report_empty, 6);

    // 3. J + F combo via raw reports -> Backspace
    platform.clearEmittedKeys();
    platform.setTimestamp(500);
    const uint8_t report_j[6] = {0x0D, 0, 0, 0, 0, 0};  // J
    platform.processHostKeyboardReport(0, report_j, 6);

    platform.setTimestamp(520);
    const uint8_t report_jf[6] = {0x0D, 0x09, 0, 0, 0, 0};  // J + F (chord)
    platform.processHostKeyboardReport(0, report_jf, 6);

    platform.setTimestamp(580);  // Overlap > 40ms
    platform.processHostKeyboardReport(0, report_empty, 6);

    const auto& emitted3 = platform.getEmittedKeys();
    assert(!emitted3.empty());
    assert(emitted3.back() == tff::Keys::KEY_BACKSPACE);

    // 4. Rollover error (0x01) safely ignored
    platform.clearEmittedKeys();
    platform.setTimestamp(600);
    const uint8_t report_rollover[6] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
    platform.processHostKeyboardReport(0, report_rollover, 6);
    assert(platform.getEmittedKeys().empty());

    std::cout << "PASSED\n";
}

static void test_host_report_queue() {
    std::cout << "Test 13: Lock-free host report queue (Core 1 -> Core 0)... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    // Test enqueue and dequeue
    const uint8_t keys1[6] = {0x04, 0, 0, 0, 0, 0};
    bool enq1 = platform.enqueueHostReport(0x01, keys1, 6);
    assert(enq1);

    const uint8_t keys2[6] = {0x05, 0x06, 0, 0, 0, 0};
    bool enq2 = platform.enqueueHostReport(0x02, keys2, 6);
    assert(enq2);

    RP2040Platform::HostKeyboardReport rep1;
    bool deq1 = platform.dequeueHostReport(rep1);
    assert(deq1);
    assert(rep1.modifiers == 0x01);
    assert(rep1.keys[0] == 0x04);

    RP2040Platform::HostKeyboardReport rep2;
    bool deq2 = platform.dequeueHostReport(rep2);
    assert(deq2);
    assert(rep2.modifiers == 0x02);
    assert(rep2.keys[0] == 0x05);
    assert(rep2.keys[1] == 0x06);

    // Dequeue on empty queue returns false
    RP2040Platform::HostKeyboardReport empty_rep;
    bool deq_empty = platform.dequeueHostReport(empty_rep);
    assert(!deq_empty);

    // Test processUsbHostEvents draining queue
    platform.clearEmittedKeys();
    platform.setTimestamp(700);
    const uint8_t report_c[6] = {0x06, 0, 0, 0, 0, 0};
    bool enq3 = platform.enqueueHostReport(0, report_c, 6);
    assert(enq3);

    platform.processUsbHostEvents();
    const auto& emitted = platform.getEmittedKeys();
    assert(emitted.size() == 1);
    assert(emitted[0] == tff::Keys::KEY_C);

    std::cout << "PASSED\n";
}

static void test_debug_buffer() {
    std::cout << "Test 14: DebugBuffer recording, rollover, formatting, pause... ";
    tff::DebugBuffer buf;
    assert(buf.empty());
    assert(buf.size() == 0);

    // 1. Record each event type
    uint8_t in_keys[6] = {0x04, 0x05, 0, 0, 0, 0};
    buf.recordInRawReport(100, 0x02, in_keys, 2);
    buf.recordInKeyEvent(110, tff::Keys::KEY_A, true);
    buf.recordTimerExpired(150);
    buf.recordOutKeyEvent(160, tff::Keys::KEY_B, true);
    uint8_t out_keys[6] = {0x05, 0, 0, 0, 0, 0};
    buf.recordOutRawReport(170, 0x00, out_keys);

    assert(!buf.empty());
    assert(buf.size() == 5);
    auto entries = buf.getEntries();
    assert(entries.size() == 5);
    assert(entries[0].type == tff::DebugEventType::IN_RAW_REPORT);
    assert(entries[0].modifiers == 0x02);
    assert(entries[0].raw_keys[0] == 0x04);
    assert(entries[1].type == tff::DebugEventType::IN_KEY_EVENT);
    assert(entries[1].keycode == static_cast<uint16_t>(tff::Keys::KEY_A));
    assert(entries[1].val == 1);
    assert(entries[2].type == tff::DebugEventType::TIMER_EXPIRED);
    assert(entries[3].type == tff::DebugEventType::OUT_KEY_EVENT);
    assert(entries[3].keycode == static_cast<uint16_t>(tff::Keys::KEY_B));
    assert(entries[4].type == tff::DebugEventType::OUT_RAW_REPORT);

    // 2. Formatting verification
    std::string dump = buf.formatDump(1000);
    assert(dump.find("=== TFF RP2040 DEBUG DUMP ===") != std::string::npos);
    assert(dump.find("Uptime: 1.000s | Events: 5") != std::string::npos);
    assert(dump.find("IN_RAW : mod=02") != std::string::npos);
    assert(dump.find("IN_EV  :") != std::string::npos);
    assert(dump.find("TIMER  : expired") != std::string::npos);
    assert(dump.find("OUT_EV :") != std::string::npos);
    assert(dump.find("OUT_RAW: mod=00") != std::string::npos);
    assert(dump.find("=== END DUMP ===") != std::string::npos);

    // 3. Circular rollover past capacity (64)
    buf.clear();
    assert(buf.empty());
    assert(buf.size() == 0);

    for (uint32_t i = 0; i < 70; ++i) {
        buf.recordInKeyEvent(1000 + i * 10, static_cast<tff::KeyCode>(i), true);
    }
    assert(buf.size() == tff::DebugBuffer::CAPACITY);
    auto rolled = buf.getEntries();
    assert(rolled.size() == tff::DebugBuffer::CAPACITY);
    // Oldest 6 events (i=0..5) were overwritten; oldest surviving is i=6
    assert(rolled[0].timestamp_ms == 1000 + 6 * 10);
    assert(rolled[0].keycode == 6);
    // Newest is i=69
    assert(rolled.back().timestamp_ms == 1000 + 69 * 10);
    assert(rolled.back().keycode == 69);

    // 4. Pause state
    buf.setPaused(true);
    assert(buf.isPaused());
    buf.recordInKeyEvent(9999, tff::Keys::KEY_Z, true);
    assert(buf.size() == tff::DebugBuffer::CAPACITY);
    assert(buf.getEntries().back().keycode == 69);  // Not overwritten while paused

    buf.setPaused(false);
    assert(!buf.isPaused());
    buf.recordInKeyEvent(9999, tff::Keys::KEY_Z, true);
    assert(buf.getEntries().back().keycode == static_cast<uint16_t>(tff::Keys::KEY_Z));

    std::cout << "PASSED\n";
}

static void test_rp2040_debug_dump_chord() {
    std::cout << "Test 15: RP2040 debug dump chord (d + f + j + k)... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    // 1. Normal keystroke before chord: Key A press and release
    platform.clearEmittedKeys();
    platform.setTimestamp(100);
    const uint8_t report_a[6] = {0x04, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0, report_a, 6);
    platform.setTimestamp(150);
    const uint8_t report_empty[6] = {0, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0, report_empty, 6);

    const auto& emitted_before = platform.getEmittedKeys();
    assert(emitted_before.size() == 1);
    assert(emitted_before[0] == tff::Keys::KEY_A);
    assert(platform.getLastDebugDump().empty());

    // 2. Press d + f + j + k simultaneously
    platform.clearEmittedKeys();
    platform.setTimestamp(300);
    const uint8_t report_chord[6] = {0x07, 0x09, 0x0D, 0x0E, 0, 0};
    platform.processHostKeyboardReport(0, report_chord, 6);

    // Verify dump was triggered
    const std::string& dump = platform.getLastDebugDump();
    assert(!dump.empty());
    assert(dump.find("=== TFF RP2040 DEBUG DUMP ===") != std::string::npos);
    assert(dump.find("=== END DUMP ===") != std::string::npos);
    // Verify dump captured the earlier 'A' event
    assert(dump.find("KEY_A") != std::string::npos);

    // 3. Release the chord
    platform.setTimestamp(400);
    platform.processHostKeyboardReport(0, report_empty, 6);

    // 4. Normal keystroke after chord: Key B press and release
    platform.clearEmittedKeys();
    platform.setTimestamp(500);
    const uint8_t report_b[6] = {0x05, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0, report_b, 6);
    platform.setTimestamp(550);
    platform.processHostKeyboardReport(0, report_empty, 6);

    const auto& emitted_after = platform.getEmittedKeys();
    assert(!emitted_after.empty());
    assert(emitted_after[0] == tff::Keys::KEY_B);

    std::cout << "PASSED\n";
}

static void test_rp2040_debug_dump_shift_d() {
    std::cout << "Test 16: RP2040 debug dump chord (LeftShift + RightShift + D)... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    // 1. Press both Shifts (0x02 | 0x20 = 0x22)
    platform.setTimestamp(100);
    const uint8_t report_empty[6] = {0, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0x22, report_empty, 6);

    // 2. Press 'D' (0x07) while holding both Shifts
    platform.clearEmittedKeys();
    platform.setTimestamp(150);
    const uint8_t report_d[6] = {0x07, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0x22, report_d, 6);

    // Verify dump was triggered
    const std::string& dump = platform.getLastDebugDump();
    assert(!dump.empty());
    assert(dump.find("=== TFF RP2040 DEBUG DUMP ===") != std::string::npos);

    // 3. Release all keys
    platform.setTimestamp(200);
    platform.processHostKeyboardReport(0x00, report_empty, 6);

    // 4. Subsequent key C should work cleanly without stuck modifiers
    platform.clearEmittedKeys();
    platform.setTimestamp(300);
    const uint8_t report_c[6] = {0x06, 0, 0, 0, 0, 0};
    platform.processHostKeyboardReport(0x00, report_c, 6);
    platform.setTimestamp(350);
    platform.processHostKeyboardReport(0x00, report_empty, 6);

    const auto& emitted = platform.getEmittedKeys();
    assert(!emitted.empty());
    assert(emitted[0] == tff::Keys::KEY_C);

    std::cout << "PASSED\n";
}

static void test_rp2040_cdc_dump_command() {
    std::cout << "Test 17: RP2040 CDC serial dump command trigger... ";
    RP2040Platform platform;
    bool ok = platform.initialize();
    assert(ok);

    // Record an event
    platform.setTimestamp(100);
    const uint8_t report_z[6] = {0x1D, 0, 0, 0, 0, 0};  // Z
    platform.processHostKeyboardReport(0, report_z, 6);

    // Trigger dump via CDC callback interface
    platform.clearEmittedKeys();
    tff_rp2040_cdc_dump();

    const std::string& dump = platform.getLastDebugDump();
    assert(!dump.empty());
    assert(dump.find("=== TFF RP2040 DEBUG DUMP ===") != std::string::npos);
    assert(dump.find("KEY_Z") != std::string::npos);

    // CDC dump does not type to HID, so emitted_down_keys should be empty
    assert(platform.getEmittedKeys().empty());

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
    test_unmapped_keys();
    test_one_shot_keys();
    test_raw_keyboard_report();
    test_host_keyboard_report_processing();
    test_host_report_queue();
    test_debug_buffer();
    test_rp2040_debug_dump_chord();
    test_rp2040_debug_dump_shift_d();
    test_rp2040_cdc_dump_command();

    std::cout << "\nAll RP2040 platform unified engine tests passed!\n";
    return 0;
}