#ifndef RP2040_PLATFORM_H
#define RP2040_PLATFORM_H

#include "tff_engine.h"
#include "tff_parser.h"
#include "tff_types.h"
#include "tff_key_codes.h"
#include <vector>
#include <memory>
#include <string>

#ifdef PICO_BUILD
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"
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
     * @brief Check and service active timers (e.g. tap-hold or combo expiration)
     */
    void checkTimers();

    /**
     * @brief Send mapped keys via USB device (or buffer in test mode)
     */
    bool sendDeviceKeys(const std::vector<uint32_t>& key_codes);

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

private:
    class RP2040EventWriter;

    std::unique_ptr<RP2040EventWriter> writer_;
    std::unique_ptr<tff::TFFEngine> engine_;
    tff::Config config_;

    bool initialized_;
    bool running_;
    uint32_t test_timestamp_ms_;

    uint32_t getCurrentTimestamp();

    bool initUsbHost();
    bool initUsbDevice();
    void processUsbHostEvents();
    void processUsbDeviceEvents();
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

#endif  // RP2040_PLATFORM_H