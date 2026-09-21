#include "linux_platform.h"
#include "tff_key_codes.h"
#include <iostream>
#include <fstream>
#include <cstdio>
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
        if (c.keys.size() == 2 && c.keys[0] == tff::Keys::KEY_J && c.keys[1] == tff::Keys::KEY_F &&
            c.out_keys.size() == 1 && c.out_keys[0] == tff::Keys::KEY_BACKSPACE) {
            found_jf_backspace = true;
            break;
        }
    }
    assert(found_jf_backspace);

    // Verify d f j -> esc (triple combo)
    bool found_dfj_esc = false;
    for (const auto& c : combos) {
        if (c.keys.size() == 3 && c.keys[0] == tff::Keys::KEY_D && c.keys[1] == tff::Keys::KEY_F &&
            c.keys[2] == tff::Keys::KEY_J && c.out_keys.size() == 1 &&
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
        if (k == tff::Keys::KEY_BACKSPACE)
            has_backspace = true;
        if (k == tff::Keys::KEY_J)
            has_j = true;
        if (k == tff::Keys::KEY_F)
            has_f = true;
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
        bool disabled = platform.enableHotplug(false);
        assert(disabled);
        assert(!platform.isHotplugEnabled());
    }

    platform.setGrab(true);
    assert(platform.isGrabbed());
    platform.setGrab(false);
    assert(!platform.isGrabbed());

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

void testConfigHotReloadValid() {
    std::cout << "Test 8: Configuration hot-reloading with valid YAML... ";
    LinuxPlatform platform;
    platform.initialize();

    const std::string temp_yaml = "/tmp/tff_test_config_reload.yaml";
    {
        std::ofstream out(temp_yaml);
        out << "combos:\n"
            << "  - keys: f j\n"
            << "    outKeys: 1\n";
    }

    bool loaded = platform.loadConfiguration(temp_yaml);
    assert(loaded);
    assert(platform.getConfigFilePath() == temp_yaml);
    assert(platform.getComboCount() == 1);

    // Update configuration with a second combo
    {
        std::ofstream out(temp_yaml);
        out << "combos:\n"
            << "  - keys: f j\n"
            << "    outKeys: 1\n"
            << "  - keys: j f\n"
            << "    outKeys: 2\n";
    }

    // Hot-reload without restarting or dropping grabs
    bool reloaded = platform.reloadConfiguration();
    assert(reloaded);
    assert(platform.getComboCount() == 2);

    std::remove(temp_yaml.c_str());
    platform.cleanup();
    std::cout << "PASSED\n";
}

void testConfigHotReloadInvalidSyntaxPreservesCurrent() {
    std::cout << "Test 9: Invalid syntax on reload retains existing configuration... ";
    LinuxPlatform platform;
    platform.initialize();

    const std::string temp_yaml = "/tmp/tff_test_config_invalid.yaml";
    {
        std::ofstream out(temp_yaml);
        out << "combos:\n"
            << "  - keys: f j\n"
            << "    outKeys: 1\n"
            << "  - keys: j f\n"
            << "    outKeys: 2\n";
    }

    bool loaded = platform.loadConfiguration(temp_yaml);
    assert(loaded);
    assert(platform.getComboCount() == 2);

    // Corrupt the YAML file with invalid syntax (unknown key word)
    {
        std::ofstream out(temp_yaml);
        out << "combos:\n"
            << "  - keys: unknown_invalid_key_xyz\n"
            << "    outKeys: 1\n";
    }

    // Reload should fail gracefully and NOT crash or clear existing combos
    bool reloaded = platform.reloadConfiguration();
    assert(!reloaded);
    assert(platform.getComboCount() == 2);

    // Non-existent file reload also fails gracefully and keeps combos
    bool non_existent = platform.reloadConfiguration("/tmp/this_file_does_not_exist_987654.yaml");
    assert(!non_existent);
    assert(platform.getComboCount() == 2);

    std::remove(temp_yaml.c_str());
    platform.cleanup();
    std::cout << "PASSED\n";
}

void testConfigWatchInotify() {
    std::cout << "Test 10: Configuration inotify watch toggle... ";
    LinuxPlatform platform;
    platform.initialize();

    const std::string temp_yaml = "/tmp/tff_test_config_watch.yaml";
    {
        std::ofstream out(temp_yaml);
        out << "combos:\n"
            << "  - keys: f j\n"
            << "    outKeys: 1\n";
    }

    platform.loadConfiguration(temp_yaml);
    assert(!platform.isConfigWatchEnabled());

    bool watch_enabled = platform.enableConfigWatch(true, temp_yaml);
    if (watch_enabled) {
        assert(platform.isConfigWatchEnabled());
        bool watch_disabled = platform.enableConfigWatch(false);
        assert(watch_disabled);
        assert(!platform.isConfigWatchEnabled());
    }

    std::remove(temp_yaml.c_str());
    platform.cleanup();
    std::cout << "PASSED\n";
}

void testPackagingLayoutAndIntegrity() {
    std::cout << "Test 11: Packaging layout and configuration files... ";

    // Helper to read entire file (supports running from repo root or build dir)
    auto readFile = [](const std::string& path) -> std::string {
        std::ifstream in(path);
        if (!in.is_open()) {
            in.open("../" + path);
        }
        if (!in.is_open())
            return "";
        return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    };

    // 1. Udev rules
    std::string udev = readFile("packaging/udev/99-tff.rules");
    assert(!udev.empty());
    assert(udev.find("KERNEL==\"uinput\"") != std::string::npos);
    assert(udev.find("GROUP=\"input\"") != std::string::npos);
    assert(udev.find("TAG+=\"uaccess\"") != std::string::npos);

    // 2. Systemd units
    std::string sys_unit = readFile("packaging/systemd/system/ten-flying-fingers.service");
    assert(!sys_unit.empty());
    assert(sys_unit.find("ExecStart=/usr/bin/tff --config /etc/tff/tff-combos.yaml") !=
           std::string::npos);
    assert(sys_unit.find("Restart=always") != std::string::npos);

    std::string user_unit = readFile("packaging/systemd/user/ten-flying-fingers.service");
    assert(!user_unit.empty());
    assert(user_unit.find("ExecStart=/usr/bin/tff") != std::string::npos);

    // 3. Debian packaging metadata
    std::string conffiles = readFile("packaging/debian/conffiles");
    assert(!conffiles.empty());
    assert(conffiles.find("/etc/tff/tff-combos.yaml") != std::string::npos);

    std::string postinst = readFile("packaging/debian/postinst");
    assert(!postinst.empty());
    assert(postinst.rfind("#!/bin/sh", 0) == 0);
    assert(postinst.find("udevadm") != std::string::npos);
    assert(postinst.find("systemctl") != std::string::npos);

    std::string postrm = readFile("packaging/debian/postrm");
    assert(!postrm.empty());
    assert(postrm.rfind("#!/bin/sh", 0) == 0);
    assert(postrm.find("udevadm") != std::string::npos);
    assert(postrm.find("systemctl") != std::string::npos);

    std::string control_in = readFile("packaging/debian/control.in");
    assert(!control_in.empty());
    assert(control_in.find("Package: tff2") != std::string::npos);
    assert(control_in.find("Provides: tff, tff-linux") != std::string::npos);
    assert(control_in.find("Depends: libc6 (>= 2.31)") != std::string::npos);

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
    testConfigHotReloadValid();
    testConfigHotReloadInvalidSyntaxPreservesCurrent();
    testConfigWatchInotify();
    testPackagingLayoutAndIntegrity();
    std::cout << "All Linux platform tests PASSED!\n";
    return 0;
}
