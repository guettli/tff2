#ifndef TFF_WIN_UTIL_H
#define TFF_WIN_UTIL_H

// Helpers shared by tff.exe (console) and tffw.exe (tray): strings, paths, logging, autostart.

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace tff {
namespace win {

std::wstring widen(const std::string& utf8);
std::string narrow(const std::wstring& wide);

/// Directory containing the running executable.
std::string exeDir();
/// %APPDATA%\tff (roaming user configuration).
std::string userConfigDir();
/// %APPDATA%\tff\tff-combos.yaml
std::string userConfigPath();
/// %LOCALAPPDATA%\tff (logs, machine-local state); created if missing.
std::string localDataDir();

/**
 * @brief Finds the config file: config\tff-combos.yaml next to the working directory (dev
 * checkout), %APPDATA%\tff\tff-combos.yaml, then the copy shipped next to the executable.
 * @return Empty string if none exists.
 */
std::string resolveConfigFile();

/// Copies the default config shipped next to the executable to userConfigPath() if missing.
bool ensureUserConfig(std::string& path, std::string& err_msg);

bool readTextFile(const std::string& path, std::string& out);

/// Enables UTF-8 output and ANSI escape sequences on the console.
/// @return true if stdout is a console that renders colors (and NO_COLOR is unset).
bool setupConsole();

/// Writes log lines on a background thread, so the keyboard hook never blocks on I/O.
class AsyncLog {
public:
    explicit AsyncLog(std::function<void(const std::string&)> writer);
    ~AsyncLog();
    AsyncLog(const AsyncLog&) = delete;
    AsyncLog& operator=(const AsyncLog&) = delete;

    void push(std::string line);

private:
    void loop();

    std::function<void(const std::string&)> writer_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::string> queue_;
    bool stop_ = false;
    std::thread thread_;
};

bool isElevated();

struct AutostartStatus {
    bool run_key = false;     ///< HKCU ...\Run entry present
    bool task = false;        ///< elevated logon task present
    std::string run_command;  ///< current Run entry value
};

/// Starts tffw.exe at sign-in. elevated=false: HKCU Run key. elevated=true: Task Scheduler
/// logon task with highest privileges (needs an elevated caller), so remapping also works in
/// windows running as administrator.
bool autostartEnable(bool elevated, std::string& err_msg);
bool autostartDisable(std::string& err_msg);
/// query_task=false skips the (slow) schtasks query and reports task=false.
AutostartStatus autostartStatus(bool query_task = true);

/// Name of the mutex that keeps two remapping instances (tff.exe / tffw.exe) from running.
extern const wchar_t* const kInstanceMutexName;

/// Creates the instance mutex. Sets already_running if another TFF owns it, including one
/// running elevated (then CreateMutex fails with ERROR_ACCESS_DENIED).
void* acquireInstanceMutex(bool& already_running);

}  // namespace win
}  // namespace tff

#endif  // TFF_WIN_UTIL_H
