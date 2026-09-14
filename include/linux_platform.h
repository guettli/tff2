#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

#include "key_events.h"
#include "tff_engine.h"
#include "tff_parser.h"
#include <vector>
#include <string>
#include <memory>
#include <atomic>

/**
 * @brief Full Linux platform implementation for Ten Flying Fingers (TFF)
 *
 * Provides real-time Linux keyboard remapping using:
 * - evdev (/dev/input/event*) for reading hardware keyboard events
 * - uinput (/dev/uinput) for emitting remapped virtual keyboard events
 * - tff::TFFEngine for shared core combo matching and state machine logic
 */
class LinuxPlatform {
public:
    LinuxPlatform();
    ~LinuxPlatform();

    LinuxPlatform(const LinuxPlatform&) = delete;
    LinuxPlatform& operator=(const LinuxPlatform&) = delete;

    /**
     * @brief Configure whether input keyboard devices should be exclusively grabbed
     */
    void setGrab(bool grab) { grabbed_ = grab; }

    /**
     * @brief Initialize platform (creates virtual uinput keyboard)
     */
    bool initialize();

    /**
     * @brief Load combo configuration from YAML file
     */
    bool loadConfiguration(const std::string& config_file);

    /**
     * @brief Set combos directly
     */
    void setCombos(const std::vector<tff::Combo>& combos);

    /**
     * @brief Set verbose debug logging
     */
    void setVerbose(bool verbose);
    bool isVerbose() const { return verbose_; }

    /**
     * @brief Automatically discover keyboard event devices on the system
     */
    static std::vector<std::string> discoverKeyboards();

    /**
     * @brief Check if a given evdev path is a hardware keyboard
     * @param dev_path Device node path (e.g. /dev/input/event3)
     * @param out_name Optional pointer to store human-readable device name
     * @return true if device is a valid alphanumeric keyboard
     */
    static bool isKeyboardDevice(const std::string& dev_path, std::string* out_name = nullptr);

    /**
     * @brief Get human-readable device name from evdev path
     */
    static std::string getDeviceName(const std::string& dev_path);

    /**
     * @brief Find stable persistent symlink (by-id or by-path) for device
     */
    static std::string getDeviceAlias(const std::string& dev_path);

    /**
     * @brief Open a specific input keyboard device
     * @param device_path Path like /dev/input/event8
     * @param grab Whether to exclusively grab the device (ioctl EVIOCGRAB)
     */
    bool openInputDevice(const std::string& device_path, bool grab = false);

    /**
     * @brief Open multiple keyboard devices
     */
    bool openInputDevices(const std::vector<std::string>& device_paths, bool grab = false);

    /**
     * @brief Attach an additional keyboard device dynamically without closing existing ones
     */
    bool attachInputDevice(const std::string& device_path, bool grab = false);

    /**
     * @brief Detach and close a monitored keyboard device
     */
    bool detachInputDevice(int fd);

    /**
     * @brief Check if a device path is currently attached
     */
    bool isDeviceAttached(const std::string& device_path) const;

    /**
     * @brief Get count of currently attached input devices
     */
    size_t getAttachedDeviceCount() const;

    /**
     * @brief Enable or disable dynamic keyboard hotplug monitoring via inotify
     */
    bool enableHotplug(bool enable = true);

    /**
     * @brief Check whether dynamic hotplug monitoring is enabled
     */
    bool isHotplugEnabled() const { return hotplug_enabled_; }

    /**
     * @brief Run the real-time event loop until should_stop is set
     */
    void run(std::atomic<bool>& should_stop);

    /**
     * @brief Process a single event through the platform
     */
    bool processEvent(const tff::Event& ev);

    /**
     * @brief Send a key event (supports both simulated testing and real hardware)
     */
    bool sendKeyEvent(uint32_t key_code, bool is_pressed);

    /**
     * @brief Receive mapped key events (for test compatibility)
     */
    bool receiveMappedKeys(std::vector<uint32_t>& key_codes);

    /**
     * @brief Access the shared core TFF engine
     */
    tff::TFFEngine& getEngine();

    /**
     * @brief Cleanup platform resources (closes uinput, inotify, and evdev, releases grab)
     */
    void cleanup();

    bool isInitialized() const { return initialized_; }
    bool isGrabbed() const { return grabbed_; }
    int getInputFd() const { return evdev_fd_; }
    int getOutputFd() const { return uinput_fd_; }

private:
    struct DeviceInfo {
        int fd;
        std::string path;
        std::string name;
    };

    int uinput_fd_;
    int evdev_fd_;
    std::vector<DeviceInfo> devices_;
    bool grabbed_;
    bool initialized_;
    bool verbose_;
    bool hotplug_enabled_;
    int inotify_fd_;
    int inotify_wd_;

    std::unique_ptr<tff::EventWriter> writer_;
    std::unique_ptr<tff::TFFEngine> engine_;

    std::vector<uint32_t> received_keys_;

    int createVirtualKeyboard(const std::string& device_name = "TFF Virtual Keyboard");
    void setupInotify();
    void teardownInotify();
    void processHotplugEvents();
};

#endif // LINUX_PLATFORM_H