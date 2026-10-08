#ifndef RP2040_PLATFORM_H
#define RP2040_PLATFORM_H

#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_types.h"
#include "tff_key_codes.h"
#include "debug_buffer.h"
#include <vector>
#include <memory>
#include <string>
#include <atomic>

#ifdef PICO_BUILD
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include "pio_usb.h"
#endif

/**
 * @brief RP2040 platform handler for hardware keyboard remapping
 *
 * Handles:
 * 1. USB host input (reads from physical keyboard connected to USB-A port via TinyUSB host)
 * 2. Key combinations, tap-vs-hold, text snippets, and modal layers using shared tff::TFFEngine
 * 3. USB device output (sends mapped keys to host computer via TinyUSB device)
 */
class RP2040Platform {
public:
    struct HostKeyboardReport {
        uint8_t modifiers = 0;
        uint8_t keys[6] = {0, 0, 0, 0, 0, 0};
    };

    RP2040Platform();
    ~RP2040Platform();

    RP2040Platform(const RP2040Platform&) = delete;
    RP2040Platform& operator=(const RP2040Platform&) = delete;

    /**
     * @brief Initialize the RP2040 platform with default or loaded configuration
     */
    bool initialize();

    /**
     * @brief Load configuration from a YAML string
     */
    bool loadConfiguration(const std::string& yaml_str);

    /**
     * @brief Set configuration directly
     */
    void setConfig(const tff::Config& config);

    /**
     * @brief Main processing loop
     */
    void run();

    /**
     * @brief Process USB host keyboard input
     * @param keycode USB HID Usage key code (0x07) or internal KeyCode
     * @param pressed true if pressed, false if released
     */
    void processHostKeyEvent(uint32_t keycode, bool pressed);

    /**
     * @brief Process a raw USB HID keyboard report from host port
     * @param modifiers Bitmask of modifier keys (0xE0..0xE7)
     * @param keys Array of pressed USB HID usage codes (up to key_count)
     * @param key_count Number of keys in keys array (typically 6)
     */
    void processHostKeyboardReport(uint8_t modifiers, const uint8_t* keys, size_t key_count);

    /**
     * @brief Enqueue a raw host keyboard report (thread-safe, callable from Core 1)
     */
    bool enqueueHostReport(uint8_t modifiers, const uint8_t* keys, size_t key_count);

    /**
     * @brief Dequeue a pending host report (consumed on Core 0)
     */
    bool dequeueHostReport(HostKeyboardReport& report);

    /**
     * @brief Process queued host events (called on Core 0)
     */
    void processUsbHostEvents();

    /**
     * @brief Process pending device events (called on Core 0)
     */
    void processUsbDeviceEvents();

    /**
     * @brief Check and service active timers (e.g. tap-hold or combo expiration)
     */
    void checkTimers();

    /**
     * @brief Send mapped keys via USB device (or buffer in test mode)
     */
    bool sendDeviceKeys(const std::vector<uint32_t>& key_codes);

    /**
     * @brief Send raw 8-byte standard USB HID keyboard report
     */
    bool sendRawKeyboardReport(uint8_t modifier, const uint8_t keycodes[6]);

    /**
     * @brief Retrieve received output keycodes (for test verification)
     */
    const std::vector<uint32_t>& getEmittedKeys() const;

    /**
     * @brief Clear buffered output keys
     */
    void clearEmittedKeys();

    /**
     * @brief Access the underlying shared TFF engine
     */
    tff::TFFEngine& getEngine();

    /**
     * @brief Access the active config
     */
    const tff::Config& getConfig() const { return config_; }

    /**
     * @brief Cleanup platform resources
     */
    void cleanup();

    /**
     * @brief Set simulated timestamp for testing (ms since boot)
     */
    void setTimestamp(uint32_t ms) { test_timestamp_ms_ = ms; }

    /**
     * @brief Convert USB HID usage code (0x07) to internal Linux KeyCode
     */
    static tff::KeyCode convertUsbToKeyCode(uint8_t usb_keycode);

    /**
     * @brief Convert internal Linux KeyCode to USB HID usage code (0x07)
     */
    static uint8_t convertKeyCodeToUsb(tff::KeyCode internal_keycode);

    // Backward-compatible static helpers
    static uint32_t convertKeyCode(uint8_t usb_keycode) {
        return static_cast<uint32_t>(convertUsbToKeyCode(usb_keycode));
    }
    static uint8_t convertToUsbKeyCode(uint32_t internal_keycode) {
        return convertKeyCodeToUsb(static_cast<tff::KeyCode>(internal_keycode));
    }

    /**
     * @brief Trigger a diagnostic dump of recent inputs/outputs
     * @param type_to_hid If true, also type out the dump as virtual keystrokes
     */
    void triggerDebugDump(bool type_to_hid = true);

    /**
     * @brief Retrieve the most recent formatted diagnostic dump string
     */
    const std::string& getLastDebugDump() const { return last_debug_dump_; }

    /**
     * @brief Access the event ring buffer
     */
    const tff::DebugBuffer& getDebugBuffer() const { return debug_buffer_; }
    tff::DebugBuffer& getDebugBuffer() { return debug_buffer_; }

    /**
     * @brief Type out arbitrary text as USB HID keystrokes
     */
    void typeDumpString(const std::string& text);

private:
    class RP2040EventWriter;

    std::unique_ptr<RP2040EventWriter> writer_;
    std::unique_ptr<tff::TFFEngine> engine_;
    tff::Config config_;

    bool initialized_;
    bool running_;
    uint32_t test_timestamp_ms_;

    // Diagnostic ring buffer and state
    tff::DebugBuffer debug_buffer_;
    std::string last_debug_dump_;
    bool is_dumping_ = false;
    bool debug_chord_latched_ = false;

    // Host report queue for cross-core lock-free passing (Core 1 -> Core 0)
    static constexpr size_t REPORT_QUEUE_SIZE = 32;
    HostKeyboardReport report_queue_[REPORT_QUEUE_SIZE];
    std::atomic<size_t> queue_head_{0};
    std::atomic<size_t> queue_tail_{0};

    // State tracking for report transitions
    uint8_t prev_modifiers_ = 0;
    uint8_t prev_keys_[6] = {0, 0, 0, 0, 0, 0};
    size_t prev_key_count_ = 0;

    uint32_t getCurrentTimestamp();

    bool initUsbHost();
    bool initUsbDevice();
};

#ifdef PICO_BUILD
// USB host callback functions (extern "C")
extern "C" {
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report,
                      uint16_t desc_len);
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report,
                                uint16_t len);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
void tff_rp2040_cdc_dump(void);
#ifdef __cplusplus
}
#endif

#endif  // RP2040_PLATFORM_H