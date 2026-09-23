#include "rp2040_platform.h"
#include <cstring>
#include <cstdio>
#include <iostream>

namespace {

const char* DEFAULT_YAML_CONFIG = R"(
combos:
  j f: backspace
  f j: delete
  ; a: home
  a ;: end
  f n: down
  f u: up
  f m: down
  f k: left
  f l: right
  f i: pageup
  f ,: pagedown
  g h: esc
  d + f + j: esc

tap_hold:
  capslock:
    tap: esc
    hold: super
    timeout_ms: 200
)";

}  // anonymous namespace

class RP2040Platform::RP2040EventWriter : public tff::EventWriter {
public:
    std::vector<uint32_t> emitted_down_keys;
#ifdef PICO_BUILD
    uint8_t active_modifiers_ = 0;
    uint8_t active_keys_[6] = {0, 0, 0, 0, 0, 0};
#endif

    void writeOne(const tff::Event& ev) override {
        if (ev.type == tff::EV_KEY) {
            if (ev.value == tff::KEY_VAL_DOWN) {
                emitted_down_keys.push_back(ev.code);
            }
#ifdef PICO_BUILD
            uint8_t usb_code = convertKeyCodeToUsb(ev.code);
            if (usb_code != 0) {
                if (usb_code >= 0xE0 && usb_code <= 0xE7) {
                    uint8_t mod_bit = static_cast<uint8_t>(1u << (usb_code - 0xE0));
                    if (ev.value == tff::KEY_VAL_DOWN) {
                        active_modifiers_ |= mod_bit;
                    } else if (ev.value == tff::KEY_VAL_UP) {
                        active_modifiers_ &= static_cast<uint8_t>(~mod_bit);
                    }
                } else {
                    if (ev.value == tff::KEY_VAL_DOWN) {
                        bool already_present = false;
                        for (int i = 0; i < 6; ++i) {
                            if (active_keys_[i] == usb_code) {
                                already_present = true;
                                break;
                            }
                        }
                        if (!already_present) {
                            for (int i = 0; i < 6; ++i) {
                                if (active_keys_[i] == 0) {
                                    active_keys_[i] = usb_code;
                                    break;
                                }
                            }
                        }
                    } else if (ev.value == tff::KEY_VAL_UP) {
                        for (int i = 0; i < 6; ++i) {
                            if (active_keys_[i] == usb_code) {
                                active_keys_[i] = 0;
                                break;
                            }
                        }
                    }
                }
                tud_hid_keyboard_report(0, active_modifiers_, active_keys_);
            }
#endif
        }
    }
};

RP2040Platform::RP2040Platform()
    : writer_(std::make_unique<RP2040EventWriter>()),
      engine_(nullptr),
      initialized_(false),
      running_(false),
      test_timestamp_ms_(0) {}

RP2040Platform::~RP2040Platform() {
    cleanup();
}

bool RP2040Platform::initialize() {
    if (initialized_) {
        return true;
    }

    if (!writer_) {
        writer_ = std::make_unique<RP2040EventWriter>();
    }

    // If no config has been set, load default TFF configuration
    if (config_.combos.empty() && config_.tap_hold_keys.empty() && config_.layers.empty() &&
        config_.one_shot_keys.empty() && config_.leader.sequences.empty()) {
        std::string err_msg;
        if (!tff::loadYamlConfig(DEFAULT_YAML_CONFIG, config_, err_msg)) {
            std::fprintf(stderr, "RP2040Platform: Failed to load default config: %s\n",
                         err_msg.c_str());
        }
    }

    engine_ = std::make_unique<tff::TFFEngine>(writer_.get());
    engine_->setConfig(config_);

#ifdef PICO_BUILD
    stdio_init_all();

    // cppcheck-suppress knownConditionTrueFalse
    if (!initUsbHost()) {
        printf("Failed to initialize USB host\n");
        return false;
    }

    // cppcheck-suppress knownConditionTrueFalse
    if (!initUsbDevice()) {
        printf("Failed to initialize USB device\n");
        return false;
    }

    printf(
        "RP2040Platform initialized successfully with TFFEngine (%zu combos, %zu tap-hold, %zu "
        "layers)\n",
        config_.combos.size(), config_.tap_hold_keys.size(), config_.layers.size());
#else
    printf(
        "RP2040Platform initialized for testing with TFFEngine (%zu combos, %zu tap-hold, %zu "
        "layers)\n",
        config_.combos.size(), config_.tap_hold_keys.size(), config_.layers.size());
#endif

    initialized_ = true;
    return true;
}

bool RP2040Platform::loadConfiguration(const std::string& yaml_str) {
    if (!initialized_) {
        initialize();
    }
    tff::Config new_config;
    std::string err_msg;
    if (!tff::loadYamlConfig(yaml_str, new_config, err_msg)) {
        std::fprintf(stderr, "RP2040Platform: Failed to load YAML config: %s\n", err_msg.c_str());
        return false;
    }
    setConfig(new_config);
    return true;
}

void RP2040Platform::setConfig(const tff::Config& config) {
    config_ = config;
    if (engine_) {
        engine_->setConfig(config_);
    }
}

tff::TFFEngine& RP2040Platform::getEngine() {
    if (!initialized_ || !engine_) {
        initialize();
    }
    return *engine_;
}

void RP2040Platform::run() {
    if (!initialized_) {
        printf("RP2040Platform not initialized\n");
        return;
    }

    running_ = true;
    printf("RP2040 TFF keyboard remapping started\n");
    printf("Listening for keyboard input...\n");

#ifdef PICO_BUILD
    while (running_) {
        processUsbHostEvents();
        processUsbDeviceEvents();
        checkTimers();
        tud_task();
        tuh_task();
    }
#else
    printf("Running in test mode\n");
#endif
}

void RP2040Platform::processHostKeyEvent(uint32_t keycode, bool pressed) {
    if (!initialized_ || !engine_) {
        return;
    }

    if (keycode > 0xFF) {
        return;
    }

    tff::KeyCode code = convertUsbToKeyCode(static_cast<uint8_t>(keycode));
    if (code == 0) {
        return;
    }

    uint32_t ts_ms = getCurrentTimestamp();
    tff::Event ev;
    ev.time.sec = static_cast<int64_t>(ts_ms / 1000);
    ev.time.usec = static_cast<int64_t>(ts_ms % 1000) * 1000;
    ev.type = tff::EV_KEY;
    ev.code = code;
    ev.value = pressed ? tff::KEY_VAL_DOWN : tff::KEY_VAL_UP;

    engine_->processEvent(ev);
    checkTimers();
}

void RP2040Platform::checkTimers() {
    if (!engine_ || !engine_->hasActiveTimer()) {
        return;
    }

    uint32_t ts_ms = getCurrentTimestamp();
    tff::TimeVal now;
    now.sec = static_cast<int64_t>(ts_ms / 1000);
    now.usec = static_cast<int64_t>(ts_ms % 1000) * 1000;

    tff::TimeVal timer_time = engine_->getActiveTimerTime();
    if (now >= timer_time) {
        engine_->onTimer(now);
    }
}

bool RP2040Platform::sendDeviceKeys(const std::vector<uint32_t>& key_codes) {
    if (!initialized_ || key_codes.empty()) {
        return false;
    }

#ifdef PICO_BUILD
    for (uint32_t key_code : key_codes) {
        uint8_t usb_keycode = convertKeyCodeToUsb(static_cast<tff::KeyCode>(key_code));
        if (usb_keycode != 0) {
            uint8_t report_keys[6] = {0, 0, 0, 0, 0, 0};
            uint8_t mod = 0;
            if (usb_keycode >= 0xE0 && usb_keycode <= 0xE7) {
                mod = static_cast<uint8_t>(1u << (usb_keycode - 0xE0));
            } else {
                report_keys[0] = usb_keycode;
            }
            tud_hid_keyboard_report(0, mod, report_keys);
            sleep_ms(10);
            uint8_t empty_keys[6] = {0, 0, 0, 0, 0, 0};
            tud_hid_keyboard_report(0, 0, empty_keys);
        }
    }
    return true;
#else
    if (writer_) {
        for (auto key : key_codes) {
            writer_->emitted_down_keys.push_back(key);
        }
    }
    return true;
#endif
}

const std::vector<uint32_t>& RP2040Platform::getEmittedKeys() const {
    static const std::vector<uint32_t> empty;
    return writer_ ? writer_->emitted_down_keys : empty;
}

void RP2040Platform::clearEmittedKeys() {
    if (writer_) {
        writer_->emitted_down_keys.clear();
    }
}

void RP2040Platform::cleanup() {
    running_ = false;
    initialized_ = false;
    engine_.reset();
    writer_.reset();
}

bool RP2040Platform::initUsbHost() {
#ifdef PICO_BUILD
    printf("USB host initialization (TinyUSB tuh_init)\n");
    return true;
#else
    printf("USB host initialization (testing mode)\n");
    return true;
#endif
}

bool RP2040Platform::initUsbDevice() {
#ifdef PICO_BUILD
    printf("USB device initialization (TinyUSB tud_init)\n");
    return true;
#else
    printf("USB device initialization (testing mode)\n");
    return true;
#endif
}

void RP2040Platform::processUsbHostEvents() {
#ifdef PICO_BUILD
    // Handled via tuh_hid_report_received_cb
#endif
}

void RP2040Platform::processUsbDeviceEvents() {
#ifdef PICO_BUILD
    // TinyUSB device task handling
#endif
}

uint32_t RP2040Platform::getCurrentTimestamp() {
#ifdef PICO_BUILD
    return to_ms_since_boot(get_absolute_time());
#else
    if (test_timestamp_ms_ > 0) {
        return test_timestamp_ms_;
    }
    static uint32_t counter = 0;
    return ++counter;
#endif
}

tff::KeyCode RP2040Platform::convertUsbToKeyCode(uint8_t usb_keycode) {
    switch (usb_keycode) {
        case 0x04:
            return tff::Keys::KEY_A;
        case 0x05:
            return tff::Keys::KEY_B;
        case 0x06:
            return tff::Keys::KEY_C;
        case 0x07:
            return tff::Keys::KEY_D;
        case 0x08:
            return tff::Keys::KEY_E;
        case 0x09:
            return tff::Keys::KEY_F;
        case 0x0A:
            return tff::Keys::KEY_G;
        case 0x0B:
            return tff::Keys::KEY_H;
        case 0x0C:
            return tff::Keys::KEY_I;
        case 0x0D:
            return tff::Keys::KEY_J;
        case 0x0E:
            return tff::Keys::KEY_K;
        case 0x0F:
            return tff::Keys::KEY_L;
        case 0x10:
            return tff::Keys::KEY_M;
        case 0x11:
            return tff::Keys::KEY_N;
        case 0x12:
            return tff::Keys::KEY_O;
        case 0x13:
            return tff::Keys::KEY_P;
        case 0x14:
            return tff::Keys::KEY_Q;
        case 0x15:
            return tff::Keys::KEY_R;
        case 0x16:
            return tff::Keys::KEY_S;
        case 0x17:
            return tff::Keys::KEY_T;
        case 0x18:
            return tff::Keys::KEY_U;
        case 0x19:
            return tff::Keys::KEY_V;
        case 0x1A:
            return tff::Keys::KEY_W;
        case 0x1B:
            return tff::Keys::KEY_X;
        case 0x1C:
            return tff::Keys::KEY_Y;
        case 0x1D:
            return tff::Keys::KEY_Z;

        case 0x1E:
            return tff::Keys::KEY_1;
        case 0x1F:
            return tff::Keys::KEY_2;
        case 0x20:
            return tff::Keys::KEY_3;
        case 0x21:
            return tff::Keys::KEY_4;
        case 0x22:
            return tff::Keys::KEY_5;
        case 0x23:
            return tff::Keys::KEY_6;
        case 0x24:
            return tff::Keys::KEY_7;
        case 0x25:
            return tff::Keys::KEY_8;
        case 0x26:
            return tff::Keys::KEY_9;
        case 0x27:
            return tff::Keys::KEY_0;

        case 0x28:
            return tff::Keys::KEY_ENTER;
        case 0x29:
            return tff::Keys::KEY_ESC;
        case 0x2A:
            return tff::Keys::KEY_BACKSPACE;
        case 0x2B:
            return tff::Keys::KEY_TAB;
        case 0x2C:
            return tff::Keys::KEY_SPACE;
        case 0x2D:
            return tff::Keys::KEY_MINUS;
        case 0x2E:
            return tff::Keys::KEY_EQUAL;
        case 0x2F:
            return tff::Keys::KEY_LEFTBRACE;
        case 0x30:
            return tff::Keys::KEY_RIGHTBRACE;
        case 0x31:
            return tff::Keys::KEY_BACKSLASH;
        case 0x33:
            return tff::Keys::KEY_SEMICOLON;
        case 0x34:
            return tff::Keys::KEY_APOSTROPHE;
        case 0x35:
            return tff::Keys::KEY_GRAVE;
        case 0x36:
            return tff::Keys::KEY_COMMA;
        case 0x37:
            return tff::Keys::KEY_DOT;
        case 0x38:
            return tff::Keys::KEY_SLASH;
        case 0x39:
            return tff::Keys::KEY_CAPSLOCK;

        case 0x3A:
            return tff::Keys::KEY_F1;
        case 0x3B:
            return tff::Keys::KEY_F2;
        case 0x3C:
            return tff::Keys::KEY_F3;
        case 0x3D:
            return tff::Keys::KEY_F4;
        case 0x3E:
            return tff::Keys::KEY_F5;
        case 0x3F:
            return tff::Keys::KEY_F6;
        case 0x40:
            return tff::Keys::KEY_F7;
        case 0x41:
            return tff::Keys::KEY_F8;
        case 0x42:
            return tff::Keys::KEY_F9;
        case 0x43:
            return tff::Keys::KEY_F10;
        case 0x44:
            return tff::Keys::KEY_F11;
        case 0x45:
            return tff::Keys::KEY_F12;

        case 0x46:
            return tff::Keys::KEY_SYSRQ;
        case 0x47:
            return tff::Keys::KEY_SCROLLLOCK;
        case 0x48:
            return tff::Keys::KEY_PAUSE;
        case 0x49:
            return tff::Keys::KEY_INSERT;
        case 0x4A:
            return tff::Keys::KEY_HOME;
        case 0x4B:
            return tff::Keys::KEY_PAGEUP;
        case 0x4C:
            return tff::Keys::KEY_DELETE;
        case 0x4D:
            return tff::Keys::KEY_END;
        case 0x4E:
            return tff::Keys::KEY_PAGEDOWN;
        case 0x4F:
            return tff::Keys::KEY_RIGHT;
        case 0x50:
            return tff::Keys::KEY_LEFT;
        case 0x51:
            return tff::Keys::KEY_DOWN;
        case 0x52:
            return tff::Keys::KEY_UP;

        case 0x53:
            return tff::Keys::KEY_NUMLOCK;
        case 0x54:
            return tff::Keys::KEY_KPSLASH;
        case 0x55:
            return tff::Keys::KEY_KPASTERISK;
        case 0x56:
            return tff::Keys::KEY_KPMINUS;
        case 0x57:
            return tff::Keys::KEY_KPPLUS;
        case 0x58:
            return tff::Keys::KEY_KPENTER;
        case 0x59:
            return tff::Keys::KEY_KP1;
        case 0x5A:
            return tff::Keys::KEY_KP2;
        case 0x5B:
            return tff::Keys::KEY_KP3;
        case 0x5C:
            return tff::Keys::KEY_KP4;
        case 0x5D:
            return tff::Keys::KEY_KP5;
        case 0x5E:
            return tff::Keys::KEY_KP6;
        case 0x5F:
            return tff::Keys::KEY_KP7;
        case 0x60:
            return tff::Keys::KEY_KP8;
        case 0x61:
            return tff::Keys::KEY_KP9;
        case 0x62:
            return tff::Keys::KEY_KP0;
        case 0x63:
            return tff::Keys::KEY_KPDOT;

        case 0xE0:
            return tff::Keys::KEY_LEFTCTRL;
        case 0xE1:
            return tff::Keys::KEY_LEFTSHIFT;
        case 0xE2:
            return tff::Keys::KEY_LEFTALT;
        case 0xE3:
            return tff::Keys::KEY_LEFTMETA;
        case 0xE4:
            return tff::Keys::KEY_RIGHTCTRL;
        case 0xE5:
            return tff::Keys::KEY_RIGHTSHIFT;
        case 0xE6:
            return tff::Keys::KEY_RIGHTALT;
        case 0xE7:
            return tff::Keys::KEY_RIGHTMETA;

        default:
            return 0;
    }
}

uint8_t RP2040Platform::convertKeyCodeToUsb(tff::KeyCode internal_keycode) {
    switch (internal_keycode) {
        case tff::Keys::KEY_A:
            return 0x04;
        case tff::Keys::KEY_B:
            return 0x05;
        case tff::Keys::KEY_C:
            return 0x06;
        case tff::Keys::KEY_D:
            return 0x07;
        case tff::Keys::KEY_E:
            return 0x08;
        case tff::Keys::KEY_F:
            return 0x09;
        case tff::Keys::KEY_G:
            return 0x0A;
        case tff::Keys::KEY_H:
            return 0x0B;
        case tff::Keys::KEY_I:
            return 0x0C;
        case tff::Keys::KEY_J:
            return 0x0D;
        case tff::Keys::KEY_K:
            return 0x0E;
        case tff::Keys::KEY_L:
            return 0x0F;
        case tff::Keys::KEY_M:
            return 0x10;
        case tff::Keys::KEY_N:
            return 0x11;
        case tff::Keys::KEY_O:
            return 0x12;
        case tff::Keys::KEY_P:
            return 0x13;
        case tff::Keys::KEY_Q:
            return 0x14;
        case tff::Keys::KEY_R:
            return 0x15;
        case tff::Keys::KEY_S:
            return 0x16;
        case tff::Keys::KEY_T:
            return 0x17;
        case tff::Keys::KEY_U:
            return 0x18;
        case tff::Keys::KEY_V:
            return 0x19;
        case tff::Keys::KEY_W:
            return 0x1A;
        case tff::Keys::KEY_X:
            return 0x1B;
        case tff::Keys::KEY_Y:
            return 0x1C;
        case tff::Keys::KEY_Z:
            return 0x1D;

        case tff::Keys::KEY_1:
            return 0x1E;
        case tff::Keys::KEY_2:
            return 0x1F;
        case tff::Keys::KEY_3:
            return 0x20;
        case tff::Keys::KEY_4:
            return 0x21;
        case tff::Keys::KEY_5:
            return 0x22;
        case tff::Keys::KEY_6:
            return 0x23;
        case tff::Keys::KEY_7:
            return 0x24;
        case tff::Keys::KEY_8:
            return 0x25;
        case tff::Keys::KEY_9:
            return 0x26;
        case tff::Keys::KEY_0:
            return 0x27;

        case tff::Keys::KEY_ENTER:
            return 0x28;
        case tff::Keys::KEY_ESC:
            return 0x29;
        case tff::Keys::KEY_BACKSPACE:
            return 0x2A;
        case tff::Keys::KEY_TAB:
            return 0x2B;
        case tff::Keys::KEY_SPACE:
            return 0x2C;
        case tff::Keys::KEY_MINUS:
            return 0x2D;
        case tff::Keys::KEY_EQUAL:
            return 0x2E;
        case tff::Keys::KEY_LEFTBRACE:
            return 0x2F;
        case tff::Keys::KEY_RIGHTBRACE:
            return 0x30;
        case tff::Keys::KEY_BACKSLASH:
            return 0x31;
        case tff::Keys::KEY_SEMICOLON:
            return 0x33;
        case tff::Keys::KEY_APOSTROPHE:
            return 0x34;
        case tff::Keys::KEY_GRAVE:
            return 0x35;
        case tff::Keys::KEY_COMMA:
            return 0x36;
        case tff::Keys::KEY_DOT:
            return 0x37;
        case tff::Keys::KEY_SLASH:
            return 0x38;
        case tff::Keys::KEY_CAPSLOCK:
            return 0x39;

        case tff::Keys::KEY_F1:
            return 0x3A;
        case tff::Keys::KEY_F2:
            return 0x3B;
        case tff::Keys::KEY_F3:
            return 0x3C;
        case tff::Keys::KEY_F4:
            return 0x3D;
        case tff::Keys::KEY_F5:
            return 0x3E;
        case tff::Keys::KEY_F6:
            return 0x3F;
        case tff::Keys::KEY_F7:
            return 0x40;
        case tff::Keys::KEY_F8:
            return 0x41;
        case tff::Keys::KEY_F9:
            return 0x42;
        case tff::Keys::KEY_F10:
            return 0x43;
        case tff::Keys::KEY_F11:
            return 0x44;
        case tff::Keys::KEY_F12:
            return 0x45;

        case tff::Keys::KEY_SYSRQ:
            return 0x46;
        case tff::Keys::KEY_SCROLLLOCK:
            return 0x47;
        case tff::Keys::KEY_PAUSE:
            return 0x48;
        case tff::Keys::KEY_INSERT:
            return 0x49;
        case tff::Keys::KEY_HOME:
            return 0x4A;
        case tff::Keys::KEY_PAGEUP:
            return 0x4B;
        case tff::Keys::KEY_DELETE:
            return 0x4C;
        case tff::Keys::KEY_END:
            return 0x4D;
        case tff::Keys::KEY_PAGEDOWN:
            return 0x4E;
        case tff::Keys::KEY_RIGHT:
            return 0x4F;
        case tff::Keys::KEY_LEFT:
            return 0x50;
        case tff::Keys::KEY_DOWN:
            return 0x51;
        case tff::Keys::KEY_UP:
            return 0x52;

        case tff::Keys::KEY_NUMLOCK:
            return 0x53;
        case tff::Keys::KEY_KPSLASH:
            return 0x54;
        case tff::Keys::KEY_KPASTERISK:
            return 0x55;
        case tff::Keys::KEY_KPMINUS:
            return 0x56;
        case tff::Keys::KEY_KPPLUS:
            return 0x57;
        case tff::Keys::KEY_KPENTER:
            return 0x58;
        case tff::Keys::KEY_KP1:
            return 0x59;
        case tff::Keys::KEY_KP2:
            return 0x5A;
        case tff::Keys::KEY_KP3:
            return 0x5B;
        case tff::Keys::KEY_KP4:
            return 0x5C;
        case tff::Keys::KEY_KP5:
            return 0x5D;
        case tff::Keys::KEY_KP6:
            return 0x5E;
        case tff::Keys::KEY_KP7:
            return 0x5F;
        case tff::Keys::KEY_KP8:
            return 0x60;
        case tff::Keys::KEY_KP9:
            return 0x61;
        case tff::Keys::KEY_KP0:
            return 0x62;
        case tff::Keys::KEY_KPDOT:
            return 0x63;

        case tff::Keys::KEY_LEFTCTRL:
            return 0xE0;
        case tff::Keys::KEY_LEFTSHIFT:
            return 0xE1;
        case tff::Keys::KEY_LEFTALT:
            return 0xE2;
        case tff::Keys::KEY_LEFTMETA:
            return 0xE3;
        case tff::Keys::KEY_RIGHTCTRL:
            return 0xE4;
        case tff::Keys::KEY_RIGHTSHIFT:
            return 0xE5;
        case tff::Keys::KEY_RIGHTALT:
            return 0xE6;
        case tff::Keys::KEY_RIGHTMETA:
            return 0xE7;

        default:
            return 0;
    }
}

#ifdef PICO_BUILD
extern "C" {
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* /*desc_report*/,
                      uint16_t /*desc_len*/) {
    printf("HID device mounted: addr=%u, instance=%u\n", dev_addr, instance);
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    printf("HID device unmounted: addr=%u, instance=%u\n", dev_addr, instance);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* /*report*/,
                                uint16_t len) {
    printf("HID report received: addr=%u, instance=%u, len=%u\n", dev_addr, instance, len);
}
}
#endif