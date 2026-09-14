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
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

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
      verbose_(false) {
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

bool LinuxPlatform::loadConfiguration(const std::string& config_file) {
    if (!initialized_) {
        initialize();
    }

    std::ifstream file(config_file);
    if (!file.is_open() && config_file.rfind("../", 0) != 0) {
        file.open("../" + config_file);
    }
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << config_file << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string err_msg;
    std::vector<tff::Combo> combos;
    if (!tff::loadYamlCombos(buffer.str(), combos, err_msg)) {
        std::cerr << "Failed to parse YAML config (" << config_file << "): " << err_msg << "\n";
        return false;
    }

    setCombos(combos);
    return true;
}

std::vector<std::string> LinuxPlatform::discoverKeyboards() {
    std::vector<std::string> keyboards;
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        return keyboards;
    }

    std::vector<std::pair<int, std::string>> candidates;
    struct dirent* ent;
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
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            continue;
        }

        char dev_name[256] = {0};
        ioctl(fd, EVIOCGNAME(sizeof(dev_name) - 1), dev_name);
        std::string name_str(dev_name);

        // Skip virtual TFF devices to avoid self-monitoring loops
        if (name_str.find("TFF Virtual Keyboard") != std::string::npos ||
            name_str.find("tff-clone") != std::string::npos) {
            close(fd);
            continue;
        }

        unsigned long ev_bits[(EV_MAX + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0 || !TEST_BIT_EV(EV_KEY, ev_bits)) {
            close(fd);
            continue;
        }

        unsigned long key_bits[(KEY_MAX + sizeof(unsigned long) * 8 - 1) / (sizeof(unsigned long) * 8)] = {0};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
            close(fd);
            continue;
        }

        // Must support typical alphanumeric keyboard keys
        if (TEST_BIT_EV(KEY_A, key_bits) &&
            TEST_BIT_EV(KEY_SPACE, key_bits) &&
            TEST_BIT_EV(KEY_ENTER, key_bits)) {
            keyboards.push_back(path);
        }

        close(fd);
    }

    return keyboards;
}

bool LinuxPlatform::openInputDevice(const std::string& device_path, bool grab) {
    return openInputDevices({device_path}, grab);
}

bool LinuxPlatform::openInputDevices(const std::vector<std::string>& device_paths, bool grab) {
    // Close existing
    for (int fd : input_fds_) {
        if (fd >= 0) {
            if (grabbed_) {
                ioctl(fd, EVIOCGRAB, 0);
            }
            close(fd);
        }
    }
    input_fds_.clear();
    evdev_fd_ = -1;
    grabbed_ = false;

    for (const auto& path : device_paths) {
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "Failed to open input device " << path << ": " << strerror(errno) << "\n";
            continue;
        }

        if (grab) {
            if (ioctl(fd, EVIOCGRAB, 1) < 0) {
                std::cerr << "Warning: Failed to grab device " << path << ": " << strerror(errno) << "\n";
            }
        }

        input_fds_.push_back(fd);
    }

    if (!input_fds_.empty()) {
        evdev_fd_ = input_fds_[0];
        grabbed_ = grab;
        return true;
    }

    return false;
}

void LinuxPlatform::run(std::atomic<bool>& should_stop) {
    if (!initialized_) {
        initialize();
    }

    if (input_fds_.empty()) {
        auto discovered = discoverKeyboards();
        if (discovered.empty()) {
            std::cerr << "No keyboard devices found in /dev/input/\n";
            return;
        }
        std::cout << "Auto-discovered " << discovered.size() << " keyboard(s):\n";
        for (const auto& p : discovered) {
            std::cout << "  " << p << "\n";
        }
        if (!openInputDevices(discovered, true)) {
            std::cerr << "Failed to open discovered keyboard devices\n";
            return;
        }
    }

    std::vector<struct pollfd> pfds;
    for (int fd : input_fds_) {
        if (fd >= 0) {
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pfds.push_back(pfd);
        }
    }

    if (pfds.empty()) {
        std::cerr << "No valid file descriptors to monitor\n";
        return;
    }

    while (!should_stop.load()) {
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

        for (auto& pfd : pfds) {
            pfd.revents = 0;
        }

        int ret = poll(pfds.data(), pfds.size(), timeout_ms);
        if (ret < 0) {
            if (errno == EINTR) {
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

        for (auto& pfd : pfds) {
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                std::cerr << "Device error/disconnect on fd " << pfd.fd << "\n";
                should_stop.store(true);
                break;
            }

            if (pfd.revents & POLLIN) {
                struct input_event ie;
                while (true) {
                    ssize_t bytes = read(pfd.fd, &ie, sizeof(ie));
                    if (bytes < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        std::cerr << "read error: " << strerror(errno) << "\n";
                        should_stop.store(true);
                        break;
                    }
                    if (bytes == 0) {
                        should_stop.store(true);
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
    for (int fd : input_fds_) {
        if (fd >= 0) {
            if (grabbed_) {
                ioctl(fd, EVIOCGRAB, 0);
            }
            close(fd);
        }
    }
    input_fds_.clear();
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
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
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

bool LinuxPlatform::emitEvent(int fd, uint16_t type, uint16_t code, int32_t value) {
    if (fd < 0) return false;
    struct input_event ie;
    std::memset(&ie, 0, sizeof(ie));
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    ie.time = tv;
    ie.type = type;
    ie.code = code;
    ie.value = value;
    ssize_t bytes = write(fd, &ie, sizeof(ie));
    return bytes == sizeof(ie);
}