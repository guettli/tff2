#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

#include "key_events.h"
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Linux platform handler for testing RP2040 firmware
 *
 * This class simulates the hardware setup where:
 * 1. UpBoard sends fake keyboard events to RP2040 USB-A host port (simulating physical keyboard)
 * 2. RP2040 processes key combinations and sends mapped output to UpBoard USB-C device port
 */
class LinuxPlatform {
public:
    /**
     * @brief Constructor
     */
    LinuxPlatform();

    /**
     * @brief Destructor
     */
    ~LinuxPlatform();

    /**
     * @brief Initialize the platform for testing
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Send a key event to simulate keyboard input
     * @param key_code The key code to send
     * @param is_pressed true if key press, false if key release
     * @return true if successful
     */
    bool sendKeyEvent(uint32_t key_code, bool is_pressed);

    /**
     * @brief Receive mapped key events from RP2040
     * @param key_codes Vector to store received key codes
     * @return true if successful
     */
    bool receiveMappedKeys(std::vector<uint32_t>& key_codes);

    /**
     * @brief Cleanup platform resources
     */
    void cleanup();

private:
    // File descriptors for uinput and evdev simulation
    int uinput_fd_;
    int evdev_fd_;

    // Internal state
    bool initialized_;

    // Simulated received keys
    std::vector<uint32_t> received_keys_;

    /**
     * @brief Create a virtual input device for sending events
     * @return file descriptor, or -1 on error
     */
    int createVirtualKeyboard();

    /**
     * @brief Create a virtual output device for receiving events
     * @return file descriptor, or -1 on error
     */
    int createVirtualOutputDevice();

    /**
     * @brief Emit an input event
     * @param fd File descriptor
     * @param type Event type (EV_KEY, etc.)
     * @param code Event code (key code, etc.)
     * @param value Event value (1 for press, 0 for release)
     * @return true if successful
     */
    bool emitEvent(int fd, uint16_t type, uint16_t code, int32_t value);
};

#endif // LINUX_PLATFORM_H