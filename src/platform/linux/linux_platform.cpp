#include "linux_platform.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <cstring>
#include <cerrno>
#include <iostream>

LinuxPlatform::LinuxPlatform()
    : uinput_fd_(-1), evdev_fd_(-1), initialized_(false) {
}

LinuxPlatform::~LinuxPlatform() {
    cleanup();
}

bool LinuxPlatform::initialize() {
    if (initialized_) {
        return true;
    }

    // For testing purposes, we'll just simulate the interface
    // In a real implementation, we would create actual uinput devices
    initialized_ = true;
    std::cout << "LinuxPlatform initialized for testing\n";
    return true;
}

bool LinuxPlatform::sendKeyEvent(uint32_t key_code, bool is_pressed) {
    if (!initialized_) {
        std::cerr << "LinuxPlatform not initialized\n";
        return false;
    }

    // Convert our internal key codes to Linux key codes
    uint16_t linux_key_code = static_cast<uint16_t>(key_code);

    std::cout << "Sending key event: code=" << key_code
              << ", pressed=" << (is_pressed ? "true" : "false") << "\n";

    // In a real implementation, we would emit the actual event:
    // emitEvent(uinput_fd_, EV_KEY, linux_key_code, is_pressed ? 1 : 0);
    // emitEvent(uinput_fd_, EV_SYN, SYN_REPORT, 0);

    return true;
}

bool LinuxPlatform::receiveMappedKeys(std::vector<uint32_t>& key_codes) {
    if (!initialized_) {
        std::cerr << "LinuxPlatform not initialized\n";
        return false;
    }

    // For testing, we'll just return any simulated received keys
    key_codes = received_keys_;
    received_keys_.clear();

    return true;
}

void LinuxPlatform::cleanup() {
    if (uinput_fd_ >= 0) {
        ioctl(uinput_fd_, UI_DEV_DESTROY);
        close(uinput_fd_);
        uinput_fd_ = -1;
    }

    if (evdev_fd_ >= 0) {
        close(evdev_fd_);
        evdev_fd_ = -1;
    }

    initialized_ = false;
}

int LinuxPlatform::createVirtualKeyboard() {
    // This would create a virtual keyboard device for sending events
    // For now, we'll just return a dummy file descriptor
    return 0;
}

int LinuxPlatform::createVirtualOutputDevice() {
    // This would create a virtual output device for receiving events
    // For now, we'll just return a dummy file descriptor
    return 0;
}

bool LinuxPlatform::emitEvent(int fd, uint16_t type, uint16_t code, int32_t value) {
    // This would emit an actual input event
    // For now, we'll just simulate it
    std::cout << "Emitting event: type=" << type << ", code=" << code
              << ", value=" << value << "\n";
    return true;
}