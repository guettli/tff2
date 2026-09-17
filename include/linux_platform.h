#ifndef LINUX_PLATFORM_H
#define LINUX_PLATFORM_H

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
     * @brief Reload configuration from the currently loaded config file
     * @return true if successfully reloaded, false if parse error (existing config retained)
     */
    bool reloadConfiguration();

    /**
     * @brief Reload configuration from a specified YAML file
     * @return true if successfully reloaded, false if parse error (existing config retained)
     */
    bool reloadConfiguration(const std::string& config_file);

    /**
     * @brief Get the path to the currently active configuration file
     */
    const std::string& getConfigFilePath() const { return config_file_; }

    /**
     * @brief Enable or disable configuration file change watching via inotify
     * @param enable Whether to watch the configuration file
     * @param config_path Optional path to configuration file (uses loaded path if empty)
     */
    bool enableConfigWatch(bool enable = true, const std::string& config_path = "");

    /**
     * @brief Check whether configuration file watching is enabled
     */
    bool isConfigWatchEnabled() const { return config_watch_enabled_; }

    /**
     * @brief Set combos directly
     */
    void setCombos(const std::vector<tff::Combo>& combos);

    /**
     * @brief Set tap-hold keys directly
     */
    void setTapHoldKeys(const std::vector<tff::TapHoldKey>& keys);

    /**
     * @brief Set one-shot keys directly
     */
    void setOneShotKeys(const std::vector<tff::OneShotKey>& keys);

    /**
     * @brief Set modal layers directly
     */
    void setLayers(const std::vector<tff::Layer>& layers);

    /**
     * @brief Set sequential leader config directly
     */
    void setLeaderConfig(const tff::LeaderConfig& config);

    /**
     * @brief Set auto-shift config directly
     */
    void setAutoShiftConfig(const tff::AutoShiftConfig& config);

    /**
     * @brief Set full configuration (combos, tap-hold keys, layers, one-shot keys, leader, auto-shift)
     */
    void setConfig(const tff::Config& config);

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
     * @param should_stop Atomic flag to stop the event loop
     * @param should_reload Optional atomic flag (e.g. from SIGHUP) to trigger config reload
     */
    void run(std::atomic<bool>& should_stop, std::atomic<bool>* should_reload = nullptr);

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
     * @brief Get count of currently active combo mappings
     */
    size_t getComboCount() const { return engine_ ? engine_->getCombos().size() : 0; }

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
    bool config_watch_enabled_;
    int inotify_fd_;
    int hotplug_wd_;
    int config_wd_;
    std::string config_file_;
    std::string config_dir_;
    std::string config_basename_;

    std::unique_ptr<tff::EventWriter> writer_;
    std::unique_ptr<tff::TFFEngine> engine_;

    std::vector<uint32_t> received_keys_;

    int createVirtualKeyboard(const std::string& device_name = "TFF Virtual Keyboard");
    void setupInotify();
    void teardownInotify();
    void setupConfigWatch();
    void teardownConfigWatch();
    void processInotifyEvents();
};

#endif // LINUX_PLATFORM_H