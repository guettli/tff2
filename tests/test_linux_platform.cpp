#include "linux_platform.h"
#include "tff_key_codes.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <algorithm>

void testInitialization() {
    std::cout << "Test 1: LinuxPlatform initialization... ";
    LinuxPlatform platform;
    bool ok = platform.initialize();
    assert(ok);
    assert(platform.isInitialized());
    platform.cleanup();
    assert(!platform.isInitialized());
    std::cout << "PASSED\n";
}

void testDiscoverKeyboards() {
    std::cout << "Test 2: Keyboard auto-discovery... ";
    auto keyboards = LinuxPlatform::discoverKeyboards();
    // Validate all discovered keyboards
    for (const auto& dev : keyboards) {
        std::string name = LinuxPlatform::getDeviceName(dev);
        // Ensure virtual TFF device is excluded
        assert(name.find("TFF Virtual Keyboard") == std::string::npos);
        // Ensure device path is valid
        assert(dev.rfind("/dev/input/", 0) == 0);
    }
    std::cout << "PASSED (found " << keyboards.size() << " keyboard(s))\n";
}

void testLoadYamlConfiguration() {
    std::cout << "Test 3: Loading YAML combo configuration... ";
    LinuxPlatform platform;
    platform.initialize();

    bool ok = platform.loadConfiguration("config/tff-combos.yaml");
    assert(ok);

    const auto& combos = platform.getEngine().getCombos();
    assert(combos.size() >= 10);

    // Verify j f -> backspace
    bool found_jf_backspace = false;
    for (const auto& c : combos) {
        if (c.keys.size() == 2 &&
            c.keys[0] == tff::Keys::KEY_J &&
            c.keys[1] == tff::Keys::KEY_F &&
            c.out_keys.size() == 1 &&
            c.out_keys[0] == tff::Keys::KEY_BACKSPACE) {
            found_jf_backspace = true;
            break;
        }
    }
    assert(found_jf_backspace);

    // Verify d f j -> esc (triple combo)
    bool found_dfj_esc = false;
    for (const auto& c : combos) {
        if (c.keys.size() == 3 &&
            c.keys[0] == tff::Keys::KEY_D &&
            c.keys[1] == tff::Keys::KEY_F &&
            c.keys[2] == tff::Keys::KEY_J &&
            c.out_keys.size() == 1 &&
            c.out_keys[0] == tff::Keys::KEY_ESC) {
            found_dfj_esc = true;
            break;
        }
    }
    assert(found_dfj_esc);

    platform.cleanup();
    std::cout << "PASSED (" << combos.size() << " combos verified)\n";
}

void testComboRemapping() {
    std::cout << "Test 4: Combo remapping (j + f -> backspace)... ";
    LinuxPlatform platform;
    platform.initialize();

    // Set combo: j f -> backspace
    tff::Combo jf;
    jf.keys = {tff::Keys::KEY_J, tff::Keys::KEY_F};
    jf.out_keys = {tff::Keys::KEY_BACKSPACE};
    platform.setCombos({jf});

    // Send J down
    platform.sendKeyEvent(tff::Keys::KEY_J, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Send F down (within threshold)
    platform.sendKeyEvent(tff::Keys::KEY_F, true);
    // Overlap must be at least 40ms (min_overlap_us = 40000)
    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    // Release J and F
    platform.sendKeyEvent(tff::Keys::KEY_J, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    platform.sendKeyEvent(tff::Keys::KEY_F, false);

    std::vector<uint32_t> received;
    platform.receiveMappedKeys(received);

    // Should have received KEY_BACKSPACE
    bool has_backspace = false;
    for (uint32_t k : received) {
        if (k == tff::Keys::KEY_BACKSPACE) {
            has_backspace = true;
            break;
        }
    }
    assert(has_backspace);

    platform.cleanup();
    std::cout << "PASSED\n";
}

void testSequentialTypingNoRemap() {
    std::cout << "Test 5: Sequential typing without combo overlap... ";
    LinuxPlatform platform;
    platform.initialize();

    tff::Combo jf;
    jf.keys = {tff::Keys::KEY_J, tff::Keys::KEY_F};
    jf.out_keys = {tff::Keys::KEY_BACKSPACE};
    platform.setCombos({jf});

    // Press J down and up
    platform.sendKeyEvent(tff::Keys::KEY_J, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    platform.sendKeyEvent(tff::Keys::KEY_J, false);

    // Wait 200ms (> 150ms timeout)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Press F down and up
    platform.sendKeyEvent(tff::Keys::KEY_F, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    platform.sendKeyEvent(tff::Keys::KEY_F, false);

    std::vector<uint32_t> received;
    platform.receiveMappedKeys(received);

    // Must NOT contain backspace; must contain J and F
    bool has_backspace = false;
    bool has_j = false;
    bool has_f = false;
    for (uint32_t k : received) {
        if (k == tff::Keys::KEY_BACKSPACE) has_backspace = true;
        if (k == tff::Keys::KEY_J) has_j = true;
        if (k == tff::Keys::KEY_F) has_f = true;
    }
    assert(!has_backspace);
    assert(has_j);
    assert(has_f);

    platform.cleanup();
    std::cout << "PASSED\n";
}

void testHotplugConfiguration() {
    std::cout << "Test 6: Inotify hotplug configuration... ";
    LinuxPlatform platform;
    platform.initialize();

    assert(!platform.isHotplugEnabled());
    bool ok = platform.enableHotplug(true);
    // In environments with /dev/input permissions, ok is true; otherwise false
    if (ok) {
        assert(platform.isHotplugEnabled());
        platform.enableHotplug(false);
        assert(!platform.isHotplugEnabled());
    }

    platform.cleanup();
    assert(!platform.isHotplugEnabled());
    std::cout << "PASSED\n";
}

void testDeviceManagement() {
    std::cout << "Test 7: Device attach/detach and keyboard validation... ";
    LinuxPlatform platform;
    platform.initialize();

    assert(platform.getAttachedDeviceCount() == 0);
    assert(!platform.isDeviceAttached("/dev/input/nonexistent_device_test"));

    // Validation checks for non-devices and non-keyboards
    std::string name;
    assert(!LinuxPlatform::isKeyboardDevice("/dev/null", &name));
    assert(!LinuxPlatform::isKeyboardDevice("/nonexistent/file/path", &name));

    // Discover existing keyboards (if any in test environment)
    auto keyboards = LinuxPlatform::discoverKeyboards();
    if (!keyboards.empty()) {
        const std::string& first_kbd = keyboards[0];
        assert(LinuxPlatform::isKeyboardDevice(first_kbd, &name));

        bool attached = platform.attachInputDevice(first_kbd, false);
        if (attached) {
            assert(platform.isDeviceAttached(first_kbd));
            assert(platform.getAttachedDeviceCount() == 1);

            // Attaching same device again should be idempotent
            bool reattached = platform.attachInputDevice(first_kbd, false);
            assert(reattached);
            assert(platform.getAttachedDeviceCount() == 1);

            int fd = platform.getInputFd();
            assert(fd >= 0);
            bool detached = platform.detachInputDevice(fd);
            assert(detached);
            assert(platform.getAttachedDeviceCount() == 0);
            assert(!platform.isDeviceAttached(first_kbd));
        }
    }

    platform.cleanup();
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Linux Platform Tests ===\n";
    testInitialization();
    testDiscoverKeyboards();
    testLoadYamlConfiguration();
    testComboRemapping();
    testSequentialTypingNoRemap();
    testHotplugConfiguration();
    testDeviceManagement();
    std::cout << "All Linux platform tests PASSED!\n";
    return 0;
}
