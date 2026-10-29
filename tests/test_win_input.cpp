// Tests for the platform-neutral part of the Windows port (include/win_input.h).
// Runs on every platform; no <windows.h> needed.
#include "win_input.h"
#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include <cassert>
#include <iostream>
#include <set>
#include <utility>
#include <vector>

// assert() on a call with side effects, without cppcheck's assertWithSideEffect warning
#define CHECK(expr)                                                                                \
    do {                                                                                           \
        const bool check_ok_ = (expr);                                                             \
        assert(check_ok_);                                                                         \
    } while (0)

using namespace tff;
using namespace tff::win;

namespace {

constexpr uint16_t kVkA = 0x41;
constexpr uint16_t kVkF = 0x46;
constexpr uint16_t kVkJ = 0x4A;
constexpr uint16_t kVkRMenu = 0xA5;
constexpr uint16_t kVkLControl = 0xA2;
constexpr uint16_t kVkF13 = 0x7C;
constexpr uint16_t kVkMediaNext = 0xB0;

TimeVal ms(int64_t v) {
    return TimeVal::fromMicros(v * 1000);
}

HookEvent hook(uint16_t vk, uint32_t scan, bool up, uint32_t extra_flags = 0,
               uintptr_t extra_info = 0) {
    HookEvent ev;
    ev.vk = vk;
    ev.scan = scan;
    ev.flags = extra_flags | (up ? kLlkhfUp : 0);
    ev.extra_info = extra_info;
    return ev;
}

struct Recorder {
    std::vector<OutputInput> inputs;
    int batches = 0;
    bool reject = false;  ///< simulate SendInput failing (secure desktop, UIPI)
    void setReject(bool r) { reject = r; }
    OutputBuilder::Sender sender() {
        return [this](const std::vector<OutputInput>& batch) {
            ++batches;
            if (reject) {
                return false;
            }
            inputs.insert(inputs.end(), batch.begin(), batch.end());
            return true;
        };
    }
    std::vector<std::pair<KeyCode, bool>> keys() const {
        std::vector<std::pair<KeyCode, bool>> out;
        for (const auto& in : inputs) {
            if (in.type == OutputInput::Type::Key) {
                out.emplace_back(in.key.code, in.up);
            }
        }
        return out;
    }
};

void test_keymap_round_trip() {
    std::cout << "Test 1: Keymap round-trip evdev -> hook -> evdev... " << std::flush;
    std::set<std::pair<uint16_t, bool>> seen_scans;
    std::set<KeyCode> seen_codes;
    for (const auto& m : keyMappings()) {
        assert(seen_codes.insert(m.code).second);  // every evdev code once
        if (m.code != Keys::KEY_PAUSE) {
            assert(seen_scans.insert({m.scan, m.extended}).second);  // (scan, E0) unique
        }
        KeyCode back = 0;
        CHECK(hookToKeyCode(m.vk, m.scan, m.extended, back));
        assert(back == m.code);
        // Every mapped key has a name the YAML parser understands
        assert(!keyCodeToWord(m.code).empty());
    }
    assert(keyMappings().size() > 100);
    std::cout << "PASSED\n";
}

void test_keymap_known_keys() {
    std::cout << "Test 2: Known scan codes and special keys... " << std::flush;
    KeyMapping m;
    CHECK(keyCodeToMapping(Keys::KEY_A, m) && m.scan == 0x1E && !m.extended && m.vk == kVkA);
    CHECK(keyCodeToMapping(Keys::KEY_RIGHTALT, m) && m.scan == 0x38 && m.extended);
    CHECK(keyCodeToMapping(Keys::KEY_DELETE, m) && m.scan == 0x53 && m.extended);
    CHECK(keyCodeToMapping(Keys::KEY_PAUSE, m) && m.output_vk && m.vk == kVkPause);
    CHECK(!keyCodeToMapping(Keys::BTN_LEFT, m));

    KeyCode code = 0;
    // NumLock and Pause share scan 0x45 and are told apart by virtual-key
    CHECK(hookToKeyCode(kVkNumLock, 0x45, true, code) && code == Keys::KEY_NUMLOCK);
    CHECK(hookToKeyCode(kVkPause, 0x45, false, code) && code == Keys::KEY_PAUSE);
    // Ctrl+Pause arrives as VK_CANCEL (Break)
    CHECK(hookToKeyCode(kVkCancel, 0x46, true, code) && code == Keys::KEY_PAUSE);
    // Extended vs. non-extended: KP Enter vs. Enter, Right Ctrl vs. Left Ctrl
    CHECK(hookToKeyCode(0x0D, 0x1C, false, code) && code == Keys::KEY_ENTER);
    CHECK(hookToKeyCode(0x0D, 0x1C, true, code) && code == Keys::KEY_KPENTER);
    CHECK(hookToKeyCode(0xA3, 0x1D, true, code) && code == Keys::KEY_RIGHTCTRL);
    // Media key reported without scan code falls back to the virtual-key
    CHECK(hookToKeyCode(kVkMediaNext, 0, false, code) && code == 163 /* KEY_NEXTSONG */);
    CHECK(hookToKeyCode(kVkF13, 0x64, false, code) && code == 183 /* KEY_F13 */);
    // Unknown key
    CHECK(!hookToKeyCode(0xFF, 0x00, false, code));
    std::cout << "PASSED\n";
}

void test_translator_pass_through() {
    std::cout << "Test 3: Translator passes own output and unmapped keys through... " << std::flush;
    InputTranslator tr;
    std::vector<Event> out;
    // Our own injected output
    CHECK(tr.translate(hook(kVkA, 0x1E, false, kLlkhfInjected, kInjectedMagic), ms(1), out) ==
          HookDecision::PassThrough);
    assert(out.empty());
    // Unicode text injected by other software
    CHECK(tr.translate(hook(kVkPacket, 0, false, kLlkhfInjected), ms(2), out) ==
          HookDecision::PassThrough);
    // Unmapped key
    CHECK(tr.translate(hook(0xFF, 0x00, false), ms(3), out) == HookDecision::PassThrough);
    // Fake shift around navigation keys
    CHECK(tr.translate(hook(0xA0, 0x2A, false, kLlkhfExtended), ms(4), out) == HookDecision::Drop);
    assert(tr.downCount() == 0);
    std::cout << "PASSED\n";
}

void test_translator_altgr() {
    std::cout << "Test 4: AltGr fake LCtrl is dropped (physical) or passed (injected)... "
              << std::flush;
    InputTranslator tr;
    std::vector<Event> out;
    CHECK(tr.translate(hook(kVkLControl, kAltGrFakeCtrlScan, false), ms(1), out) ==
          HookDecision::Drop);
    CHECK(tr.translate(hook(kVkRMenu, 0x38, false, kLlkhfExtended), ms(1), out) ==
          HookDecision::Process);
    assert(out.size() == 1 && out[0].code == Keys::KEY_RIGHTALT);
    // The fake LCtrl Windows adds to our injected RightAlt passes through
    CHECK(tr.translate(hook(kVkLControl, kAltGrFakeCtrlScan, false, kLlkhfInjected), ms(2), out) ==
          HookDecision::PassThrough);
    std::cout << "PASSED\n";
}

void test_translator_down_repeat_up() {
    std::cout << "Test 5: Translator down / repeat / up / stale key-up... " << std::flush;
    InputTranslator tr;
    std::vector<Event> out;
    CHECK(tr.translate(hook(kVkA, 0x1E, false), ms(100), out) == HookDecision::Process);
    assert(out.size() == 1 && out[0].code == Keys::KEY_A && out[0].value == KEY_VAL_DOWN);
    assert(out[0].time == ms(100));
    assert(tr.isDown(Keys::KEY_A));

    // Auto-repeat while held
    tr.translate(hook(kVkA, 0x1E, false), ms(600), out);
    assert(out.size() == 1 && out[0].value == KEY_VAL_REPEAT);
    tr.translate(hook(kVkA, 0x1E, false), ms(633), out);
    assert(out.size() == 1 && out[0].value == KEY_VAL_REPEAT);

    tr.translate(hook(kVkA, 0x1E, true), ms(700), out);
    assert(out.size() == 1 && out[0].value == KEY_VAL_UP);
    assert(!tr.isDown(Keys::KEY_A));

    // Key-up lost (e.g. Win+L): the next press long after resynchronizes with up + down
    tr.translate(hook(kVkF, 0x21, false), ms(1000), out);
    tr.translate(hook(kVkF, 0x21, false), ms(9000), out);
    assert(out.size() == 2);
    assert(out[0].code == Keys::KEY_F && out[0].value == KEY_VAL_UP);
    assert(out[1].code == Keys::KEY_F && out[1].value == KEY_VAL_DOWN);

    // Key-up of a key pressed before TFF started passes through to Windows
    CHECK(tr.translate(hook(kVkJ, 0x24, true), ms(9100), out) == HookDecision::PassThrough);

    tr.reset();
    assert(tr.downCount() == 0);
    std::cout << "PASSED\n";
}

void test_translator_ignore_injected() {
    std::cout << "Test 6: --ignore-injected passes foreign injected input through... "
              << std::flush;
    InputTranslator tr;
    std::vector<Event> out;
    // By default, input injected by other software is remapped like physical input
    CHECK(tr.translate(hook(kVkA, 0x1E, false, kLlkhfInjected), ms(1), out) ==
          HookDecision::Process);
    tr.setIgnoreInjected(true);
    CHECK(tr.translate(hook(kVkJ, 0x24, false, kLlkhfInjected), ms(2), out) ==
          HookDecision::PassThrough);
    CHECK(tr.translate(hook(kVkJ, 0x24, false), ms(3), out) == HookDecision::Process);
    std::cout << "PASSED\n";
}

void test_output_builder_with_engine() {
    std::cout << "Test 7: Hook -> engine -> OutputBuilder pipeline (f j -> delete)... "
              << std::flush;
    Config config;
    std::string err;
    bool ok = loadYamlConfig("combos:\n  - keys: f j\n    outKeys: delete\n", config, err);
    assert(ok);

    Recorder rec;
    OutputBuilder builder(rec.sender());
    TFFEngine engine(&builder);
    engine.setCombos(config.combos);
    engine.setSettings(config.settings);

    InputTranslator tr;
    std::vector<Event> evs;
    auto feed = [&](const HookEvent& h, int64_t t) {
        HookDecision d = tr.translate(h, ms(t), evs);
        for (const auto& ev : evs) {
            engine.processEvent(ev);
        }
        builder.flush();
        return d;
    };

    // Plain key: buffered by the engine, emitted after the combo timeout
    CHECK(feed(hook(kVkA, 0x1E, false), 1000) == HookDecision::Process);
    engine.onTimer(ms(1100));
    builder.flush();
    CHECK(feed(hook(kVkA, 0x1E, true), 1150) == HookDecision::Process);
    auto keys = rec.keys();
    assert(keys.size() == 2);
    assert(keys[0] == std::make_pair(Keys::KEY_A, false));
    assert(keys[1] == std::make_pair(Keys::KEY_A, true));

    // Overlapping chord
    rec.inputs.clear();
    feed(hook(kVkF, 0x21, false), 2000);
    feed(hook(kVkJ, 0x24, false), 2010);
    feed(hook(kVkF, 0x21, true), 2050);
    feed(hook(kVkJ, 0x24, true), 2060);
    keys = rec.keys();
    assert(keys.size() == 2);
    assert(keys[0] == std::make_pair(Keys::KEY_DELETE, false));
    assert(keys[1] == std::make_pair(Keys::KEY_DELETE, true));
    assert(rec.inputs[0].key.extended && rec.inputs[0].key.scan == 0x53);
    assert(builder.heldCount() == 0);
    std::cout << "PASSED\n";
}

void test_output_builder_repeat() {
    std::cout << "Test 8: Physical auto-repeat re-sends the last held output key... " << std::flush;
    Recorder rec;
    OutputBuilder builder(rec.sender());
    builder.onPhysicalRepeat();  // nothing held: no output
    assert(rec.inputs.empty());

    builder.writeOne(Event{ms(1), EV_KEY, Keys::KEY_LEFTSHIFT, KEY_VAL_DOWN});
    builder.writeOne(Event{ms(1), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    builder.writeOne(Event{ms(1), EV_SYN, SYN_REPORT, 0});
    assert(builder.repeatKey() == Keys::KEY_A);
    builder.onPhysicalRepeat();
    builder.onPhysicalRepeat();
    auto keys = rec.keys();
    assert(keys.size() == 4);
    assert(keys[2] == std::make_pair(Keys::KEY_A, false));
    assert(keys[3] == std::make_pair(Keys::KEY_A, false));

    // Pressing a modifier stops the repeat (like a real keyboard)
    builder.writeOne(Event{ms(2), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_DOWN});
    assert(builder.repeatKey() == 0);
    builder.flush();

    // Release everything still held, newest first
    rec.inputs.clear();
    builder.releaseAll();
    keys = rec.keys();
    assert(keys.size() == 3);
    assert(keys[0] == std::make_pair(Keys::KEY_LEFTCTRL, true));
    assert(keys[1] == std::make_pair(Keys::KEY_A, true));
    assert(keys[2] == std::make_pair(Keys::KEY_LEFTSHIFT, true));
    assert(builder.heldCount() == 0);
    std::cout << "PASSED\n";
}

void test_output_builder_mouse_and_batches() {
    std::cout << "Test 9: Mouse output and SYN batching... " << std::flush;
    Recorder rec;
    OutputBuilder builder(rec.sender());
    builder.writeOne(Event{ms(1), EV_REL, REL_X, 15});
    builder.writeOne(Event{ms(1), EV_SYN, SYN_REPORT, 0});
    builder.writeOne(Event{ms(1), EV_REL, REL_WHEEL, -1});
    builder.writeOne(Event{ms(1), EV_KEY, Keys::BTN_LEFT, KEY_VAL_DOWN});
    builder.writeOne(Event{ms(1), EV_SYN, SYN_REPORT, 0});
    builder.writeOne(Event{ms(1), EV_SYN, SYN_REPORT, 0});  // empty batch is not sent
    assert(rec.batches == 2);
    assert(rec.inputs.size() == 3);
    assert(rec.inputs[0].type == OutputInput::Type::MouseMove && rec.inputs[0].dx == 15);
    assert(rec.inputs[1].type == OutputInput::Type::Wheel && rec.inputs[1].dx == -1);
    assert(rec.inputs[2].type == OutputInput::Type::MouseButton &&
           rec.inputs[2].button == Keys::BTN_LEFT && !rec.inputs[2].up);
    assert(builder.isHeld(Keys::BTN_LEFT));
    builder.releaseAll();
    assert(rec.inputs.back().type == OutputInput::Type::MouseButton && rec.inputs.back().up);
    std::cout << "PASSED\n";
}

void test_translator_keys_held_before_tracking() {
    std::cout << "Test 10: Keys Windows already holds (startup, pause) pass through... "
              << std::flush;
    InputTranslator tr;
    std::vector<Event> out;
    // Held when the hook was installed: repeats and key-up go to Windows
    tr.markForeignDown(Keys::KEY_CAPSLOCK);
    CHECK(tr.translate(hook(0x14, 0x3A, false), ms(10), out) == HookDecision::PassThrough);
    CHECK(tr.translate(hook(0x14, 0x3A, true), ms(20), out) == HookDecision::PassThrough);
    assert(!tr.isForeign(Keys::KEY_CAPSLOCK));
    CHECK(tr.translate(hook(0x14, 0x3A, false), ms(30), out) == HookDecision::Process);

    // Pressed while paused, released after resume
    tr.observePassThrough(hook(kVkJ, 0x24, false));
    assert(tr.isForeign(Keys::KEY_J));
    CHECK(tr.translate(hook(kVkJ, 0x24, false), ms(40), out) == HookDecision::PassThrough);
    CHECK(tr.translate(hook(kVkJ, 0x24, true), ms(50), out) == HookDecision::PassThrough);

    // AltGr pressed while paused: its fake LCtrl key-up must reach Windows after resume
    tr.observePassThrough(hook(kVkLControl, kAltGrFakeCtrlScan, false));
    CHECK(tr.translate(hook(kVkLControl, kAltGrFakeCtrlScan, true), ms(60), out) ==
          HookDecision::PassThrough);
    // ... while a dropped key-down means a dropped key-up
    CHECK(tr.translate(hook(kVkLControl, kAltGrFakeCtrlScan, false), ms(70), out) ==
          HookDecision::Drop);
    CHECK(tr.translate(hook(kVkLControl, kAltGrFakeCtrlScan, true), ms(80), out) ==
          HookDecision::Drop);
    std::cout << "PASSED\n";
}

void test_output_builder_repeat_source_and_retry() {
    std::cout << "Test 11: New physical key stops repeat; failed release is retried... "
              << std::flush;
    Recorder rec;
    OutputBuilder builder(rec.sender());
    builder.writeOne(Event{ms(1), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    builder.flush();
    assert(builder.repeatKey() == Keys::KEY_A);
    // A layer key goes down (no output of its own): its repeats must not repeat `a`
    builder.onPhysicalDown();
    builder.onPhysicalRepeat();
    assert(rec.keys().size() == 1);

    // Secure desktop: the key-up is rejected, so `a` stays recorded as held ...
    rec.setReject(true);
    builder.releaseAll();
    assert(builder.isHeld(Keys::KEY_A));
    // Same for key-ups the engine itself writes (e.g. engine.reset() releasing a tap-hold)
    builder.writeOne(Event{ms(2), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_DOWN});
    rec.setReject(false);
    builder.flush();
    rec.setReject(true);
    builder.writeOne(Event{ms(3), EV_KEY, Keys::KEY_LEFTCTRL, KEY_VAL_UP});
    builder.releaseAll();
    assert(builder.isHeld(Keys::KEY_LEFTCTRL) && builder.isHeld(Keys::KEY_A));
    // ... and is released on the next attempt (session unlocked)
    rec.setReject(false);
    builder.releaseAll();
    assert(!builder.isHeld(Keys::KEY_A));
    assert(!builder.isHeld(Keys::KEY_LEFTCTRL));
    auto keys = rec.keys();
    assert(keys.size() == 4);  // a down, ctrl down, then the retried ups
    assert(keys[2].second && keys[3].second);
    std::cout << "PASSED\n";
}

void test_output_builder_reentrant_flush() {
    std::cout << "Test 12: flush() re-entered from the sender doesn't resend the batch... "
              << std::flush;
    // SendInput can call the keyboard hook re-entrantly; the hook must not resend output.
    OutputBuilder* self = nullptr;
    int sent = 0;
    OutputBuilder builder([&](const std::vector<OutputInput>& batch) {
        sent += static_cast<int>(batch.size());
        if (sent < 100) {
            self->flush();
        }
        return true;
    });
    self = &builder;
    builder.writeOne(Event{ms(1), EV_KEY, Keys::KEY_A, KEY_VAL_DOWN});
    builder.writeOne(Event{ms(1), EV_SYN, SYN_REPORT, 0});
    assert(sent == 1);
    std::cout << "PASSED\n";
}

}  // namespace

void test_utf8_to_utf16() {
    std::cout << "Test 13: UTF-8 -> UTF-16 conversion... " << std::flush;
    using U = std::vector<uint16_t>;
    assert(utf8ToUtf16("") == U{});
    assert(utf8ToUtf16("A:") == (U{'A', ':'}));
    assert(utf8ToUtf16("\xC3\xBC\xE2\x82\xAC") == (U{0x00FC, 0x20AC}));  // ü €
    assert(utf8ToUtf16("\xF0\x9F\x98\x80") == (U{0xD83D, 0xDE00}));      // U+1F600
    assert(utf8ToUtf16("a\xFF"
                       "b") == (U{'a', 0xFFFD, 'b'}));                   // invalid byte
    assert(utf8ToUtf16("\xC3") == (U{0xFFFD}));                          // truncated
    assert(utf8ToUtf16("\xC0\xAF") == (U{0xFFFD, 0xFFFD}));              // overlong '/'
    assert(utf8ToUtf16("\xED\xA0\x80") == (U{0xFFFD, 0xFFFD, 0xFFFD}));  // surrogate
    std::cout << "PASSED\n";
}

void test_output_builder_text() {
    std::cout << "Test 14: Text snippets are typed as Unicode, Enter/Tab as keys... " << std::flush;
    Recorder rec;
    OutputBuilder builder(rec.sender());
    CHECK(builder.writeText("A\xC3\xBC\t\r\n"));
    assert(rec.batches == 4);  // one per character
    assert(rec.inputs.size() == 8);
    const uint16_t units[] = {'A', 'A', 0x00FC, 0x00FC};
    for (size_t i = 0; i < 4; ++i) {
        assert(rec.inputs[i].type == OutputInput::Type::Unicode);
        assert(rec.inputs[i].unit == units[i]);
        assert(rec.inputs[i].up == (i % 2 == 1));
    }
    auto keys = rec.keys();
    assert(keys.size() == 4);
    assert(keys[0] == std::make_pair(Keys::KEY_TAB, false));
    assert(keys[1] == std::make_pair(Keys::KEY_TAB, true));
    assert(keys[2] == std::make_pair(Keys::KEY_ENTER, false));
    assert(keys[3] == std::make_pair(Keys::KEY_ENTER, true));
    assert(builder.heldCount() == 0);

    // A surrogate pair is one character: one batch
    rec.inputs.clear();
    rec.batches = 0;
    CHECK(builder.writeText("\xF0\x9F\x98\x80"));
    assert(rec.batches == 1);
    assert(rec.inputs.size() == 4);
    assert(rec.inputs[0].unit == 0xD83D && !rec.inputs[0].up);
    assert(rec.inputs[1].unit == 0xD83D && rec.inputs[1].up);
    assert(rec.inputs[2].unit == 0xDE00 && !rec.inputs[2].up);

    // The engine hands text snippets to the builder instead of typing Shift + key strokes,
    // so no Shift (and no layout table) is involved
    Config config;
    std::string err;
    bool ok = loadYamlConfig("combos:\n  - keys: f j\n    outKeys: text \"Hi:\"\n", config, err);
    assert(ok);
    TFFEngine engine(&builder);
    engine.setCombos(config.combos);
    engine.setSettings(config.settings);
    rec.inputs.clear();
    engine.processEvent(Event{ms(1000), EV_KEY, Keys::KEY_F, KEY_VAL_DOWN});
    engine.processEvent(Event{ms(1010), EV_KEY, Keys::KEY_J, KEY_VAL_DOWN});
    engine.processEvent(Event{ms(1050), EV_KEY, Keys::KEY_F, KEY_VAL_UP});
    engine.processEvent(Event{ms(1060), EV_KEY, Keys::KEY_J, KEY_VAL_UP});
    builder.flush();
    assert(rec.keys().empty());
    std::vector<uint16_t> typed;
    for (const auto& in : rec.inputs) {
        if (in.type == OutputInput::Type::Unicode && !in.up) {
            typed.push_back(in.unit);
        }
    }
    assert(typed == (std::vector<uint16_t>{'H', 'i', ':'}));
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running Windows input translation tests:\n";
    test_keymap_round_trip();
    test_keymap_known_keys();
    test_translator_pass_through();
    test_translator_altgr();
    test_translator_down_repeat_up();
    test_translator_ignore_injected();
    test_output_builder_with_engine();
    test_output_builder_repeat();
    test_output_builder_mouse_and_batches();
    test_translator_keys_held_before_tracking();
    test_output_builder_repeat_source_and_retry();
    test_output_builder_reentrant_flush();
    test_utf8_to_utf16();
    test_output_builder_text();
    std::cout << "All Windows input translation tests PASSED!\n";
    return 0;
}
