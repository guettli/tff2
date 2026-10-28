#include "win_input.h"
#include "tff_key_codes.h"
#include <algorithm>

namespace tff {
namespace win {

namespace {

// clang-format off
// evdev code, set-1 scan code, E0-extended, virtual-key code, inject by virtual-key
const std::vector<KeyMapping> kKeyMappings = {
    {1,   0x01, false, 0x1B, false},  // KEY_ESC              VK_ESCAPE
    {2,   0x02, false, 0x31, false},  // KEY_1
    {3,   0x03, false, 0x32, false},  // KEY_2
    {4,   0x04, false, 0x33, false},  // KEY_3
    {5,   0x05, false, 0x34, false},  // KEY_4
    {6,   0x06, false, 0x35, false},  // KEY_5
    {7,   0x07, false, 0x36, false},  // KEY_6
    {8,   0x08, false, 0x37, false},  // KEY_7
    {9,   0x09, false, 0x38, false},  // KEY_8
    {10,  0x0A, false, 0x39, false},  // KEY_9
    {11,  0x0B, false, 0x30, false},  // KEY_0
    {12,  0x0C, false, 0xBD, false},  // KEY_MINUS            VK_OEM_MINUS
    {13,  0x0D, false, 0xBB, false},  // KEY_EQUAL            VK_OEM_PLUS
    {14,  0x0E, false, 0x08, false},  // KEY_BACKSPACE        VK_BACK
    {15,  0x0F, false, 0x09, false},  // KEY_TAB
    {16,  0x10, false, 0x51, false},  // KEY_Q
    {17,  0x11, false, 0x57, false},  // KEY_W
    {18,  0x12, false, 0x45, false},  // KEY_E
    {19,  0x13, false, 0x52, false},  // KEY_R
    {20,  0x14, false, 0x54, false},  // KEY_T
    {21,  0x15, false, 0x59, false},  // KEY_Y
    {22,  0x16, false, 0x55, false},  // KEY_U
    {23,  0x17, false, 0x49, false},  // KEY_I
    {24,  0x18, false, 0x4F, false},  // KEY_O
    {25,  0x19, false, 0x50, false},  // KEY_P
    {26,  0x1A, false, 0xDB, false},  // KEY_LEFTBRACE        VK_OEM_4
    {27,  0x1B, false, 0xDD, false},  // KEY_RIGHTBRACE       VK_OEM_6
    {28,  0x1C, false, 0x0D, false},  // KEY_ENTER            VK_RETURN
    {29,  0x1D, false, 0xA2, false},  // KEY_LEFTCTRL         VK_LCONTROL
    {30,  0x1E, false, 0x41, false},  // KEY_A
    {31,  0x1F, false, 0x53, false},  // KEY_S
    {32,  0x20, false, 0x44, false},  // KEY_D
    {33,  0x21, false, 0x46, false},  // KEY_F
    {34,  0x22, false, 0x47, false},  // KEY_G
    {35,  0x23, false, 0x48, false},  // KEY_H
    {36,  0x24, false, 0x4A, false},  // KEY_J
    {37,  0x25, false, 0x4B, false},  // KEY_K
    {38,  0x26, false, 0x4C, false},  // KEY_L
    {39,  0x27, false, 0xBA, false},  // KEY_SEMICOLON        VK_OEM_1
    {40,  0x28, false, 0xDE, false},  // KEY_APOSTROPHE       VK_OEM_7
    {41,  0x29, false, 0xC0, false},  // KEY_GRAVE            VK_OEM_3
    {42,  0x2A, false, 0xA0, false},  // KEY_LEFTSHIFT        VK_LSHIFT
    {43,  0x2B, false, 0xDC, false},  // KEY_BACKSLASH        VK_OEM_5
    {44,  0x2C, false, 0x5A, false},  // KEY_Z
    {45,  0x2D, false, 0x58, false},  // KEY_X
    {46,  0x2E, false, 0x43, false},  // KEY_C
    {47,  0x2F, false, 0x56, false},  // KEY_V
    {48,  0x30, false, 0x42, false},  // KEY_B
    {49,  0x31, false, 0x4E, false},  // KEY_N
    {50,  0x32, false, 0x4D, false},  // KEY_M
    {51,  0x33, false, 0xBC, false},  // KEY_COMMA            VK_OEM_COMMA
    {52,  0x34, false, 0xBE, false},  // KEY_DOT              VK_OEM_PERIOD
    {53,  0x35, false, 0xBF, false},  // KEY_SLASH            VK_OEM_2
    {54,  0x36, false, 0xA1, false},  // KEY_RIGHTSHIFT       VK_RSHIFT
    {55,  0x37, false, 0x6A, false},  // KEY_KPASTERISK       VK_MULTIPLY
    {56,  0x38, false, 0xA4, false},  // KEY_LEFTALT          VK_LMENU
    {57,  0x39, false, 0x20, false},  // KEY_SPACE
    {58,  0x3A, false, 0x14, false},  // KEY_CAPSLOCK         VK_CAPITAL
    {59,  0x3B, false, 0x70, false},  // KEY_F1
    {60,  0x3C, false, 0x71, false},  // KEY_F2
    {61,  0x3D, false, 0x72, false},  // KEY_F3
    {62,  0x3E, false, 0x73, false},  // KEY_F4
    {63,  0x3F, false, 0x74, false},  // KEY_F5
    {64,  0x40, false, 0x75, false},  // KEY_F6
    {65,  0x41, false, 0x76, false},  // KEY_F7
    {66,  0x42, false, 0x77, false},  // KEY_F8
    {67,  0x43, false, 0x78, false},  // KEY_F9
    {68,  0x44, false, 0x79, false},  // KEY_F10
    // NumLock and Pause share scan code 0x45; the hook reports NumLock as extended.
    {69,  0x45, true,  0x90, true},   // KEY_NUMLOCK          VK_NUMLOCK
    {70,  0x46, false, 0x91, false},  // KEY_SCROLLLOCK       VK_SCROLL
    {71,  0x47, false, 0x67, false},  // KEY_KP7              VK_NUMPAD7
    {72,  0x48, false, 0x68, false},  // KEY_KP8
    {73,  0x49, false, 0x69, false},  // KEY_KP9
    {74,  0x4A, false, 0x6D, false},  // KEY_KPMINUS          VK_SUBTRACT
    {75,  0x4B, false, 0x64, false},  // KEY_KP4
    {76,  0x4C, false, 0x65, false},  // KEY_KP5
    {77,  0x4D, false, 0x66, false},  // KEY_KP6
    {78,  0x4E, false, 0x6B, false},  // KEY_KPPLUS           VK_ADD
    {79,  0x4F, false, 0x61, false},  // KEY_KP1
    {80,  0x50, false, 0x62, false},  // KEY_KP2
    {81,  0x51, false, 0x63, false},  // KEY_KP3
    {82,  0x52, false, 0x60, false},  // KEY_KP0
    {83,  0x53, false, 0x6E, false},  // KEY_KPDOT            VK_DECIMAL
    {86,  0x56, false, 0xE2, false},  // KEY_102ND            VK_OEM_102 (ISO <>|)
    {87,  0x57, false, 0x7A, false},  // KEY_F11
    {88,  0x58, false, 0x7B, false},  // KEY_F12
    {89,  0x73, false, 0xC1, false},  // KEY_RO               VK_ABNT_C1
    {92,  0x79, false, 0x1C, false},  // KEY_HENKAN           VK_CONVERT
    {93,  0x70, false, 0x00, false},  // KEY_KATAKANAHIRAGANA
    {94,  0x7B, false, 0x1D, false},  // KEY_MUHENKAN         VK_NONCONVERT
    {96,  0x1C, true,  0x0D, false},  // KEY_KPENTER          VK_RETURN
    {97,  0x1D, true,  0xA3, false},  // KEY_RIGHTCTRL        VK_RCONTROL
    {98,  0x35, true,  0x6F, false},  // KEY_KPSLASH          VK_DIVIDE
    {99,  0x37, true,  0x2C, false},  // KEY_SYSRQ            VK_SNAPSHOT (Print Screen)
    {100, 0x38, true,  0xA5, false},  // KEY_RIGHTALT         VK_RMENU (AltGr)
    {102, 0x47, true,  0x24, false},  // KEY_HOME
    {103, 0x48, true,  0x26, false},  // KEY_UP
    {104, 0x49, true,  0x21, false},  // KEY_PAGEUP           VK_PRIOR
    {105, 0x4B, true,  0x25, false},  // KEY_LEFT
    {106, 0x4D, true,  0x27, false},  // KEY_RIGHT
    {107, 0x4F, true,  0x23, false},  // KEY_END
    {108, 0x50, true,  0x28, false},  // KEY_DOWN
    {109, 0x51, true,  0x22, false},  // KEY_PAGEDOWN         VK_NEXT
    {110, 0x52, true,  0x2D, false},  // KEY_INSERT
    {111, 0x53, true,  0x2E, false},  // KEY_DELETE
    {113, 0x20, true,  0xAD, true},   // KEY_MUTE             VK_VOLUME_MUTE
    {114, 0x2E, true,  0xAE, true},   // KEY_VOLUMEDOWN       VK_VOLUME_DOWN
    {115, 0x30, true,  0xAF, true},   // KEY_VOLUMEUP         VK_VOLUME_UP
    {117, 0x59, false, 0x92, false},  // KEY_KPEQUAL          VK_OEM_NEC_EQUAL
    {119, 0x45, false, 0x13, true},   // KEY_PAUSE            VK_PAUSE (E1 sequence in hardware)
    {121, 0x7E, false, 0xC2, false},  // KEY_KPCOMMA          VK_ABNT_C2
    {122, 0x72, false, 0x15, false},  // KEY_HANGEUL          VK_HANGUL
    {123, 0x71, false, 0x19, false},  // KEY_HANJA            VK_HANJA
    {124, 0x7D, false, 0x00, false},  // KEY_YEN
    {125, 0x5B, true,  0x5B, false},  // KEY_LEFTMETA         VK_LWIN
    {126, 0x5C, true,  0x5C, false},  // KEY_RIGHTMETA        VK_RWIN
    {127, 0x5D, true,  0x5D, false},  // KEY_COMPOSE          VK_APPS (context menu)
    {128, 0x68, true,  0xA9, true},   // KEY_STOP             VK_BROWSER_STOP
    {140, 0x21, true,  0xB7, true},   // KEY_CALC             VK_LAUNCH_APP2
    {155, 0x6C, true,  0xB4, true},   // KEY_MAIL             VK_LAUNCH_MAIL
    {156, 0x66, true,  0xAB, true},   // KEY_BOOKMARKS        VK_BROWSER_FAVORITES
    {157, 0x6B, true,  0xB6, true},   // KEY_COMPUTER         VK_LAUNCH_APP1
    {158, 0x6A, true,  0xA6, true},   // KEY_BACK             VK_BROWSER_BACK
    {159, 0x69, true,  0xA7, true},   // KEY_FORWARD          VK_BROWSER_FORWARD
    {163, 0x19, true,  0xB0, true},   // KEY_NEXTSONG         VK_MEDIA_NEXT_TRACK
    {164, 0x22, true,  0xB3, true},   // KEY_PLAYPAUSE        VK_MEDIA_PLAY_PAUSE
    {165, 0x10, true,  0xB1, true},   // KEY_PREVIOUSSONG     VK_MEDIA_PREV_TRACK
    {166, 0x24, true,  0xB2, true},   // KEY_STOPCD           VK_MEDIA_STOP
    {171, 0x6D, true,  0xB5, true},   // KEY_CONFIG           VK_LAUNCH_MEDIA_SELECT
    {172, 0x32, true,  0xAC, true},   // KEY_HOMEPAGE         VK_BROWSER_HOME
    {173, 0x67, true,  0xA8, true},   // KEY_REFRESH          VK_BROWSER_REFRESH
    {183, 0x64, false, 0x7C, false},  // KEY_F13
    {184, 0x65, false, 0x7D, false},  // KEY_F14
    {185, 0x66, false, 0x7E, false},  // KEY_F15
    {186, 0x67, false, 0x7F, false},  // KEY_F16
    {187, 0x68, false, 0x80, false},  // KEY_F17
    {188, 0x69, false, 0x81, false},  // KEY_F18
    {189, 0x6A, false, 0x82, false},  // KEY_F19
    {190, 0x6B, false, 0x83, false},  // KEY_F20
    {191, 0x6C, false, 0x84, false},  // KEY_F21
    {192, 0x6D, false, 0x85, false},  // KEY_F22
    {193, 0x6E, false, 0x86, false},  // KEY_F23
    {194, 0x76, false, 0x87, false},  // KEY_F24
    {217, 0x65, true,  0xAA, true},   // KEY_SEARCH           VK_BROWSER_SEARCH
};
// clang-format on

const KeyMapping* findByCode(KeyCode code) {
    for (const auto& m : kKeyMappings) {
        if (m.code == code) {
            return &m;
        }
    }
    return nullptr;
}

bool isMouseButton(KeyCode code) {
    return code >= Keys::BTN_LEFT && code <= Keys::BTN_EXTRA;
}

}  // namespace

const std::vector<KeyMapping>& keyMappings() {
    return kKeyMappings;
}

bool keyCodeToMapping(KeyCode code, KeyMapping& out) {
    const KeyMapping* m = findByCode(code);
    if (m == nullptr) {
        return false;
    }
    out = *m;
    return true;
}

bool hookToKeyCode(uint32_t vk, uint32_t scan, bool extended, KeyCode& out) {
    // Keys whose scan code is ambiguous or missing: decide by virtual-key.
    if (vk == kVkPause || vk == kVkCancel) {  // Pause, Ctrl+Pause (Break)
        out = Keys::KEY_PAUSE;
        return true;
    }
    if (vk == kVkNumLock) {
        out = Keys::KEY_NUMLOCK;
        return true;
    }
    if (scan != 0) {
        for (const auto& m : kKeyMappings) {
            if (m.scan == scan && m.extended == extended && m.code != Keys::KEY_PAUSE) {
                out = m.code;
                return true;
            }
        }
    }
    // Some devices (consumer-control keys, injected input) only report a virtual-key.
    if (vk != 0) {
        for (const auto& m : kKeyMappings) {
            if (m.vk == vk && m.output_vk) {
                out = m.code;
                return true;
            }
        }
    }
    return false;
}

HookDecision InputTranslator::translate(const HookEvent& in, TimeVal now, std::vector<Event>& out) {
    out.clear();
    if (in.extra_info == kInjectedMagic) {
        return HookDecision::PassThrough;  // our own output
    }
    const bool injected = (in.flags & kLlkhfInjected) != 0;
    if (in.scan == kAltGrFakeCtrlScan) {
        // Windows puts a fake LCtrl in front of AltGr. For a physical AltGr, drop it: Windows
        // adds it again when we inject RightAlt. The one belonging to our injection passes.
        return injected ? HookDecision::PassThrough : HookDecision::Drop;
    }
    if (injected && ignore_injected_) {
        return HookDecision::PassThrough;
    }
    if (in.vk == kVkPacket) {
        return HookDecision::PassThrough;  // KEYEVENTF_UNICODE text from other software
    }
    const bool extended = (in.flags & kLlkhfExtended) != 0;
    if (extended && (in.scan == 0x2A || in.scan == 0x36)) {
        // Fake shifts (E0 2A / E0 36) some keyboards wrap around navigation keys.
        return HookDecision::Drop;
    }

    KeyCode code = 0;
    if (!hookToKeyCode(in.vk, in.scan, extended, code)) {
        return HookDecision::PassThrough;
    }

    Event ev;
    ev.time = now;
    ev.type = EV_KEY;
    ev.code = code;

    if ((in.flags & kLlkhfUp) != 0) {
        if (down_.erase(code) == 0) {
            // Pressed before TFF started (or while paused): Windows saw the key-down, so it
            // must see the key-up too, or the key stays stuck.
            return HookDecision::PassThrough;
        }
        ev.value = KEY_VAL_UP;
        out.push_back(ev);
        return HookDecision::Process;
    }

    auto it = down_.find(code);
    if (it == down_.end()) {
        down_[code] = now;
        ev.value = KEY_VAL_DOWN;
        out.push_back(ev);
    } else if (timeSubMicros(it->second, now) > stale_after_us_) {
        // The key-up got lost: resynchronize the engine with a key-up before the new press.
        it->second = now;
        ev.value = KEY_VAL_UP;
        out.push_back(ev);
        ev.value = KEY_VAL_DOWN;
        out.push_back(ev);
    } else {
        it->second = now;
        ev.value = KEY_VAL_REPEAT;
        out.push_back(ev);
    }
    return HookDecision::Process;
}

void OutputBuilder::writeOne(const Event& ev) {
    if (observer_) {
        observer_(ev);
    }
    switch (ev.type) {
        case EV_SYN:
            flush();
            break;
        case EV_KEY:
            if (ev.value == KEY_VAL_REPEAT) {
                onPhysicalRepeat();
            } else if (isMouseButton(ev.code)) {
                OutputInput in;
                in.type = OutputInput::Type::MouseButton;
                in.button = ev.code;
                in.up = ev.value == KEY_VAL_UP;
                pending_.push_back(in);
                auto it = std::find(held_.begin(), held_.end(), ev.code);
                if (in.up && it != held_.end()) {
                    held_.erase(it);
                } else if (!in.up && it == held_.end()) {
                    held_.push_back(ev.code);
                }
            } else {
                addKey(ev.code, ev.value == KEY_VAL_UP);
            }
            break;
        case EV_REL: {
            OutputInput in;
            if (ev.code == REL_X) {
                in.type = OutputInput::Type::MouseMove;
                in.dx = ev.value;
            } else if (ev.code == REL_Y) {
                in.type = OutputInput::Type::MouseMove;
                in.dy = ev.value;
            } else if (ev.code == REL_WHEEL) {
                in.type = OutputInput::Type::Wheel;
                in.dx = ev.value;
            } else if (ev.code == REL_HWHEEL) {
                in.type = OutputInput::Type::HWheel;
                in.dx = ev.value;
            } else {
                break;
            }
            pending_.push_back(in);
            break;
        }
        default:
            break;
    }
}

void OutputBuilder::addKey(KeyCode code, bool up) {
    OutputInput in;
    in.type = OutputInput::Type::Key;
    if (!keyCodeToMapping(code, in.key)) {
        return;
    }
    in.up = up;
    pending_.push_back(in);

    auto it = std::find(held_.begin(), held_.end(), code);
    if (up) {
        if (it != held_.end()) {
            held_.erase(it);
        }
        if (repeat_key_ == code) {
            repeat_key_ = 0;
        }
    } else {
        if (it == held_.end()) {
            held_.push_back(code);
        }
        // Like a real keyboard, only the most recently pressed key repeats; pressing a
        // modifier stops the repeat of the key held before it.
        repeat_key_ = isModifier(code) ? 0 : code;
    }
}

void OutputBuilder::flush() {
    if (pending_.empty()) {
        return;
    }
    if (sender_) {
        sender_(pending_);
    }
    pending_.clear();
}

void OutputBuilder::onPhysicalRepeat() {
    if (repeat_key_ == 0) {
        return;
    }
    OutputInput in;
    in.type = OutputInput::Type::Key;
    if (!keyCodeToMapping(repeat_key_, in.key)) {
        return;
    }
    in.up = false;
    pending_.push_back(in);
    flush();
}

void OutputBuilder::releaseAll() {
    for (auto it = held_.rbegin(); it != held_.rend(); ++it) {
        OutputInput in;
        in.up = true;
        if (isMouseButton(*it)) {
            in.type = OutputInput::Type::MouseButton;
            in.button = *it;
        } else {
            in.type = OutputInput::Type::Key;
            if (!keyCodeToMapping(*it, in.key)) {
                continue;
            }
        }
        pending_.push_back(in);
    }
    held_.clear();
    repeat_key_ = 0;
    flush();
}

bool OutputBuilder::isHeld(KeyCode code) const {
    return std::find(held_.begin(), held_.end(), code) != held_.end();
}

}  // namespace win
}  // namespace tff
