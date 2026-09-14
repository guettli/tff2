#ifndef RP2040_PLATFORM_H
#define RP2040_PLATFORM_H

#include "key_events.h"
#include "tff_app.h"
#include <vector>
#include <memory>

#ifdef PICO_BUILD
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "tusb.h"
#include <ArduinoJson.h>
#endif

/**
 * @brief RP2040 platform handler for actual hardware implementation
 *
 * This class handles the complete RP2040 firmware implementation:
 * 1. USB host input (reads from keyboard connected to USB-A port)
 * 2. Key combination processing using TFF core logic
 * 3. USB device output (sends mapped keys to computer via USB-C port)
 */
class RP2040Platform {
public:
    /**
     * @brief Constructor
     */
    RP2040Platform();

    /**
     * @brief Destructor
     */
    ~RP2040Platform();

    /**
     * @brief Initialize the RP2040 platform
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Main processing loop
     * Continuously reads input, processes combinations, and sends output
     */
    void run();

    /**
     * @brief Process USB host keyboard input
     * @param keycode The key code received from USB host
     * @param pressed true if key pressed, false if released
     */
    void processHostKeyEvent(uint32_t keycode, bool pressed);

    /**
     * @brief Send mapped keys via USB device
     * @param key_codes Vector of key codes to send
     * @return true if successful
     */
    bool sendDeviceKeys(const std::vector<uint32_t>& key_codes);

    /**
     * @brief Cleanup platform resources
     */
    void cleanup();

    /**
     * @brief Convert USB key code to internal key code
     * @param usb_keycode USB key code
     * @return internal key code
     */
    static uint32_t convertKeyCode(uint8_t usb_keycode);

    /**
     * @brief Convert internal key code to USB key code
     * @param internal_keycode internal key code
     * @return USB key code
     */
    static uint8_t convertToUsbKeyCode(uint32_t internal_keycode);

private:
    // Core TFF application
    std::unique_ptr<TFFApp> tff_app_;

    // Platform state
    bool initialized_;
    bool running_;

    // USB state tracking
    bool keys_pressed_[256]; // Track currently pressed keys

    /**
     * @brief Initialize USB host functionality
     * @return true if successful
     */
    bool initUsbHost();

    /**
     * @brief Initialize USB device functionality
     * @return true if successful
     */
    bool initUsbDevice();

    /**
     * @brief Process pending USB host events
     */
    void processUsbHostEvents();

    /**
     * @brief Process pending USB device events
     */
    void processUsbDeviceEvents();

    /**
     * @brief Get current timestamp in milliseconds
     * @return timestamp in milliseconds
     */
    uint32_t getCurrentTimestamp();

#ifdef PICO_BUILD
    /**
     * @brief Load TFF configuration from JSON file
     * @param config_path Path to JSON configuration file
     * @return true if successful
     */
    bool loadTffConfiguration(const char* config_path);
#endif
};

// USB host callback functions (extern "C")
extern "C" {
    void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len);
    void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);
    void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
}

#endif // RP2040_PLATFORM_H