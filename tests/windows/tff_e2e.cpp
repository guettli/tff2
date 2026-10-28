// End-to-end test for tff.exe on a real Windows desktop.
//
//   tff_e2e.exe <path to tff.exe> <config.yaml>
//
// 1. Installs its own WH_KEYBOARD_LL "sink" hook. tff.exe installs its hook later, so Windows
//    calls tff.exe first: keys tff swallows never reach the sink, and everything tff.exe injects
//    (tagged with kInjectedMagic) passes tff and is recorded - and swallowed - by the sink, so
//    nothing is typed into whatever window has focus.
// 2. Starts tff.exe and types scenarios with SendInput (scan codes, like a keyboard).
// 3. Compares what tff.exe injected with the expected key sequence.
//
// Exit codes: 0 passed, 1 failed, 77 skipped (no interactive desktop: hooks see no input).
#include "win_input.h"
#include "tff_key_codes.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <timeapi.h>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using tff::KeyCode;
namespace Keys = tff::Keys;

namespace {

constexpr ULONG_PTR kHarnessMagic = 0x45324554;  // "TE2E": keys typed by this harness
constexpr int kSkip = 77;

struct Recorded {
    KeyCode code;
    bool up;
    bool operator==(const Recorded& o) const { return code == o.code && up == o.up; }
};

std::mutex g_mutex;
std::vector<Recorded> g_output;  // injected by tff.exe
std::vector<Recorded> g_leaked;  // typed by us and NOT swallowed by tff.exe
std::atomic<int> g_harness_seen{0};

LRESULT CALLBACK sinkHook(int code, WPARAM wparam, LPARAM lparam) {
    if (code == HC_ACTION) {
        const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lparam);
        KeyCode key = 0;
        tff::win::hookToKeyCode(k->vkCode, k->scanCode, (k->flags & LLKHF_EXTENDED) != 0, key);
        const bool up = (k->flags & LLKHF_UP) != 0;
        if (k->dwExtraInfo == tff::win::kInjectedMagic) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_output.push_back({key, up});
            return 1;
        }
        if (k->dwExtraInfo == kHarnessMagic) {
            ++g_harness_seen;
            std::lock_guard<std::mutex> lock(g_mutex);
            g_leaked.push_back({key, up});
            return 1;
        }
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

struct Step {
    KeyCode key;
    bool up;
    int sleep_ms;
};

bool type(const Step& s) {
    tff::win::KeyMapping m;
    if (!tff::win::keyCodeToMapping(s.key, m)) {
        return false;
    }
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wScan = m.scan;
    in.ki.dwFlags = KEYEVENTF_SCANCODE | (m.extended ? KEYEVENTF_EXTENDEDKEY : 0) |
                    (s.up ? KEYEVENTF_KEYUP : 0);
    in.ki.dwExtraInfo = kHarnessMagic;
    const UINT n = SendInput(1, &in, sizeof(INPUT));
    Sleep(static_cast<DWORD>(s.sleep_ms));
    return n == 1;
}

std::string describe(const std::vector<Recorded>& seq) {
    std::ostringstream ss;
    for (const auto& r : seq) {
        ss << tff::keyCodeToWord(r.code) << (r.up ? "^ " : "v ");
    }
    return ss.str().empty() ? "(nothing)" : ss.str();
}

struct Scenario {
    const char* name;
    std::vector<Step> steps;
    std::vector<Recorded> expected;
};

std::vector<Scenario> scenarios() {
    const bool D = false;
    const bool U = true;
    return {
        {"plain keys pass through",
         {{Keys::KEY_A, D, 30}, {Keys::KEY_A, U, 150}, {Keys::KEY_B, D, 30}, {Keys::KEY_B, U, 0}},
         {{Keys::KEY_A, D}, {Keys::KEY_A, U}, {Keys::KEY_B, D}, {Keys::KEY_B, U}}},
        {"chord f+j -> delete",
         {{Keys::KEY_F, D, 10}, {Keys::KEY_J, D, 40}, {Keys::KEY_F, U, 10}, {Keys::KEY_J, U, 0}},
         {{Keys::KEY_DELETE, D}, {Keys::KEY_DELETE, U}}},
        {"chord j+f -> backspace",
         {{Keys::KEY_J, D, 10}, {Keys::KEY_F, D, 40}, {Keys::KEY_J, U, 10}, {Keys::KEY_F, U, 0}},
         {{Keys::KEY_BACKSPACE, D}, {Keys::KEY_BACKSPACE, U}}},
        {"f alone after combo timeout",
         {{Keys::KEY_F, D, 200}, {Keys::KEY_F, U, 0}},
         {{Keys::KEY_F, D}, {Keys::KEY_F, U}}},
        {"capslock tap -> esc",
         {{Keys::KEY_CAPSLOCK, D, 50}, {Keys::KEY_CAPSLOCK, U, 0}},
         {{Keys::KEY_ESC, D}, {Keys::KEY_ESC, U}}},
        {"capslock hold + c -> ctrl+c",
         {{Keys::KEY_CAPSLOCK, D, 400},
          {Keys::KEY_C, D, 30},
          {Keys::KEY_C, U, 30},
          {Keys::KEY_CAPSLOCK, U, 0}},
         {{Keys::KEY_LEFTCTRL, D}, {Keys::KEY_C, D}, {Keys::KEY_C, U}, {Keys::KEY_LEFTCTRL, U}}},
        {"space layer: h -> left",
         {{Keys::KEY_SPACE, D, 400},
          {Keys::KEY_H, D, 30},
          {Keys::KEY_H, U, 30},
          {Keys::KEY_SPACE, U, 0}},
         {{Keys::KEY_LEFT, D}, {Keys::KEY_LEFT, U}}},
        {"auto-repeat re-sends held key",
         {{Keys::KEY_A, D, 500}, {Keys::KEY_A, D, 40}, {Keys::KEY_A, D, 40}, {Keys::KEY_A, U, 0}},
         {{Keys::KEY_A, D}, {Keys::KEY_A, D}, {Keys::KEY_A, D}, {Keys::KEY_A, U}}},
    };
}

std::string readFile(const std::wstring& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int runTests(const std::wstring& tff_exe, const std::wstring& config) {
    // Calibration: do LL hooks see SendInput at all on this machine (interactive desktop)?
    if (!type({Keys::KEY_A, false, 20}) || !type({Keys::KEY_A, true, 200})) {
        std::cout << "SKIP: SendInput failed (error " << GetLastError()
                  << "), no interactive desktop\n";
        return kSkip;
    }
    if (g_harness_seen.load() < 2) {
        std::cout << "SKIP: the keyboard hook did not see injected input (no interactive "
                     "desktop?)\n";
        return kSkip;
    }
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_leaked.clear();
    }

    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    const std::wstring log_path = std::wstring(tmp) + L"tff_e2e_tff.log";
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE log = CreateFileW(log_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = log;
    si.hStdError = log;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + tff_exe + L"\" --verbose -c \"" + config + L"\"";
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        std::cout << "FAIL: cannot start tff.exe (error " << GetLastError() << ")\n";
        return 1;
    }
    CloseHandle(log);

    bool ready = false;
    for (int i = 0; i < 100 && !ready; ++i) {
        Sleep(100);
        ready = readFile(log_path).find("Keyboard hook installed") != std::string::npos;
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
            break;
        }
    }
    int failures = 0;
    if (!ready) {
        std::cout << "FAIL: tff.exe did not install its hook. Output:\n"
                  << readFile(log_path) << "\n";
        failures = 1;
    } else {
        Sleep(200);
        for (const auto& sc : scenarios()) {
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_output.clear();
                g_leaked.clear();
            }
            bool sent = true;
            for (const auto& step : sc.steps) {
                sent = type(step) && sent;
            }
            Sleep(500);  // let pending timers fire
            std::vector<Recorded> got;
            std::vector<Recorded> leaked;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                got = g_output;
                leaked = g_leaked;
            }
            const bool ok = sent && got == sc.expected && leaked.empty();
            std::cout << (ok ? "PASS" : "FAIL") << ": " << sc.name << "\n";
            if (!ok) {
                ++failures;
                std::cout << "  expected: " << describe(sc.expected) << "\n"
                          << "  got:      " << describe(got) << "\n";
                if (!leaked.empty()) {
                    std::cout << "  not swallowed by tff: " << describe(leaked) << "\n";
                }
            }
        }
    }

    TerminateProcess(pi.hProcess, 0);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (failures > 0) {
        std::cout << "--- tff.exe output ---\n" << readFile(log_path) << "\n";
    }
    DeleteFileW(log_path.c_str());
    return failures == 0 ? 0 : 1;
}

}  // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: tff_e2e <tff.exe> <config.yaml>\n";
        return 2;
    }
    timeBeginPeriod(1);  // 1 ms Sleep() resolution for the chord timings

    HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, sinkHook, GetModuleHandleW(nullptr), 0);
    if (hook == nullptr) {
        std::cout << "SKIP: SetWindowsHookEx failed (error " << GetLastError() << ")\n";
        return kSkip;
    }

    // The hook runs on this thread, so this thread pumps messages while a worker types.
    std::atomic<int> rc{1};
    std::thread worker([&] { rc = runTests(argv[1], argv[2]); });
    HANDLE wh = worker.native_handle();
    for (;;) {
        DWORD r = MsgWaitForMultipleObjects(1, &wh, FALSE, INFINITE, QS_ALLINPUT);
        if (r == WAIT_OBJECT_0) {
            break;
        }
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            DispatchMessageW(&msg);
        }
    }
    worker.join();
    UnhookWindowsHookEx(hook);
    timeEndPeriod(1);
    std::cout << (rc == 0 ? "E2E PASSED" : (rc == kSkip ? "E2E SKIPPED" : "E2E FAILED")) << "\n";
    return rc;
}
