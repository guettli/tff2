#ifndef WINDOWS_PLATFORM_H
#define WINDOWS_PLATFORM_H

#include "tff_engine.h"
#include "tff_monitor.h"
#include "debug_buffer.h"
#include "tff_parser.h"
#include "win_input.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace tff {
namespace win {
// Windows types text snippets as characters (KEYEVENTF_UNICODE), so they may contain any
// character, not only those on settings.layout.
inline LoadOptions configLoadOptions() {
    LoadOptions options;
    options.text_any_unicode = true;
    return options;
}
}  // namespace win
}  // namespace tff

/**
 * @brief Windows platform: remaps keys with a WH_KEYBOARD_LL hook and SendInput.
 *
 * All engine work happens on the thread that calls run(): it installs the hook, pumps
 * messages and fires engine timers via MsgWaitForMultipleObjectsEx. Other threads talk to it
 * with requestStop() / requestReload() / setPaused() (PostThreadMessage under the hood).
 *
 * The hook callback must return quickly (Windows silently removes slow hooks), so it never
 * writes to the console or to files: log lines go through a queue and are written by a
 * separate thread (see setLogSink()).
 */
class WindowsPlatform {
public:
    using LogSink = std::function<void(const std::string& line)>;
    using LayerCallback = std::function<void(const std::string& layer_name, bool active)>;
    using SendInputFn = std::function<UINT(UINT count, INPUT* inputs)>;

    WindowsPlatform();
    ~WindowsPlatform();
    WindowsPlatform(const WindowsPlatform&) = delete;
    WindowsPlatform& operator=(const WindowsPlatform&) = delete;

    bool initialize();

    /// Load a YAML config. Errors are returned in err_msg (and not printed).
    bool loadConfiguration(const std::string& config_file, std::string& err_msg);
    bool reloadConfiguration(std::string& err_msg);
    const std::string& getConfigFile() const { return config_file_; }

    void setConfig(const tff::Config& config);
    const tff::Settings& getSettings() const { return settings_; }
    tff::TFFEngine& getEngine();

    void setVerbose(bool verbose) { verbose_ = verbose; }
    void setWatchConfig(bool watch) { watch_config_ = watch; }
    void setIgnoreInjected(bool ignore) { translator_.setIgnoreInjected(ignore); }
    /// Observe-only mode for `tff monitor`: nothing is swallowed or injected.
    void setMonitorMode(const tff::MonitorOptions& options);

    /// Receives log/monitor lines (called from the engine thread; must not block for long).
    void setLogSink(LogSink sink) { log_sink_ = std::move(sink); }
    /// Called on the engine thread when a modal layer is toggled.
    void setLayerCallback(LayerCallback cb) { layer_callback_ = std::move(cb); }
    /// Called on the engine thread after a config reload (ok=false: old config kept).
    void setReloadCallback(std::function<void(bool ok, const std::string& msg)> cb) {
        reload_callback_ = std::move(cb);
    }
    /// Replace SendInput (tests).
    void setSendInputFn(SendInputFn fn) { send_input_ = std::move(fn); }

    /**
     * @brief Install the hook and run the message loop until requestStop().
     * @return 0 on clean shutdown, non-zero if the hook could not be installed.
     */
    int run();
    void requestStop();
    void requestReload();
    void setPaused(bool paused);
    bool isPaused() const { return paused_.load(); }
    bool isRunning() const { return running_.load(); }

    /**
     * @brief Process one low-level hook event (what the hook callback does).
     * @return true if the event is swallowed.
     */
    bool handleHookEvent(const tff::win::HookEvent& ev);
    /// Fire due engine timers (what the message loop does on timeout).
    void handleTimers();
    /// Release every injected key and reset all engine state (lock screen, pause, quit).
    void resetState(const char* reason);

    /// Monotonic time in the engine's TimeVal format, anchored to the wall clock at startup.
    tff::TimeVal now() const;

    tff::DebugBuffer& getDebugBuffer() { return debug_buffer_; }
    const tff::win::OutputBuilder& getOutput() const { return output_; }

private:
    static LRESULT CALLBACK hookProc(int code, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    bool installHook();
    void removeHook();
    void createMessageWindow();
    void destroyMessageWindow();
    void setupConfigWatch();
    void teardownConfigWatch();
    void onConfigDirChanged();
    void checkHookAlive();
    bool sendInputs(const std::vector<tff::win::OutputInput>& inputs);
    void snapshotHeldKeys();
    void processDeferred();
    void reloadNow();
    void processEngineEvent(const tff::Event& ev);
    void triggerDebugDump();
    void log(const std::string& line);

    static WindowsPlatform* active_;  ///< instance whose hook is installed

    bool initialized_ = false;
    bool verbose_ = false;
    bool watch_config_ = false;
    bool monitor_mode_ = false;
    std::atomic<bool> paused_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::atomic<DWORD> thread_id_{0};

    HHOOK hook_ = nullptr;
    HWND msg_window_ = nullptr;
    HANDLE config_change_ = INVALID_HANDLE_VALUE;
    int64_t config_mtime_ = 0;
    ULONGLONG reload_due_tick_ = 0;  ///< debounced config reload (0 = none pending)
    bool snapshot_pending_ = false;  ///< re-read held keys after a hook reinstall
    /// SendInput is running. Windows may call the hook re-entrantly meanwhile; such key events
    /// are translated right away but reach the engine only after the outer call (deferred_).
    bool injecting_ = false;
    std::vector<tff::Event> deferred_;
    tff::TimeVal stuck_timer_{-1, 0};  ///< engine timer that stayed due after firing
    tff::TimeVal stuck_since_{};

    // Hook watchdog: raw input proves a key was pressed; if the hook didn't see it, the hook
    // was removed (LowLevelHooksTimeout) and gets reinstalled.
    ULONGLONG last_hook_tick_ = 0;
    ULONGLONG raw_input_tick_ = 0;

    LARGE_INTEGER qpc_freq_{};
    LARGE_INTEGER qpc_start_{};
    int64_t epoch_start_us_ = 0;

    std::string config_file_;
    tff::Settings settings_;
    tff::win::InputTranslator translator_;
    tff::win::OutputBuilder output_;
    std::unique_ptr<tff::TFFEngine> engine_;
    std::unique_ptr<tff::EventMonitor> monitor_;
    std::vector<tff::Event> scratch_events_;
    std::vector<INPUT> scratch_inputs_;

    tff::DebugBuffer debug_buffer_;
    bool dump_hotkey_latched_ = false;
    bool is_dumping_ = false;

    LogSink log_sink_;
    LayerCallback layer_callback_;
    std::function<void(bool, const std::string&)> reload_callback_;
    SendInputFn send_input_;
};

#endif  // WINDOWS_PLATFORM_H
