#include "linux_platform.h"
#include "tff_key_codes.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <poll.h>
#include <dirent.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <sys/inotify.h>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <climits>

namespace {

#define TEST_BIT_EV(bit, arr) (((arr)[(bit) / (sizeof(unsigned long) * 8)] & (1UL << ((bit) % (sizeof(unsigned long) * 8)))) != 0)

class UInputWriter : public tff::EventWriter {
public:
    UInputWriter(int uinput_fd, std::vector<uint32_t>* received_keys, bool verbose = false)
        : uinput_fd_(uinput_fd), received_keys_(received_keys), verbose_(verbose) {}

    void setUinputFd(int fd) { uinput_fd_ = fd; }
    void setVerbose(bool verbose) { verbose_ = verbose; }

    void writeOne(const tff::Event& ev) override {
        if (verbose_ && ev.type == EV_KEY) {
            std::cout << "[TFF Out] " << tff::keyCodeToWord(ev.code)
                      << " (" << ev.code << ") "
                      << (ev.value == tff::KEY_VAL_DOWN ? "DOWN" : "UP") << "\n";
        }
        if (ev.type == EV_KEY && ev.value == tff::KEY_VAL_DOWN && received_keys_) {
            received_keys_->push_back(ev.code);
        }
        if (uinput_fd_ >= 0) {
            struct input_event ie;
            std::memset(&ie, 0, sizeof(ie));
            ie.time.tv_sec = ev.time.sec;
            ie.time.tv_usec = ev.time.usec;
            ie.type = ev.type;
            ie.code = ev.code;
            ie.value = ev.value;
            ssize_t bytes = write(uinput_fd_, &ie, sizeof(ie));
            (void)bytes;
        }
    }

private:
    int uinput_fd_;
    std::vector<uint32_t>* received_keys_;
    bool verbose_;
};

} // anonymous namespace

LinuxPlatform::LinuxPlatform()
    : uinput_fd_(-1),
      evdev_fd_(-1),
      grabbed_(false),
      initialized_(false),
      verbose_(false),
      hotplug_enabled_(false),
      config_watch_enabled_(false),
      inotify_fd_(-1),
      hotplug_wd_(-1),
      config_wd_(-1) {
}

LinuxPlatform::~LinuxPlatform() {
    cleanup();
}

bool LinuxPlatform::initialize() {
    if (initialized_) {
        return true;
    }

    uinput_fd_ = createVirtualKeyboard("TFF Virtual Keyboard");
    if (uinput_fd_ < 0) {
        // May happen in non-root test environments without /dev/uinput access;
        // fall back gracefully to simulation mode
    }

    writer_ = std::make_unique<UInputWriter>(uinput_fd_, &received_keys_, verbose_);
    engine_ = std::make_unique<tff::TFFEngine>(writer_.get());

    initialized_ = true;
    return true;
}

void LinuxPlatform::setVerbose(bool verbose) {
    verbose_ = verbose;
    if (writer_) {
        static_cast<UInputWriter*>(writer_.get())->setVerbose(verbose);
    }
}

tff::TFFEngine& LinuxPlatform::getEngine() {
    if (!initialized_) {
        initialize();
    }
    return *engine_;
}

void LinuxPlatform::setCombos(const std::vector<tff::Combo>& combos) {
    if (!initialized_) {
        initialize();
    }
    if (engine_) {
        engine_->setCombos(combos);
    }
}

void LinuxPlatform::setTapHoldKeys(const std::vector<tff::TapHoldKey>& keys) {
    if (!initialized_) {
        initialize();
    }
    if (engine_) {
        engine_->setTapHoldKeys(keys);
    }
}

void LinuxPlatform::setConfig(const tff::Config& config) {
    setCombos(config.combos);
    setTapHoldKeys(config.tap_hold_keys);
}

bool LinuxPlatform::loadConfiguration(const std::string& config_file) {
    if (!initialized_) {
        initialize();
    }

    std::string resolved = config_file;
    std::ifstream file(resolved);
    if (!file.is_open() && resolved.rfind("../", 0) != 0) {
        file.open("../" + resolved);
        if (file.is_open()) {
            resolved = "../" + resolved;
        }
    }
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << config_file << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string err_msg;
    tff::Config config;
    if (!tff::loadYamlConfig(buffer.str(), config, err_msg)) {
        std::cerr << "Failed to parse YAML config (" << resolved << "): " << err_msg << "\n";
        return false;
    }

    config_file_ = resolved;
    setConfig(config);
    return true;
}

bool LinuxPlatform::reloadConfiguration() {
    if (config_file_.empty()) {
        std::cerr << "Error: cannot reload configuration, no config file specified\n";
        return false;
    }
    return reloadConfiguration(config_file_);
}

bool LinuxPlatform::reloadConfiguration(const std::string& config_file) {
    std::string resolved = config_file;
    std::ifstream file(resolved);
    if (!file.is_open() && resolved.rfind("../", 0) != 0) {
        file.open("../" + resolved);
        if (file.is_open()) {
            resolved = "../" + resolved;
        }
    }
    if (!file.is_open()) {
        std::cerr << "Error reloading config: failed to open " << config_file << "\n";
        std::cerr << "Keeping current configuration ("
                  << (engine_ ? engine_->getCombos().size() : 0) << " combo(s) active)\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string err_msg;
    tff::Config config;
    if (!tff::loadYamlConfig(buffer.str(), config, err_msg)) {
        std::cerr << "Error reloading config (" << resolved << "): " << err_msg << "\n";
        std::cerr << "Keeping current configuration ("
                  << (engine_ ? engine_->getCombos().size() : 0) << " combo(s) active)\n";
        return false;
    }

    config_file_ = resolved;
    setConfig(config);
    std::cout << "[TFF Config] Successfully reloaded configuration from " << resolved
              << " (" << config.combos.size() << " combo(s), "
              << config.tap_hold_keys.size() << " tap-hold key(s) active)\n";
    return true;
}

bool LinuxPlatform::isKeyboardDevice(const std::string& dev_path, std::string* out_name) {
    int fd = open(dev_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    char dev_name[256] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(dev_name) - 1), dev_name);
    std::string name_str(dev_name);

    // Skip virtual TFF devices to avoid self-monitoring loops
    if (name_str.find("TFF Virtual Keyboard") != std::string::npos ||
        name_str.find("tff-clone") != std::string::npos) {
        close(fd);
        return false;
    }

    unsigned long ev_bits[(EV_MAX + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0 || !TEST_BIT_EV(EV_KEY, ev_bits)) {
        close(fd);
        return false;
    }

    unsigned long key_bits[(KEY_MAX + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        close(fd);
        return false;
    }

    // Must support typical alphanumeric keyboard keys
    bool is_kbd = TEST_BIT_EV(KEY_A, key_bits) &&
                  TEST_BIT_EV(KEY_SPACE, key_bits) &&
                  TEST_BIT_EV(KEY_ENTER, key_bits);
    if (is_kbd && out_name) {
        *out_name = name_str;
    }

    close(fd);
    return is_kbd;
}

std::vector<std::string> LinuxPlatform::discoverKeyboards() {
    std::vector<std::string> keyboards;
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        return keyboards;
    }

    std::vector<std::pair<int, std::string>> candidates;
    const struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name(ent->d_name);
        if (name.rfind("event", 0) == 0) {
            int idx = -1;
            try {
                idx = std::stoi(name.substr(5));
            } catch (...) {
                idx = 9999;
            }
            candidates.push_back({idx, "/dev/input/" + name});
        }
    }
    closedir(dir);

    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& item : candidates) {
        const std::string& path = item.second;
        if (isKeyboardDevice(path)) {
            keyboards.push_back(path);
        }
    }

    return keyboards;
}

std::string LinuxPlatform::getDeviceName(const std::string& dev_path) {
    int fd = open(dev_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return "";
    char name[256] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
    close(fd);
    return std::string(name);
}

std::string LinuxPlatform::getDeviceAlias(const std::string& dev_path) {
    char real_dev[PATH_MAX];
    if (!realpath(dev_path.c_str(), real_dev)) {
        return "";
    }
    std::string real_str(real_dev);

    const std::vector<std::string> base_dirs = {"/dev/input/by-id", "/dev/input/by-path"};
    for (const auto& base_dir : base_dirs) {
        DIR* dir = opendir(base_dir.c_str());
        if (!dir) continue;
        const struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_name[0] == '.') continue;
            std::string full_path = base_dir + "/" + ent->d_name;
            char real_entry[PATH_MAX];
            if (realpath(full_path.c_str(), real_entry)) {
                if (real_str == real_entry) {
                    closedir(dir);
                    return full_path;
                }
            }
        }
        closedir(dir);
    }
    return "";
}

bool LinuxPlatform::openInputDevice(const std::string& device_path, bool grab) {
    return openInputDevices({device_path}, grab);
}

bool LinuxPlatform::isDeviceAttached(const std::string& device_path) const {
    char real_dev[PATH_MAX];
    std::string canonical = device_path;
    if (realpath(device_path.c_str(), real_dev)) {
        canonical = real_dev;
    }

    for (const auto& dev : devices_) {
        char existing_real[PATH_MAX];
        std::string existing_canonical = dev.path;
        if (realpath(dev.path.c_str(), existing_real)) {
            existing_canonical = existing_real;
        }
        if (canonical == existing_canonical) {
            return true;
        }
    }
    return false;
}

size_t LinuxPlatform::getAttachedDeviceCount() const {
    return devices_.size();
}

bool LinuxPlatform::attachInputDevice(const std::string& device_path, bool grab) {
    if (isDeviceAttached(device_path)) {
        return true;
    }

    int fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (verbose_) {
            std::cerr << "Failed to open input device " << device_path << ": " << strerror(errno) << "\n";
        }
        return false;
    }

    if (grab) {
        if (ioctl(fd, EVIOCGRAB, 1) < 0) {
            std::cerr << "Warning: Failed to grab device " << device_path << ": " << strerror(errno) << "\n";
        }
    }

    char name[256] = {0};
    ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
    std::string name_str(name);

    devices_.push_back({fd, device_path, name_str});
    evdev_fd_ = devices_[0].fd;
    grabbed_ = grab;

    if (verbose_) {
        std::cout << "[TFF Hotplug] Attached keyboard: " << device_path
                  << " (" << name_str << ")\n";
    }

    return true;
}

bool LinuxPlatform::detachInputDevice(int fd) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
                          [fd](const DeviceInfo& d) { return d.fd == fd; });
    if (it == devices_.end()) {
        return false;
    }

    std::string path = it->path;
    std::string name = it->name;

    if (grabbed_) {
        ioctl(fd, EVIOCGRAB, 0);
    }
    close(fd);

    devices_.erase(it);
    evdev_fd_ = devices_.empty() ? -1 : devices_[0].fd;

    std::cout << "[TFF Hotplug] Detached keyboard: " << path;
    if (!name.empty()) {
        std::cout << " (" << name << ")";
    }
    std::cout << "\n";

    return true;
}

bool LinuxPlatform::openInputDevices(const std::vector<std::string>& device_paths, bool grab) {
    for (const auto& dev : devices_) {
        if (dev.fd >= 0) {
            if (grabbed_) {
                ioctl(dev.fd, EVIOCGRAB, 0);
            }
            close(dev.fd);
        }
    }
    devices_.clear();
    evdev_fd_ = -1;
    grabbed_ = grab;

    for (const auto& path : device_paths) {
        attachInputDevice(path, grab);
    }

    return !devices_.empty();
}

bool LinuxPlatform::enableHotplug(bool enable) {
    if (enable == hotplug_enabled_) {
        return true;
    }
    if (enable) {
        setupInotify();
    } else {
        teardownInotify();
    }
    return hotplug_enabled_ == enable;
}

void LinuxPlatform::setupInotify() {
    if (hotplug_wd_ >= 0) {
        return;
    }
    if (inotify_fd_ < 0) {
        inotify_fd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (inotify_fd_ < 0) {
            std::cerr << "Warning: Failed to initialize inotify for keyboard hotplugging: "
                      << strerror(errno) << "\n";
            hotplug_enabled_ = false;
            return;
        }
    }
    hotplug_wd_ = inotify_add_watch(inotify_fd_, "/dev/input", IN_CREATE | IN_ATTRIB);
    if (hotplug_wd_ < 0) {
        std::cerr << "Warning: Failed to watch /dev/input for hotplugging: "
                  << strerror(errno) << "\n";
        if (config_wd_ < 0) {
            close(inotify_fd_);
            inotify_fd_ = -1;
        }
        hotplug_enabled_ = false;
        return;
    }
    hotplug_enabled_ = true;
    if (verbose_) {
        std::cout << "[TFF Hotplug] Monitoring /dev/input for dynamic keyboard events\n";
    }
}

void LinuxPlatform::teardownInotify() {
    if (hotplug_wd_ >= 0 && inotify_fd_ >= 0) {
        inotify_rm_watch(inotify_fd_, hotplug_wd_);
        hotplug_wd_ = -1;
    }
    hotplug_enabled_ = false;
    if (config_wd_ < 0 && inotify_fd_ >= 0) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
}

bool LinuxPlatform::enableConfigWatch(bool enable, const std::string& config_path) {
    if (!config_path.empty()) {
        config_file_ = config_path;
    }
    if (enable == config_watch_enabled_) {
        return true;
    }
    if (enable) {
        setupConfigWatch();
    } else {
        teardownConfigWatch();
    }
    return config_watch_enabled_ == enable;
}

void LinuxPlatform::setupConfigWatch() {
    if (config_file_.empty()) {
        config_watch_enabled_ = false;
        return;
    }

    if (inotify_fd_ < 0) {
        inotify_fd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
        if (inotify_fd_ < 0) {
            std::cerr << "Warning: Failed to initialize inotify for config watching: "
                      << strerror(errno) << "\n";
            config_watch_enabled_ = false;
            return;
        }
    }

    if (config_wd_ >= 0) {
        inotify_rm_watch(inotify_fd_, config_wd_);
        config_wd_ = -1;
    }

    size_t pos = config_file_.find_last_of('/');
    if (pos != std::string::npos) {
        config_dir_ = config_file_.substr(0, pos);
        config_basename_ = config_file_.substr(pos + 1);
        if (config_dir_.empty()) {
            config_dir_ = "/";
        }
    } else {
        config_dir_ = ".";
        config_basename_ = config_file_;
    }

    // Try watching parent directory for IN_CLOSE_WRITE and IN_MOVED_TO
    config_wd_ = inotify_add_watch(inotify_fd_, config_dir_.c_str(), IN_CLOSE_WRITE | IN_MOVED_TO);
    if (config_wd_ < 0) {
        // Fallback to watching file directly
        config_wd_ = inotify_add_watch(inotify_fd_, config_file_.c_str(), IN_CLOSE_WRITE);
    }

    if (config_wd_ < 0) {
        std::cerr << "Warning: Failed to watch config file (" << config_file_
                  << ") via inotify: " << strerror(errno) << "\n";
        config_watch_enabled_ = false;
        if (hotplug_wd_ < 0) {
            close(inotify_fd_);
            inotify_fd_ = -1;
        }
        return;
    }

    config_watch_enabled_ = true;
    if (verbose_) {
        std::cout << "[TFF Config] Monitoring " << config_file_ << " for live changes\n";
    }
}

void LinuxPlatform::teardownConfigWatch() {
    if (config_wd_ >= 0 && inotify_fd_ >= 0) {
        inotify_rm_watch(inotify_fd_, config_wd_);
        config_wd_ = -1;
    }
    config_watch_enabled_ = false;
    if (hotplug_wd_ < 0 && inotify_fd_ >= 0) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
}

void LinuxPlatform::processInotifyEvents() {
    if (inotify_fd_ < 0) return;

    alignas(struct inotify_event) char buffer[4096];
    while (true) {
        ssize_t bytes = read(inotify_fd_, buffer, sizeof(buffer));
        if (bytes <= 0) {
            break;
        }

        for (ssize_t i = 0; i < bytes;) {
            const struct inotify_event* event = reinterpret_cast<const struct inotify_event*>(buffer + i);
            if (hotplug_wd_ >= 0 && event->wd == hotplug_wd_) {
                if (event->len > 0) {
                    std::string entry_name(event->name);
                    if (entry_name.rfind("event", 0) == 0) {
                        std::string dev_path = "/dev/input/" + entry_name;
                        if (!isDeviceAttached(dev_path)) {
                            // If device node not yet accessible (e.g. udev setting permissions),
                            // retry up to 3 times only if open fails with EACCES or ENOENT.
                            int test_fd = open(dev_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
                            if (test_fd < 0 && (errno == EACCES || errno == ENOENT)) {
                                for (int attempt = 0; attempt < 3; ++attempt) {
                                    usleep(15000);
                                    test_fd = open(dev_path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
                                    if (test_fd >= 0) break;
                                }
                            }
                            if (test_fd >= 0) {
                                close(test_fd);
                            }

                            std::string kbd_name;
                            if (isKeyboardDevice(dev_path, &kbd_name)) {
                                if (attachInputDevice(dev_path, grabbed_)) {
                                    std::cout << "[TFF Hotplug] Connected keyboard: " << dev_path;
                                    if (!kbd_name.empty()) {
                                        std::cout << " (" << kbd_name << ")";
                                    }
                                    std::cout << "\n";
                                }
                            }
                        }
                    }
                }
            } else if (config_wd_ >= 0 && event->wd == config_wd_) {
                bool trigger_reload = false;
                if (event->len > 0) {
                    std::string entry_name(event->name);
                    if (entry_name == config_basename_) {
                        trigger_reload = true;
                    }
                } else {
                    // Direct file watch
                    trigger_reload = true;
                }
                if (trigger_reload) {
                    reloadConfiguration();
                }
            }
            i += sizeof(struct inotify_event) + event->len;
        }
    }
}

void LinuxPlatform::run(std::atomic<bool>& should_stop, std::atomic<bool>* should_reload) {
    if (!initialized_) {
        initialize();
    }

    if (devices_.empty()) {
        auto discovered = discoverKeyboards();
        if (discovered.empty()) {
            std::cout << "No keyboard devices currently connected in /dev/input/. Waiting for keyboards...\n";
        } else {
            std::cout << "Auto-discovered " << discovered.size() << " keyboard(s):\n";
            for (const auto& p : discovered) {
                std::cout << "  " << p << "\n";
            }
            openInputDevices(discovered, grabbed_);
        }
    }

    if (hotplug_enabled_ && hotplug_wd_ < 0) {
        setupInotify();
    }
    if (config_watch_enabled_ && config_wd_ < 0) {
        setupConfigWatch();
    }

    while (!should_stop.load()) {
        if (should_reload && should_reload->exchange(false)) {
            reloadConfiguration();
        }

        std::vector<struct pollfd> pfds;
        if (inotify_fd_ >= 0) {
            struct pollfd pfd;
            pfd.fd = inotify_fd_;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pfds.push_back(pfd);
        }
        for (const auto& dev : devices_) {
            if (dev.fd >= 0) {
                struct pollfd pfd;
                pfd.fd = dev.fd;
                pfd.events = POLLIN;
                pfd.revents = 0;
                pfds.push_back(pfd);
            }
        }

        if (pfds.empty()) {
            std::cerr << "No keyboard devices or hotplug monitor active. Exiting.\n";
            break;
        }

        int timeout_ms = -1;
        if (engine_->hasActiveTimer()) {
            struct timeval now;
            gettimeofday(&now, nullptr);
            tff::TimeVal now_tv{now.tv_sec, now.tv_usec};
            tff::TimeVal target = engine_->getActiveTimerTime();
            if (target <= now_tv) {
                engine_->onTimer(now_tv);
                continue;
            }
            int64_t diff_us = tff::timeSubMicros(now_tv, target);
            timeout_ms = static_cast<int>(diff_us / 1000) + 1;
            if (timeout_ms < 1) timeout_ms = 1;
        }

        int ret = poll(pfds.data(), pfds.size(), timeout_ms);
        if (ret < 0) {
            if (errno == EINTR) {
                if (should_reload && should_reload->exchange(false)) {
                    reloadConfiguration();
                }
                continue;
            }
            std::cerr << "poll error: " << strerror(errno) << "\n";
            break;
        }

        if (ret == 0) {
            // Timer expired
            struct timeval now;
            gettimeofday(&now, nullptr);
            engine_->onTimer(tff::TimeVal{now.tv_sec, now.tv_usec});
            continue;
        }

        // 1. First detach any keyboard devices that encountered disconnect or poll error
        std::vector<int> fds_to_detach;
        for (const auto& pfd : pfds) {
            if (pfd.revents == 0 || pfd.fd == inotify_fd_) {
                continue;
            }
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                fds_to_detach.push_back(pfd.fd);
            }
        }
        for (int fd : fds_to_detach) {
            detachInputDevice(fd);
        }

        // 2. Process inotify events (both hotplug and config changes)
        for (const auto& pfd : pfds) {
            if (pfd.fd == inotify_fd_) {
                if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    std::cerr << "Warning: inotify error; disabling inotify watches.\n";
                    teardownInotify();
                    teardownConfigWatch();
                } else if (pfd.revents & POLLIN) {
                    processInotifyEvents();
                }
                break;
            }
        }

        // 3. Process incoming key events on active devices
        for (const auto& pfd : pfds) {
            if (pfd.fd == inotify_fd_ || (pfd.revents & POLLIN) == 0) {
                continue;
            }
            // Skip devices already marked for detachment
            if (std::find(fds_to_detach.begin(), fds_to_detach.end(), pfd.fd) != fds_to_detach.end()) {
                continue;
            }

            struct input_event ie;
            while (true) {
                ssize_t bytes = read(pfd.fd, &ie, sizeof(ie));
                if (bytes < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    if (errno == EINTR) {
                        continue;
                    }
                    if (errno == ENODEV) {
                        detachInputDevice(pfd.fd);
                        break;
                    }
                    std::cerr << "read error on fd " << pfd.fd << ": " << strerror(errno) << "\n";
                    detachInputDevice(pfd.fd);
                    break;
                }
                if (bytes == 0) {
                    detachInputDevice(pfd.fd);
                    break;
                }
                if (bytes == sizeof(ie)) {
                    tff::Event ev;
                    ev.time = tff::TimeVal{ie.time.tv_sec, ie.time.tv_usec};
                    ev.type = ie.type;
                    ev.code = ie.code;
                    ev.value = ie.value;

                    if (verbose_ && ev.type == EV_KEY) {
                        std::cout << "[TFF In] " << tff::keyCodeToWord(ev.code)
                                  << " (" << ev.code << ") "
                                  << (ev.value == tff::KEY_VAL_DOWN ? "DOWN" :
                                     (ev.value == tff::KEY_VAL_UP ? "UP" : "REPEAT")) << "\n";
                    }

                    engine_->processEvent(ev);
                }
            }
        }
    }

    engine_->finish();
}

bool LinuxPlatform::processEvent(const tff::Event& ev) {
    if (!initialized_) {
        initialize();
    }
    return engine_->processEvent(ev);
}

bool LinuxPlatform::sendKeyEvent(uint32_t key_code, bool is_pressed) {
    if (!initialized_) {
        initialize();
    }

    struct timeval tv;
    gettimeofday(&tv, nullptr);

    tff::Event ev;
    ev.time = tff::TimeVal{tv.tv_sec, tv.tv_usec};
    ev.type = EV_KEY;
    ev.code = static_cast<uint16_t>(key_code);
    ev.value = is_pressed ? tff::KEY_VAL_DOWN : tff::KEY_VAL_UP;

    if (engine_) {
        engine_->processEvent(ev);
    }

    return true;
}

bool LinuxPlatform::receiveMappedKeys(std::vector<uint32_t>& key_codes) {
    if (!initialized_) {
        return false;
    }

    key_codes = received_keys_;
    received_keys_.clear();
    return true;
}

void LinuxPlatform::cleanup() {
    teardownInotify();
    teardownConfigWatch();

    for (const auto& dev : devices_) {
        if (dev.fd >= 0) {
            if (grabbed_) {
                ioctl(dev.fd, EVIOCGRAB, 0);
            }
            close(dev.fd);
        }
    }
    devices_.clear();
    evdev_fd_ = -1;
    grabbed_ = false;

    if (uinput_fd_ >= 0) {
        ioctl(uinput_fd_, UI_DEV_DESTROY);
        close(uinput_fd_);
        uinput_fd_ = -1;
        if (writer_) {
            static_cast<UInputWriter*>(writer_.get())->setUinputFd(-1);
        }
    }

    initialized_ = false;
}

int LinuxPlatform::createVirtualKeyboard(const std::string& device_name) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_MSC) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_REP) < 0) {
        close(fd);
        return -1;
    }

    for (int i = 0; i < KEY_MAX; ++i) {
        ioctl(fd, UI_SET_KEYBIT, i);
    }

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    std::strncpy(usetup.name, device_name.c_str(), UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
        close(fd);
        return -1;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}