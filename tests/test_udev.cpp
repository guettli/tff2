#include "tff_udev.h"

#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <unistd.h>

void testRuleContent() {
    std::cout << "Test 1: Udev rule content..." << std::endl;
    std::string rule = tff::udev::getUdevRuleContent();
    assert(!rule.empty());
    assert(rule.find("KERNEL==\"uinput\"") != std::string::npos);
    assert(rule.find("KERNEL==\"event*\"") != std::string::npos);
    assert(rule.find("TAG+=\"uaccess\"") != std::string::npos);
    assert(rule.find("GROUP=\"input\"") != std::string::npos);
    assert(rule.find("MODE=\"0660\"") != std::string::npos);

    std::string mod = tff::udev::getModulesLoadContent();
    assert(!mod.empty());
    assert(mod.find("uinput") != std::string::npos);
    std::cout << "  Passed!" << std::endl;
}

void testCheckPermissions() {
    std::cout << "Test 2: Check permissions API..." << std::endl;
    auto res = tff::udev::checkPermissions("/nonexistent/path/rule.rules");
    assert(!res.username.empty());
    assert(res.uid == getuid());
    assert(res.gid == getgid());
    assert(res.is_root == (getuid() == 0));
    assert(!res.udev_rule_installed);
    std::cout << "  User: " << res.username << " (UID: " << res.uid << ")" << std::endl;
    std::cout << "  /dev/uinput exists: " << (res.uinput_exists ? "yes" : "no") << std::endl;
    std::cout << "  Event devices: " << res.event_devices_total << std::endl;
    std::cout << "  Passed!" << std::endl;
}

void testFormatDiagnosticReport() {
    std::cout << "Test 3: Diagnostic report formatting..." << std::endl;
    tff::udev::PermissionCheckResult res;
    res.username = "testuser";
    res.uid = 1000;
    res.gid = 1000;
    res.is_root = false;
    res.in_input_group_active = false;
    res.in_input_group_configured = false;
    res.uinput_exists = true;
    res.uinput_readable = false;
    res.uinput_writable = false;
    res.event_devices_total = 2;
    res.event_devices_accessible = 0;
    res.udev_rule_installed = false;
    res.can_run_non_root = false;

    tff::udev::SetupUdevOptions opts;
    opts.color = false;
    std::string plain = tff::udev::formatDiagnosticReport(res, opts);
    assert(plain.find("\033[") == std::string::npos); // No ANSI color escapes
    assert(plain.find("testuser") != std::string::npos);
    assert(plain.find("NON-ROOT EXECUTION NOT READY") != std::string::npos);
    assert(plain.find("sudo usermod -aG input testuser") != std::string::npos);
    assert(plain.find("sudo tff setup-udev --install") != std::string::npos);

    opts.color = true;
    std::string colored = tff::udev::formatDiagnosticReport(res, opts);
    assert(colored.find("\033[") != std::string::npos); // ANSI color escapes present

    // Test when ready
    res.can_run_non_root = true;
    std::string ready_report = tff::udev::formatDiagnosticReport(res, opts);
    assert(ready_report.find("READY FOR NON-ROOT EXECUTION") != std::string::npos);
    std::cout << "  Passed!" << std::endl;
}

void testInstallUdevRuleCustomPath() {
    std::cout << "Test 4: Install udev rule to custom path..." << std::endl;
    std::string test_dir = "/tmp/tff_udev_test_" + std::to_string(getpid());
    std::string test_rule = test_dir + "/subdir/99-tff-test.rules";
    std::string test_mod = test_dir + "/modules/uinput-test.conf";

    tff::udev::SetupUdevOptions opts;
    opts.rule_path = test_rule;
    opts.modules_load_path = test_mod;
    opts.reload = false; // Do not run system udevadm during test

    std::string err_msg;
    bool ok = tff::udev::installUdevRule(opts, err_msg);
    assert(ok);
    assert(err_msg.empty());

    // Verify rule file was written
    bool rule_exists = std::filesystem::exists(test_rule);
    assert(rule_exists);
    std::ifstream in(test_rule);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    assert(content == tff::udev::getUdevRuleContent());

    // Verify checkPermissions detects installed rule
    auto check_res = tff::udev::checkPermissions(test_rule);
    assert(check_res.udev_rule_installed);
    assert(check_res.installed_rule_path == test_rule);

    // Clean up
    std::filesystem::remove_all(test_dir);
    std::cout << "  Passed!" << std::endl;
}

void testInstallUdevRuleErrorHandling() {
    std::cout << "Test 5: Install udev rule error handling..." << std::endl;
    if (getuid() != 0) {
        tff::udev::SetupUdevOptions opts;
        opts.rule_path = "/root/nonexistent_forbidden_dir/99-tff.rules";
        opts.reload = false;

        std::string err_msg;
        bool ok = tff::udev::installUdevRule(opts, err_msg);
        assert(!ok);
        assert(!err_msg.empty());
    }
    std::cout << "  Passed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running TFF udev and permissions tests" << std::endl;
    std::cout << "========================================" << std::endl;

    testRuleContent();
    testCheckPermissions();
    testFormatDiagnosticReport();
    testInstallUdevRuleCustomPath();
    testInstallUdevRuleErrorHandling();

    std::cout << "\nAll 5 udev tests passed successfully!" << std::endl;
    return 0;
}
