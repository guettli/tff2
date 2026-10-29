#include "windows_platform.h"
#include "tff_key_codes.h"
#include "tff_parser.h"
#include <wtsapi32.h>
#include <timeapi.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

WindowsPlatform* WindowsPlatform::active_ = nullptr;

namespace {

constexpr UINT WM_TFF_STOP = WM_APP + 1;
constexpr UINT WM_TFF_RELOAD = WM_APP + 2;
constexpr UINT WM_TFF_PAUSE = WM_APP + 3;
constexpr UINT WM_TFF_RESUME = WM_APP + 4;

/// Raw keyboard input without a hook callback for this long means the hook is gone.
constexpr ULONGLONG kHookDeadAfterMs = 1000;

/// An engine timer that is still due right after firing (the engine didn't advance it) is
/// retried after this delay instead of in a busy loop.
constexpr int64_t kStuckTimerRetryUs = 10 * 1000;

/// Editors save in several steps (truncate, write, rename): reload once the file is quiet.
constexpr ULONGLONG kConfigReloadDelayMs = 150;

const wchar_t kMessageWindowClass[] = L"TffMessageWindow";

int64_t fileMtime(const std::string& path) {
    std::error_code ec;
    auto t = std::filesystem::last_write_time(std::filesystem::u8path(path), ec);
    if (ec) {
        return 0;
    }
    return static_cast<int64_t>(t.time_since_epoch().count());
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

uint32_t toMs(const tff::TimeVal& t) {
    return static_cast<uint32_t>(t.sec * 1000 + t.usec / 1000);
}

DWORD mouseButtonFlags(tff::KeyCode button, bool up, DWORD& data) {
    data = 0;
    switch (button) {
        case tff::Keys::BTN_LEFT:
            return up ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
        case tff::Keys::BTN_RIGHT:
            return up ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
        case tff::Keys::BTN_MIDDLE:
            return up ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
        case tff::Keys::BTN_SIDE:
            data = XBUTTON1;
            return up ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN;
        case tff::Keys::BTN_EXTRA:
            data = XBUTTON2;
            return up ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN;
        default:
            return 0;
    }
}

}  // namespace

WindowsPlatform::WindowsPlatform() {
    QueryPerformanceFrequency(&qpc_freq_);
    QueryPerformanceCounter(&qpc_start_);
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    // FILETIME counts 100ns intervals since 1601-01-01
    epoch_start_us_ = static_cast<int64_t>((u.QuadPart - 116444736000000000ULL) / 10ULL);
}

WindowsPlatform::~WindowsPlatform() {
    removeHook();
    teardownConfigWatch();
    destroyMessageWindow();
}

tff::TimeVal WindowsPlatform::now() const {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    const int64_t ticks = c.QuadPart - qpc_start_.QuadPart;
    const int64_t freq = qpc_freq_.QuadPart;
    const int64_t us = (ticks / freq) * 1000000LL + ((ticks % freq) * 1000000LL) / freq;
    return tff::TimeVal::fromMicros(epoch_start_us_ + us);
}

bool WindowsPlatform::initialize() {
    if (initialized_) {
        return true;
    }
    output_.setSender(
        [this](const std::vector<tff::win::OutputInput>& inputs) { return sendInputs(inputs); });
    output_.setObserver([this](const tff::Event& ev) {
        if (ev.type != tff::EV_KEY || ev.value == tff::KEY_VAL_REPEAT) {
            return;
        }
        debug_buffer_.recordOutKeyEvent(toMs(ev.time), ev.code, ev.value == tff::KEY_VAL_DOWN);
        if (verbose_ && !monitor_mode_) {
            log("[TFF Out] " + tff::keyCodeToWord(ev.code) + " (" + std::to_string(ev.code) + ") " +
                (ev.value == tff::KEY_VAL_DOWN ? "DOWN" : "UP"));
        }
    });
    engine_ = std::make_unique<tff::TFFEngine>(&output_);
    engine_->setLayerToggleCallback([this](const std::string& layer_name, bool active) {
        if (layer_callback_) {
            layer_callback_(layer_name, active);
        }
    });
    if (monitor_) {
        monitor_->attachToEngine(*engine_);
    }
    initialized_ = true;
    return true;
}

tff::TFFEngine& WindowsPlatform::getEngine() {
    initialize();
    return *engine_;
}

void WindowsPlatform::setMonitorMode(const tff::MonitorOptions& options) {
    monitor_mode_ = true;
    monitor_ = std::make_unique<tff::EventMonitor>(options);
    if (engine_) {
        monitor_->attachToEngine(*engine_);
    }
}

void WindowsPlatform::setConfig(const tff::Config& config) {
    initialize();
    engine_->setCombos(config.combos);
    engine_->setTapHoldKeys(config.tap_hold_keys);
    engine_->setTapDances(config.tap_dances);
    engine_->setOneShotKeys(config.one_shot_keys);
    engine_->setLayers(config.layers);
    engine_->setLeaderConfig(config.leader);
    engine_->setAutoShiftConfig(config.auto_shift);
    engine_->setMouseConfig(config.mouse);
    engine_->setSettings(config.settings);
    settings_ = config.settings;
}

bool WindowsPlatform::loadConfiguration(const std::string& config_file, std::string& err_msg) {
    initialize();
    std::string content;
    if (!readFile(config_file, content)) {
        err_msg = "Failed to open config file: " + config_file;
        return false;
    }
    tff::Config config;
    std::string parse_err;
    if (!tff::loadYamlConfig(content, config, parse_err)) {
        err_msg = "Failed to parse YAML config (" + config_file + "): " + parse_err;
        return false;
    }
    std::error_code ec;
    auto abs = std::filesystem::absolute(std::filesystem::u8path(config_file), ec);
    config_file_ = ec ? config_file : abs.u8string();
    config_mtime_ = fileMtime(config_file_);
    setConfig(config);
    return true;
}

bool WindowsPlatform::reloadConfiguration(std::string& err_msg) {
    if (config_file_.empty()) {
        err_msg = "cannot reload configuration, no config file specified";
        return false;
    }
    const std::string path = config_file_;
    if (!loadConfiguration(path, err_msg)) {
        err_msg += "\nKeeping current configuration";
        return false;
    }
    return true;
}

void WindowsPlatform::log(const std::string& line) {
    if (log_sink_) {
        log_sink_(line);
    }
}

// ---------------------------------------------------------------------------
// Hook
// ---------------------------------------------------------------------------

LRESULT CALLBACK WindowsPlatform::hookProc(int code, WPARAM wparam, LPARAM lparam) {
    if (code == HC_ACTION && active_ != nullptr) {
        const auto* k = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lparam);
        tff::win::HookEvent ev;
        ev.vk = k->vkCode;
        ev.scan = k->scanCode;
        ev.flags = k->flags;
        ev.extra_info = static_cast<uintptr_t>(k->dwExtraInfo);
        if (active_->handleHookEvent(ev)) {
            return 1;
        }
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

bool WindowsPlatform::installHook() {
    active_ = this;
    hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, hookProc, GetModuleHandleW(nullptr), 0);
    if (hook_ == nullptr) {
        active_ = nullptr;
        log("[TFF] SetWindowsHookEx(WH_KEYBOARD_LL) failed, error " +
            std::to_string(GetLastError()));
        return false;
    }
    last_hook_tick_ = GetTickCount64();
    return true;
}

void WindowsPlatform::removeHook() {
    if (hook_ != nullptr) {
        UnhookWindowsHookEx(hook_);
        hook_ = nullptr;
    }
    if (active_ == this) {
        active_ = nullptr;
    }
}

bool WindowsPlatform::handleHookEvent(const tff::win::HookEvent& ev) {
    initialize();
    last_hook_tick_ = GetTickCount64();
    if (ev.extra_info == tff::win::kInjectedMagic) {
        // Our own output. SendInput may deliver it to this hook re-entrantly, while the
        // outer call is still sending: touch nothing (no timers, no flush).
        return false;
    }
    if (paused_.load()) {
        translator_.observePassThrough(ev);
        return false;
    }
    if (injecting_) {
        // Another key arrived while we are inside SendInput (engine output in progress).
        // Decide now, but feed the engine only after the outer call has finished.
        std::vector<tff::Event> events;
        const auto decision = translator_.translate(ev, now(), events);
        if (decision == tff::win::HookDecision::PassThrough) {
            return false;
        }
        deferred_.insert(deferred_.end(), events.begin(), events.end());
        return !monitor_mode_;
    }
    // Timers that are already due come first: the message loop may not have woken up yet,
    // and a tap-hold or combo decision depends on whether its deadline passed before this key.
    handleTimers();
    const auto decision = translator_.translate(ev, now(), scratch_events_);
    if (decision == tff::win::HookDecision::PassThrough) {
        return false;
    }
    if (decision == tff::win::HookDecision::Drop) {
        return !monitor_mode_;
    }
    for (const auto& e : scratch_events_) {
        processEngineEvent(e);
    }
    output_.flush();
    processDeferred();
    return !monitor_mode_;
}

void WindowsPlatform::processDeferred() {
    if (injecting_) {
        return;  // only from the top level, never from inside engine output
    }
    for (int round = 0; round < 8 && !deferred_.empty(); ++round) {
        std::vector<tff::Event> events;
        events.swap(deferred_);
        for (const auto& e : events) {
            processEngineEvent(e);
        }
        output_.flush();
    }
}

void WindowsPlatform::processEngineEvent(const tff::Event& ev) {
    if (monitor_mode_) {
        monitor_->clearTrace();
        engine_->processEvent(ev);
        log(monitor_->formatEvent(ev));
        return;
    }

    // Diagnostic dump hotkey (F12 or Pause/Break), same as on Linux: never reaches the engine.
    if (tff::isDiagnosticDumpHotkey(ev.code)) {
        if (ev.value == tff::KEY_VAL_DOWN && !dump_hotkey_latched_) {
            dump_hotkey_latched_ = true;
            triggerDebugDump();
        } else if (ev.value == tff::KEY_VAL_UP) {
            dump_hotkey_latched_ = false;
        }
        return;
    }

    if (ev.value != tff::KEY_VAL_REPEAT) {
        debug_buffer_.recordInKeyEvent(toMs(ev.time), ev.code, ev.value == tff::KEY_VAL_DOWN);
        if (verbose_) {
            log("[TFF In]  " + tff::keyCodeToWord(ev.code) + " (" + std::to_string(ev.code) + ") " +
                (ev.value == tff::KEY_VAL_DOWN ? "DOWN" : "UP"));
        }
    }
    if (ev.value == tff::KEY_VAL_DOWN) {
        output_.onPhysicalDown();
    }
    engine_->processEvent(ev);
    if (ev.value == tff::KEY_VAL_REPEAT) {
        output_.onPhysicalRepeat();
    }
}

void WindowsPlatform::snapshotHeldKeys() {
    // Keys Windows already sees as held went down before our hook existed (TFF starting,
    // hook reinstalled). Their repeats and key-ups must reach Windows, not the engine.
    const bool altgr_down = (GetAsyncKeyState(VK_RMENU) & 0x8000) != 0;
    for (const auto& m : tff::win::keyMappings()) {
        if (m.vk == 0 || (GetAsyncKeyState(m.vk) & 0x8000) == 0) {
            continue;
        }
        if (m.code == tff::Keys::KEY_KPENTER) {
            continue;  // shares VK_RETURN with Enter; GetAsyncKeyState can't tell them apart
        }
        if (m.code == tff::Keys::KEY_LEFTCTRL && altgr_down) {
            continue;  // the fake LCtrl of AltGr, released through the AltGr path
        }
        translator_.markForeignDown(m.code);
    }
}

void WindowsPlatform::handleTimers() {
    initialize();
    for (int i = 0; i < 16 && engine_->hasActiveTimer(); ++i) {
        const tff::TimeVal t = now();
        const tff::TimeVal due = engine_->getActiveTimerTime();
        if (due > t) {
            break;
        }
        if (due == stuck_timer_ && tff::timeSubMicros(stuck_since_, t) < kStuckTimerRetryUs) {
            break;  // fired already without effect; don't spin on it
        }
        if (monitor_mode_) {
            monitor_->clearTrace();
        }
        engine_->onTimer(t);
        if (engine_->hasActiveTimer() && engine_->getActiveTimerTime() == due) {
            stuck_timer_ = due;  // the engine left this timer due: retry later
            stuck_since_ = t;
            break;
        }
        if (monitor_mode_) {
            std::string line = monitor_->formatTimer(t);
            if (!line.empty()) {
                log(line);
            }
        }
    }
    output_.flush();
    processDeferred();
}

void WindowsPlatform::resetState(const char* reason) {
    initialize();
    // The engine releases what it holds itself (tap-hold modifiers, layer remaps, ...);
    // releaseAll() then lifts whatever is still down, so no key gets a second key-up.
    deferred_.clear();
    engine_->reset();
    output_.releaseAll();
    translator_.reset();
    if (verbose_) {
        log(std::string("[TFF] State reset (") + reason + ")");
    }
}

bool WindowsPlatform::sendInputs(const std::vector<tff::win::OutputInput>& inputs) {
    if (monitor_mode_ || inputs.empty()) {
        return true;
    }
    scratch_inputs_.clear();
    for (const auto& in : inputs) {
        INPUT x{};
        switch (in.type) {
            case tff::win::OutputInput::Type::Key: {
                x.type = INPUT_KEYBOARD;
                DWORD flags = in.key.extended ? KEYEVENTF_EXTENDEDKEY : 0;
                if (in.key.output_vk) {
                    x.ki.wVk = in.key.vk;
                } else {
                    // Scan codes keep the output independent of the active keyboard layout,
                    // like evdev on Linux: physical key positions.
                    flags |= KEYEVENTF_SCANCODE;
                }
                x.ki.wScan = in.key.scan;
                if (in.up) {
                    flags |= KEYEVENTF_KEYUP;
                }
                x.ki.dwFlags = flags;
                x.ki.dwExtraInfo = tff::win::kInjectedMagic;
                break;
            }
            case tff::win::OutputInput::Type::MouseMove:
                x.type = INPUT_MOUSE;
                x.mi.dx = in.dx;
                x.mi.dy = in.dy;
                x.mi.dwFlags = MOUSEEVENTF_MOVE;
                x.mi.dwExtraInfo = tff::win::kInjectedMagic;
                break;
            case tff::win::OutputInput::Type::MouseButton: {
                DWORD data = 0;
                x.type = INPUT_MOUSE;
                x.mi.dwFlags = mouseButtonFlags(in.button, in.up, data);
                x.mi.mouseData = data;
                x.mi.dwExtraInfo = tff::win::kInjectedMagic;
                if (x.mi.dwFlags == 0) {
                    continue;
                }
                break;
            }
            case tff::win::OutputInput::Type::Wheel:
            case tff::win::OutputInput::Type::HWheel:
                x.type = INPUT_MOUSE;
                x.mi.dwFlags = in.type == tff::win::OutputInput::Type::Wheel ? MOUSEEVENTF_WHEEL
                                                                             : MOUSEEVENTF_HWHEEL;
                x.mi.mouseData = static_cast<DWORD>(in.dx * WHEEL_DELTA);
                x.mi.dwExtraInfo = tff::win::kInjectedMagic;
                break;
        }
        scratch_inputs_.push_back(x);
    }
    if (scratch_inputs_.empty()) {
        return true;
    }
    const UINT n = static_cast<UINT>(scratch_inputs_.size());
    const bool was_injecting = injecting_;
    injecting_ = true;
    const UINT sent = send_input_ ? send_input_(n, scratch_inputs_.data())
                                  : SendInput(n, scratch_inputs_.data(), sizeof(INPUT));
    injecting_ = was_injecting;
    if (sent != n) {
        // Typical cause: UIPI. The foreground window runs elevated and TFF does not.
        log("[TFF] SendInput injected " + std::to_string(sent) + " of " + std::to_string(n) +
            " event(s), error " + std::to_string(GetLastError()) +
            " (is the active window running as administrator?)");
        return false;
    }
    return true;
}

void WindowsPlatform::triggerDebugDump() {
    if (is_dumping_) {
        return;
    }
    is_dumping_ = true;
    debug_buffer_.setPaused(true);
    const std::string dump = debug_buffer_.formatDump(toMs(now()));
    log("[TFF Dump]\n" + dump);
    const tff::TimeVal t = now();
    const std::string layout = engine_ ? engine_->getSettings().layout : "us";
    tff::emitKeyStrokesForText(
        dump,
        [this, t](tff::KeyCode code, bool shift) {
            if (shift) {
                output_.writeOne(
                    tff::Event{t, tff::EV_KEY, tff::Keys::KEY_LEFTSHIFT, tff::KEY_VAL_DOWN});
            }
            output_.writeOne(tff::Event{t, tff::EV_KEY, code, tff::KEY_VAL_DOWN});
            output_.writeOne(tff::Event{t, tff::EV_KEY, code, tff::KEY_VAL_UP});
            if (shift) {
                output_.writeOne(
                    tff::Event{t, tff::EV_KEY, tff::Keys::KEY_LEFTSHIFT, tff::KEY_VAL_UP});
            }
        },
        layout);
    output_.flush();
    debug_buffer_.setPaused(false);
    is_dumping_ = false;
}

// ---------------------------------------------------------------------------
// Message window: session lock notifications and the raw-input hook watchdog
// ---------------------------------------------------------------------------

LRESULT CALLBACK WindowsPlatform::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<WindowsPlatform*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self != nullptr) {
        if (msg == WM_WTSSESSION_CHANGE) {
            if (wparam == WTS_SESSION_LOCK || wparam == WTS_SESSION_UNLOCK) {
                // Key-ups on the lock screen never reach the hook.
                self->resetState(wparam == WTS_SESSION_LOCK ? "session locked"
                                                            : "session unlocked");
            }
            return 0;
        }
        if (msg == WM_INPUT) {
            RAWINPUTHEADER header;
            UINT size = sizeof(header);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_HEADER, &header, &size,
                                sizeof(RAWINPUTHEADER)) == sizeof(RAWINPUTHEADER) &&
                header.dwType == RIM_TYPEKEYBOARD && header.hDevice != nullptr) {
                // A physical key event. The hook sees input before raw input does, so a
                // hook that hasn't been called for a while has been removed by Windows.
                self->raw_input_tick_ = GetTickCount64();
                self->checkHookAlive();
            }
        }
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void WindowsPlatform::checkHookAlive() {
    if (hook_ == nullptr || paused_.load() || raw_input_tick_ == 0) {
        return;
    }
    if (raw_input_tick_ > last_hook_tick_ + kHookDeadAfterMs) {
        log("[TFF] Keyboard hook was removed by Windows (callback too slow?), reinstalling");
        removeHook();
        resetState("hook reinstalled");
        installHook();
        // Keys pressed while the hook was gone went to Windows. Take the snapshot after the
        // key-ups injected by resetState() above have been processed (next loop iteration).
        snapshot_pending_ = true;
    }
    raw_input_tick_ = 0;
}

void WindowsPlatform::createMessageWindow() {
    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = windowProc;
    wc.hInstance = inst;
    wc.lpszClassName = kMessageWindowClass;
    RegisterClassExW(&wc);  // fails harmlessly if already registered
    msg_window_ = CreateWindowExW(0, kMessageWindowClass, L"TFF", 0, 0, 0, 0, 0, HWND_MESSAGE,
                                  nullptr, inst, nullptr);
    if (msg_window_ == nullptr) {
        return;
    }
    SetWindowLongPtrW(msg_window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    WTSRegisterSessionNotification(msg_window_, NOTIFY_FOR_THIS_SESSION);
    if (!monitor_mode_) {
        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;  // generic desktop
        rid.usUsage = 0x06;      // keyboard
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = msg_window_;
        RegisterRawInputDevices(&rid, 1, sizeof(rid));
    }
}

void WindowsPlatform::destroyMessageWindow() {
    if (msg_window_ != nullptr) {
        WTSUnRegisterSessionNotification(msg_window_);
        DestroyWindow(msg_window_);
        msg_window_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Config watch
// ---------------------------------------------------------------------------

void WindowsPlatform::setupConfigWatch() {
    if (config_file_.empty()) {
        return;
    }
    auto dir = std::filesystem::u8path(config_file_).parent_path();
    config_change_ = FindFirstChangeNotificationW(
        dir.wstring().c_str(), FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE);
    if (config_change_ == INVALID_HANDLE_VALUE) {
        log("[TFF] Cannot watch config directory " + dir.u8string() + ", error " +
            std::to_string(GetLastError()));
    } else if (verbose_) {
        log("[TFF] Watching " + config_file_ + " for changes");
    }
}

void WindowsPlatform::teardownConfigWatch() {
    if (config_change_ != INVALID_HANDLE_VALUE) {
        FindCloseChangeNotification(config_change_);
        config_change_ = INVALID_HANDLE_VALUE;
    }
}

void WindowsPlatform::onConfigDirChanged() {
    FindNextChangeNotification(config_change_);
    const int64_t mtime = fileMtime(config_file_);
    if (mtime == 0 || mtime == config_mtime_) {
        return;  // another file in the directory changed, or the file is mid-replace
    }
    // Reload from the message loop once the file has been quiet for a moment; never sleep
    // here, this thread also runs the keyboard hook.
    reload_due_tick_ = GetTickCount64() + kConfigReloadDelayMs;
}

void WindowsPlatform::reloadNow() {
    reload_due_tick_ = 0;
    const int64_t mtime = fileMtime(config_file_);
    std::string err;
    const bool ok = reloadConfiguration(err);
    const std::string msg =
        ok ? "Configuration reloaded from " + config_file_ : "Error reloading config: " + err;
    log("[TFF Config] " + msg);
    if (reload_callback_) {
        reload_callback_(ok, msg);
    }
    if (!ok) {
        config_mtime_ = mtime;  // don't retry until the file changes again
    }
}

// ---------------------------------------------------------------------------
// Message loop
// ---------------------------------------------------------------------------

int WindowsPlatform::run() {
    initialize();
    MSG msg;
    // Make sure the thread has a message queue before anyone posts to it.
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    thread_id_ = GetCurrentThreadId();
    // Keystrokes must never wait behind other work.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // 1 ms timer resolution for combo / tap-hold deadlines (default is 15.6 ms). Windows 11
    // ignores it for processes without visible windows (the tray app) unless they opt out.
#ifdef PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
    PROCESS_POWER_THROTTLING_STATE throttling{};
    throttling.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    throttling.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
    throttling.StateMask = 0;
    SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &throttling,
                          sizeof(throttling));
#endif
    timeBeginPeriod(1);
    createMessageWindow();
    snapshotHeldKeys();
    if (!installHook()) {
        timeEndPeriod(1);
        destroyMessageWindow();
        thread_id_ = 0;
        return 1;
    }
    if (watch_config_) {
        setupConfigWatch();
    }
    running_ = true;
    log("[TFF] Keyboard hook installed");

    while (!stop_requested_.load()) {
        DWORD timeout = INFINITE;
        if (engine_->hasActiveTimer()) {
            const int64_t diff_us = tff::timeSubMicros(now(), engine_->getActiveTimerTime());
            // A due timer is handled below; still pump messages (hook callbacks) first.
            if (diff_us > 0) {
                timeout = static_cast<DWORD>(diff_us / 1000 + 1);
            } else if (engine_->getActiveTimerTime() == stuck_timer_) {
                timeout = static_cast<DWORD>(kStuckTimerRetryUs / 1000);
            } else {
                timeout = 0;  // due now: handled below, after pumping messages
            }
        }
        if (reload_due_tick_ != 0) {
            const ULONGLONG t = GetTickCount64();
            const DWORD wait = reload_due_tick_ > t ? static_cast<DWORD>(reload_due_tick_ - t) : 0;
            timeout = std::min(timeout, wait);
        }

        HANDLE handles[1];
        DWORD count = 0;
        if (config_change_ != INVALID_HANDLE_VALUE) {
            handles[count++] = config_change_;
        }
        const DWORD r = MsgWaitForMultipleObjectsEx(count, count ? handles : nullptr, timeout,
                                                    QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        if (count > 0 && r == WAIT_OBJECT_0) {
            onConfigDirChanged();
        }

        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT || msg.message == WM_TFF_STOP) {
                stop_requested_ = true;
                break;
            }
            if (msg.hwnd == nullptr) {
                if (msg.message == WM_TFF_RELOAD) {
                    std::string err;
                    const bool ok = reloadConfiguration(err);
                    const std::string text = ok ? "Configuration reloaded from " + config_file_
                                                : "Error reloading config: " + err;
                    log("[TFF Config] " + text);
                    if (reload_callback_) {
                        reload_callback_(ok, text);
                    }
                } else if (msg.message == WM_TFF_PAUSE) {
                    resetState("paused");
                }
                continue;
            }
            DispatchMessageW(&msg);
        }
        handleTimers();
        if (snapshot_pending_) {
            snapshot_pending_ = false;
            snapshotHeldKeys();
        }
        if (reload_due_tick_ != 0 && GetTickCount64() >= reload_due_tick_) {
            reloadNow();
        }
    }

    running_ = false;
    resetState("shutdown");
    removeHook();
    teardownConfigWatch();
    destroyMessageWindow();
    timeEndPeriod(1);
    thread_id_ = 0;
    log("[TFF] Keyboard hook removed");
    return 0;
}

void WindowsPlatform::requestStop() {
    stop_requested_ = true;
    const DWORD tid = thread_id_.load();
    if (tid != 0) {
        PostThreadMessageW(tid, WM_TFF_STOP, 0, 0);
    }
}

void WindowsPlatform::requestReload() {
    const DWORD tid = thread_id_.load();
    if (tid != 0) {
        PostThreadMessageW(tid, WM_TFF_RELOAD, 0, 0);
    }
}

void WindowsPlatform::setPaused(bool paused) {
    if (paused_.exchange(paused) == paused) {
        return;
    }
    const DWORD tid = thread_id_.load();
    if (tid != 0) {
        PostThreadMessageW(tid, paused ? WM_TFF_PAUSE : WM_TFF_RESUME, 0, 0);
    }
}
