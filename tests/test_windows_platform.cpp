// Tests for the Win32 glue (WindowsPlatform, win_util) with SendInput replaced by a recorder.
#include "windows_platform.h"
#include "win_util.h"
#include "tff_key_codes.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using tff::win::HookEvent;
using tff::win::kInjectedMagic;
using tff::win::kLlkhfExtended;
using tff::win::kLlkhfUp;

namespace {

const char kConfig[] =
    "settings:\n"
    "  combo_timeout_ms: 40\n"
    "combos:\n"
    "  - keys: f j\n"
    "    outKeys: delete\n"
    "tap_hold:\n"
    "  capslock:\n"
    "    tap: esc\n"
    "    hold: leftctrl\n";

struct SendRecorder {
    std::vector<INPUT> inputs;
    WindowsPlatform::SendInputFn fn() {
        return [this](UINT count, INPUT* data) {
            inputs.insert(inputs.end(), data, data + count);
            return count;
        };
    }
    std::vector<INPUT> keys() const {
        std::vector<INPUT> out;
        for (const auto& in : inputs) {
            if (in.type == INPUT_KEYBOARD) {
                out.push_back(in);
            }
        }
        return out;
    }
};

std::filesystem::path tempDir() {
    auto dir = std::filesystem::temp_directory_path() / "tff-test-windows-platform";
    std::filesystem::create_directories(dir);
    return dir;
}

std::string writeConfig(const std::string& name, const std::string& content) {
    auto path = tempDir() / std::filesystem::u8path(name);
    std::ofstream f(path, std::ios::binary);
    f << content;
    return path.u8string();
}

HookEvent key(uint32_t vk, uint32_t scan, bool up, uint32_t flags = 0) {
    HookEvent ev;
    ev.vk = vk;
    ev.scan = scan;
    ev.flags = flags | (up ? kLlkhfUp : 0);
    return ev;
}

void waitAndFireTimers(WindowsPlatform& p, int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    p.handleTimers();
}

void test_plain_key_via_scancode() {
    std::cout << "Test 1: Plain key is swallowed and re-injected by scan code... " << std::flush;
    SendRecorder rec;
    WindowsPlatform p;
    p.setSendInputFn(rec.fn());
    std::string err;
    bool ok = p.loadConfiguration(writeConfig("plain.yaml", kConfig), err);
    assert(ok);

    assert(p.handleHookEvent(key(0x41, 0x1E, false)));  // 'a' down: swallowed
    waitAndFireTimers(p, 80);                           // combo timeout expires
    assert(p.handleHookEvent(key(0x41, 0x1E, true)));

    auto keys = rec.keys();
    assert(keys.size() == 2);
    assert(keys[0].ki.wScan == 0x1E && keys[0].ki.wVk == 0);
    assert(keys[0].ki.dwFlags == KEYEVENTF_SCANCODE);
    assert(keys[0].ki.dwExtraInfo == kInjectedMagic);
    assert(keys[1].ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP));
    std::cout << "PASSED\n";
}

void test_chord_extended_key() {
    std::cout << "Test 2: f+j chord emits Delete with KEYEVENTF_EXTENDEDKEY... " << std::flush;
    SendRecorder rec;
    WindowsPlatform p;
    p.setSendInputFn(rec.fn());
    std::string err;
    bool ok = p.loadConfiguration(writeConfig("chord.yaml", kConfig), err);
    assert(ok);

    assert(p.handleHookEvent(key(0x46, 0x21, false)));
    assert(p.handleHookEvent(key(0x4A, 0x24, false)));
    assert(p.handleHookEvent(key(0x46, 0x21, true)));
    assert(p.handleHookEvent(key(0x4A, 0x24, true)));
    p.handleTimers();

    auto keys = rec.keys();
    assert(keys.size() == 2);
    assert(keys[0].ki.wScan == 0x53);
    assert(keys[0].ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY));
    assert(keys[1].ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP));
    std::cout << "PASSED\n";
}

void test_pass_through_cases() {
    std::cout << "Test 3: Own output, unmapped keys and paused state pass through... "
              << std::flush;
    SendRecorder rec;
    WindowsPlatform p;
    p.setSendInputFn(rec.fn());
    std::string err;
    bool ok = p.loadConfiguration(writeConfig("pass.yaml", kConfig), err);
    assert(ok);

    HookEvent own = key(0x41, 0x1E, false, tff::win::kLlkhfInjected);
    own.extra_info = kInjectedMagic;
    assert(!p.handleHookEvent(own));
    assert(!p.handleHookEvent(key(0xFF, 0x00, false)));

    p.setPaused(true);
    assert(p.isPaused());
    assert(!p.handleHookEvent(key(0x41, 0x1E, false)));
    assert(!p.handleHookEvent(key(0x41, 0x1E, true)));
    p.setPaused(false);
    assert(rec.inputs.empty());
    std::cout << "PASSED\n";
}

void test_tap_hold_and_reset() {
    std::cout << "Test 4: Tap-hold modifier, repeat and resetState release held keys... "
              << std::flush;
    SendRecorder rec;
    WindowsPlatform p;
    p.setSendInputFn(rec.fn());
    std::string err;
    bool ok = p.loadConfiguration(writeConfig("taphold.yaml", kConfig), err);
    assert(ok);

    // Hold CapsLock past the tap-hold timeout: LeftCtrl goes down
    assert(p.handleHookEvent(key(0x14, 0x3A, false)));
    waitAndFireTimers(p, 260);
    auto keys = rec.keys();
    assert(!keys.empty());
    assert(keys.back().ki.wScan == 0x1D && (keys.back().ki.dwFlags & KEYEVENTF_KEYUP) == 0);

    // 'a' while held, then auto-repeat of 'a' re-sends 'a' down
    assert(p.handleHookEvent(key(0x41, 0x1E, false)));
    waitAndFireTimers(p, 80);
    size_t before = rec.keys().size();
    assert(p.handleHookEvent(key(0x41, 0x1E, false)));  // repeat
    keys = rec.keys();
    assert(keys.size() == before + 1);
    assert(keys.back().ki.wScan == 0x1E && (keys.back().ki.dwFlags & KEYEVENTF_KEYUP) == 0);

    // Session lock: everything held is released
    rec.inputs.clear();
    p.resetState("test");
    keys = rec.keys();
    assert(keys.size() == 2);
    for (const auto& k : keys) {
        assert((k.ki.dwFlags & KEYEVENTF_KEYUP) != 0);
    }
    assert(p.getOutput().heldCount() == 0);
    std::cout << "PASSED\n";
}

void test_monitor_mode() {
    std::cout << "Test 5: Monitor mode observes without swallowing or injecting... " << std::flush;
    SendRecorder rec;
    WindowsPlatform p;
    tff::MonitorOptions opts;
    opts.color = false;
    p.setMonitorMode(opts);
    p.setSendInputFn(rec.fn());
    std::vector<std::string> lines;
    p.setLogSink([&lines](const std::string& l) { lines.push_back(l); });
    std::string err;
    bool ok = p.loadConfiguration(writeConfig("monitor.yaml", kConfig), err);
    assert(ok);

    assert(!p.handleHookEvent(key(0x46, 0x21, false)));
    assert(!p.handleHookEvent(key(0x4A, 0x24, false)));
    assert(!p.handleHookEvent(key(0x46, 0x21, true)));
    assert(!p.handleHookEvent(key(0x4A, 0x24, true)));
    assert(rec.inputs.empty());
    assert(lines.size() >= 4);
    bool saw_delete = false;
    for (const auto& l : lines) {
        if (l.find("DELETE") != std::string::npos || l.find("delete") != std::string::npos) {
            saw_delete = true;
        }
    }
    assert(saw_delete);
    std::cout << "PASSED\n";
}

void test_config_errors_and_reload() {
    std::cout << "Test 6: Invalid config is rejected, reload keeps the old config... "
              << std::flush;
    WindowsPlatform p;
    std::string err;
    assert(!p.loadConfiguration("C:\\does\\not\\exist\\tff.yaml", err));
    assert(err.find("Failed to open") != std::string::npos);

    // Non-ASCII path (UTF-8 active code page + u8path)
    const std::string path = writeConfig(u8"k\u00f6nfig-\u00e4\u00fc.yaml", kConfig);
    bool ok = p.loadConfiguration(path, err);
    assert(ok);
    assert(p.getEngine().getCombos().size() == 1);

    {
        std::ofstream f(std::filesystem::u8path(path), std::ios::binary);
        f << "combos:\n  - keys: f\n    outKeys: notakey\n";
    }
    assert(!p.reloadConfiguration(err));
    assert(p.getEngine().getCombos().size() == 1);

    {
        std::ofstream f(std::filesystem::u8path(path), std::ios::binary);
        f << "combos:\r\n  - keys: f j\r\n    outKeys: delete\r\n  - keys: j f\r\n"
             "    outKeys: backspace\r\n";
    }
    ok = p.reloadConfiguration(err);
    assert(ok);  // CRLF line endings (Notepad) parse fine
    assert(p.getEngine().getCombos().size() == 2);
    std::cout << "PASSED\n";
}

void test_win_util() {
    std::cout << "Test 7: UTF-8 helpers and default paths... " << std::flush;
    const std::string s = u8"Gr\u00fc\u00dfe \u2192 \U0001F600";
    assert(tff::win::narrow(tff::win::widen(s)) == s);
    assert(tff::win::widen("").empty());
    assert(tff::win::userConfigPath().size() > 20);
    assert(tff::win::userConfigPath().find("\\tff\\tff-combos.yaml") != std::string::npos);
    assert(!tff::win::exeDir().empty());
    std::string content;
    assert(tff::win::readTextFile(writeConfig(u8"\u00f6.yaml", "x: 1\n"), content));
    assert(content == "x: 1\n");
    std::cout << "PASSED\n";
}

void test_run_and_stop() {
    std::cout << "Test 8: run() installs the hook and stops on request... " << std::flush;
    WindowsPlatform p;
    std::string err;
    bool ok = p.loadConfiguration(writeConfig("run.yaml", kConfig), err);
    assert(ok);
    int rc = -1;
    std::thread t([&] { rc = p.run(); });
    for (int i = 0; i < 200 && !p.isRunning(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const bool started = p.isRunning();
    p.requestStop();
    t.join();
    if (!started) {
        std::cout << "SKIPPED (hook could not be installed, rc=" << rc << ")\n";
        return;
    }
    assert(rc == 0);
    assert(!p.isRunning());
    std::cout << "PASSED\n";
}

}  // namespace

int main() {
    std::cout << "Running Windows platform tests:\n";
    test_plain_key_via_scancode();
    test_chord_extended_key();
    test_pass_through_cases();
    test_tap_hold_and_reset();
    test_monitor_mode();
    test_config_errors_and_reload();
    test_win_util();
    test_run_and_stop();
    std::cout << "All Windows platform tests PASSED!\n";
    return 0;
}
