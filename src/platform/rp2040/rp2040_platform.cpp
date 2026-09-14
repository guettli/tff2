#include "rp2040_platform.h"
#include <cstring>
#include <cstdio>

RP2040Platform::RP2040Platform()
    : tff_app_(nullptr), initialized_(false), running_(false) {
    // Initialize key state tracking
    memset(keys_pressed_, 0, sizeof(keys_pressed_));
}

RP2040Platform::~RP2040Platform() {
    cleanup();
}

bool RP2040Platform::initialize() {
    if (initialized_) {
        return true;
    }

#ifdef PICO_BUILD
    // Initialize chosen serial port
    stdio_init_all();

    // Initialize core TFF application
    tff_app_ = std::make_unique<TFFApp>(100); // 100ms overlap threshold

    // Load TFF configuration from JSON
    if (!loadTffConfiguration("/config/tff-combos.json")) {
        printf("Failed to load TFF configuration, using defaults\n");
        // Add default mappings
        tff_app_->getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});      // F+J -> 1
        tff_app_->getKeyMapper().addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});      // J+F -> 2
        tff_app_->getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE}); // F+Space -> 3
    }

    // Initialize USB host and device
    if (!initUsbHost()) {
        printf("Failed to initialize USB host\n");
        return false;
    }

    if (!initUsbDevice()) {
        printf("Failed to initialize USB device\n");
        return false;
    }

    printf("RP2040Platform initialized successfully\n");
    initialized_ = true;
    return true;
#else
    // For testing without Pico SDK
    tff_app_ = std::make_unique<TFFApp>(100);

    // Add default mappings
    tff_app_->getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::ONE});
    tff_app_->getKeyMapper().addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::TWO});
    tff_app_->getKeyMapper().addMapping(KeyCodes::F_KEY, KeyCodes::SPACE_KEY, {KeyCodes::THREE});

    printf("RP2040Platform initialized for testing\n");
    initialized_ = true;
    return true;
#endif
}

void RP2040Platform::run() {
    if (!initialized_) {
        printf("RP2040Platform not initialized\n");
        return;
    }

    running_ = true;
    printf("RP2040 TFF-like keyboard remapping started\n");
    printf("Listening for keyboard input...\n");

#ifdef PICO_BUILD
    while (running_) {
        // Process USB host events
        processUsbHostEvents();

        // Process USB device events
        processUsbDeviceEvents();

        // TinyUSB housekeeping
        tud_task();
        tuh_task();

        // Small delay to prevent busy looping
        // sleep_ms(1); // Placeholder - actual RP2040 implementation would use proper delay
    }
#else
    // For testing, just run a simple loop
    printf("Running in test mode - press Ctrl+C to exit\n");
    while (running_) {
        // In a real implementation, we would process actual events
        // For now, we'll just sleep
        // sleep_ms(100); // Placeholder - actual RP2040 implementation would use proper delay
    }
#endif
}

void RP2040Platform::processHostKeyEvent(uint32_t keycode, bool pressed) {
    if (!initialized_) {
        return;
    }

#ifdef PICO_BUILD
    uint32_t timestamp = getCurrentTimestamp();

    // Track key state
    if (keycode < 256) {
        keys_pressed_[keycode] = pressed;
    }

    // Process the key event through our TFF application
    std::vector<uint32_t> output_keys = tff_app_->processKeyEvent(keycode, timestamp, pressed);

    // If we got output keys, send them via USB device
    if (!output_keys.empty()) {
        printf("Sending mapped keys: ");
        for (auto key : output_keys) {
            printf("%u ", key);
        }
        printf("\n");

        sendDeviceKeys(output_keys);
    }
#else
    // For testing
    printf("Processing host key event: keycode=%u, pressed=%s\n",
           (unsigned int)keycode, pressed ? "true" : "false");
#endif
}

bool RP2040Platform::sendDeviceKeys(const std::vector<uint32_t>& key_codes) {
    if (!initialized_ || key_codes.empty()) {
        return false;
    }

#ifdef PICO_BUILD
    // Convert internal key codes to USB key codes and send
    for (uint32_t key_code : key_codes) {
        uint8_t usb_keycode = convertToUsbKeyCode(key_code);
        if (usb_keycode != 0) {
            // Send key press
            tud_hid_keyboard_report(0, 0, &usb_keycode);
            sleep_ms(10); // Brief delay

            // Send key release
            tud_hid_keyboard_report(0, 0, nullptr);
        }
    }
    return true;
#else
    // For testing
    printf("Sending device keys: ");
    for (auto key : key_codes) {
        printf("%u ", (unsigned int)key);
    }
    printf("\n");
    return true;
#endif
}

void RP2040Platform::cleanup() {
    running_ = false;
    initialized_ = false;

    if (tff_app_) {
        tff_app_.reset();
    }
}

bool RP2040Platform::initUsbHost() {
#ifdef PICO_BUILD
    // USB host initialization would go here
    // This would involve setting up the USB host port and callbacks
    printf("USB host initialization (placeholder)\n");
    return true;
#else
    printf("USB host initialization (testing mode)\n");
    return true;
#endif
}

bool RP2040Platform::initUsbDevice() {
#ifdef PICO_BUILD
    // USB device initialization would go here
    // This would involve setting up the USB device functionality
    printf("USB device initialization (placeholder)\n");
    return true;
#else
    printf("USB device initialization (testing mode)\n");
    return true;
#endif
}

void RP2040Platform::processUsbHostEvents() {
    // Process pending USB host events
    // In a real implementation, this would check for new keyboard reports
#ifdef PICO_BUILD
    // Actual implementation would process incoming HID reports
#endif
}

void RP2040Platform::processUsbDeviceEvents() {
    // Process pending USB device events
    // In a real implementation, this would handle device state changes
#ifdef PICO_BUILD
    // Actual implementation would handle USB device events
#endif
}

uint32_t RP2040Platform::getCurrentTimestamp() {
#ifdef PICO_BUILD
    return to_ms_since_boot(get_absolute_time());
#else
    // For testing, use a simple counter or system clock
    static uint32_t test_counter = 0;
    return test_counter++;
#endif
}

uint32_t RP2040Platform::convertKeyCode(uint8_t usb_keycode) {
    // Convert USB key codes to our internal key codes
    // This is a simplified mapping for common keys

    switch (usb_keycode) {
        case 0x09: return KeyCodes::F_KEY;      // F key
        case 0x0A: return KeyCodes::J_KEY;      // J key
        case 0x2C: return KeyCodes::SPACE_KEY;  // Space key
        case 0x1E: return KeyCodes::ONE;        // 1 key
        case 0x1F: return KeyCodes::TWO;        // 2 key
        case 0x20: return KeyCodes::THREE;      // 3 key
        case 0x21: return KeyCodes::FOUR;       // 4 key
        default: return usb_keycode;            // Pass through other codes
    }
}

uint8_t RP2040Platform::convertToUsbKeyCode(uint32_t internal_keycode) {
    // Convert our internal key codes to USB key codes

    switch (internal_keycode) {
        case KeyCodes::F_KEY: return 0x09;      // F key
        case KeyCodes::J_KEY: return 0x0A;      // J key
        case KeyCodes::SPACE_KEY: return 0x2C;  // Space key
        case KeyCodes::ONE: return 0x1E;        // 1 key
        case KeyCodes::TWO: return 0x1F;        // 2 key
        case KeyCodes::THREE: return 0x20;     // 3 key
        case KeyCodes::FOUR: return 0x21;       // 4 key
        case KeyCodes::LEFT_ARROW: return 0x50;  // Left arrow
        case KeyCodes::RIGHT_ARROW: return 0x4F; // Right arrow
        case KeyCodes::UP_ARROW: return 0x52;   // Up arrow
        case KeyCodes::DOWN_ARROW: return 0x51;  // Down arrow
        case KeyCodes::ENTER: return 0x28;      // Enter key
        case KeyCodes::BACKSPACE: return 0x2A;  // Backspace key
        case KeyCodes::DELETE: return 0x4C;     // Delete key
        case KeyCodes::ESCAPE: return 0x29;     // Escape key
        case KeyCodes::HOME: return 0x4A;        // Home key
        case KeyCodes::END: return 0x4D;        // End key
        case KeyCodes::PAGE_UP: return 0x4B;     // Page up key
        case KeyCodes::PAGE_DOWN: return 0x4E;  // Page down key
        case KeyCodes::SEMICOLON: return 0x33;   // Semicolon key
        case KeyCodes::A: return 0x04;          // A key
        case KeyCodes::N: return 0x11;           // N key
        case KeyCodes::U: return 0x18;           // U key
        case KeyCodes::M: return 0x10;           // M key
        case KeyCodes::K: return 0x0E;           // K key
        case KeyCodes::L: return 0x0F;           // L key
        case KeyCodes::I: return 0x0C;           // I key
        case KeyCodes::COMMA: return 0x36;      // Comma key
        case KeyCodes::G: return 0x0A;          // G key
        case KeyCodes::H: return 0x0B;           // H key
        case KeyCodes::D: return 0x07;          // D key
        default:
            // If it's already a USB key code, pass through
            if (internal_keycode <= 0xFF) {
                return static_cast<uint8_t>(internal_keycode);
            }
            return 0;  // Invalid key code
    }
}

#ifdef PICO_BUILD
bool RP2040Platform::loadTffConfiguration(const char* config_path) {
    // In a real implementation, this would read the JSON file from flash storage
    // and parse it using ArduinoJson

    printf("Loading TFF configuration from: %s\n", config_path);

    // For now, we'll simulate loading the configuration by adding the TFF mappings
    // In a real implementation, this would parse the actual JSON file

    if (!tff_app_) {
        printf("TFF application not initialized\n");
        return false;
    }

    KeyMapper& mapper = tff_app_->getKeyMapper();
    mapper.clearMappings();

    // j f -> backspace
    mapper.addMapping(KeyCodes::J_KEY, KeyCodes::F_KEY, {KeyCodes::BACKSPACE});

    // f j -> delete
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::J_KEY, {KeyCodes::DELETE});

    // semicolon a -> home
    mapper.addMapping(KeyCodes::SEMICOLON, KeyCodes::A, {KeyCodes::HOME});

    // a semicolon -> end
    mapper.addMapping(KeyCodes::A, KeyCodes::SEMICOLON, {KeyCodes::END});

    // f n -> down
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::N, {KeyCodes::DOWN_ARROW});

    // f u -> up
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::U, {KeyCodes::UP_ARROW});

    // f m -> down
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::M, {KeyCodes::DOWN_ARROW});

    // f k -> left
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::K, {KeyCodes::LEFT_ARROW});

    // f l -> right
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::L, {KeyCodes::RIGHT_ARROW});

    // f i -> pageup
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::I, {KeyCodes::PAGE_UP});

    // f comma -> pagedown
    mapper.addMapping(KeyCodes::F_KEY, KeyCodes::COMMA, {KeyCodes::PAGE_DOWN});

    // g h -> esc
    mapper.addMapping(KeyCodes::G, KeyCodes::H, {KeyCodes::ESCAPE});

    printf("Loaded TFF configuration with %zu mappings\n", mapper.getMappingCount());
    return true;
}
#endif

// USB host callback implementations
#ifdef PICO_BUILD
extern "C" {
    void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* /*desc_report*/, uint16_t /*desc_len*/) {
        // Called when a HID device is mounted
        printf("HID device mounted: addr=%u, instance=%u\n", dev_addr, instance);
    }

    void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
        // Called when a HID device is unmounted
        printf("HID device unmounted: addr=%u, instance=%u\n", dev_addr, instance);
    }

    void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* /*report*/, uint16_t len) {
        // Called when a HID report is received
        // This would parse keyboard reports and call processHostKeyEvent
        printf("HID report received: addr=%u, instance=%u, len=%u\n", dev_addr, instance, len);
    }
}
#endif