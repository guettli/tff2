#include "win_util.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <io.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace tff {
namespace win {

const wchar_t* const kInstanceMutexName = L"Local\\TFF2-Remapper";

namespace {

const wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t kRunValue[] = L"TFF2";
const wchar_t kTaskName[] = L"TFF2";

std::string knownFolder(REFKNOWNFOLDERID id) {
    PWSTR path = nullptr;
    std::string result;
    if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &path))) {
        result = narrow(path);
    }
    CoTaskMemFree(path);
    return result;
}

bool isFile(const std::string& path) {
    std::error_code ec;
    auto p = std::filesystem::u8path(path);
    return std::filesystem::exists(p, ec) && !std::filesystem::is_directory(p, ec);
}

/// Runs a command without a console window; returns its exit code (or -1).
int runHidden(const std::wstring& command_line) {
    std::wstring cmd = command_line;  // CreateProcessW may modify the buffer
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, 30000);
    DWORD code = static_cast<DWORD>(-1);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(code);
}

std::wstring xmlEscape(const std::wstring& s) {
    std::wstring out;
    for (wchar_t c : s) {
        switch (c) {
            case L'&':
                out += L"&amp;";
                break;
            case L'<':
                out += L"&lt;";
                break;
            case L'>':
                out += L"&gt;";
                break;
            default:
                out += c;
        }
    }
    return out;
}

std::wstring trayExePath() {
    return widen(exeDir()) + L"\\tffw.exe";
}

}  // namespace

std::wstring widen(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
    return out;
}

std::string narrow(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0,
                                nullptr, nullptr);
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), n,
                        nullptr, nullptr);
    return out;
}

std::string exeDir() {
    std::wstring buf(MAX_PATH, L'\0');
    for (;;) {
        DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (n < buf.size()) {
            buf.resize(n);
            break;
        }
        buf.resize(buf.size() * 2);
    }
    return std::filesystem::path(buf).parent_path().u8string();
}

std::string userConfigDir() {
    std::string appdata = knownFolder(FOLDERID_RoamingAppData);
    if (appdata.empty()) {
        const char* env = std::getenv("APPDATA");
        appdata = env != nullptr ? env : ".";
    }
    return appdata + "\\tff";
}

std::string userConfigPath() {
    return userConfigDir() + "\\tff-combos.yaml";
}

std::string localDataDir() {
    std::string local = knownFolder(FOLDERID_LocalAppData);
    if (local.empty()) {
        const char* env = std::getenv("LOCALAPPDATA");
        local = env != nullptr ? env : ".";
    }
    std::string dir = local + "\\tff";
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(dir), ec);
    return dir;
}

std::string resolveConfigFile() {
    const std::vector<std::string> candidates = {
        "config\\tff-combos.yaml",      "..\\config\\tff-combos.yaml",          userConfigPath(),
        exeDir() + "\\tff-combos.yaml", exeDir() + "\\config\\tff-combos.yaml",
    };
    for (const auto& path : candidates) {
        if (isFile(path)) {
            return path;
        }
    }
    return "";
}

bool ensureUserConfig(std::string& path, std::string& err_msg) {
    path = userConfigPath();
    if (isFile(path)) {
        return true;
    }
    const std::string bundled = exeDir() + "\\tff-combos.yaml";
    if (!isFile(bundled)) {
        err_msg =
            "No configuration found and no default config next to the program (" + bundled + ")";
        return false;
    }
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(userConfigDir()), ec);
    std::filesystem::copy_file(std::filesystem::u8path(bundled), std::filesystem::u8path(path),
                               std::filesystem::copy_options::skip_existing, ec);
    if (ec) {
        err_msg = "Cannot create " + path + ": " + ec.message();
        return false;
    }
    return true;
}

bool readTextFile(const std::string& path, std::string& out) {
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

bool setupConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out == INVALID_HANDLE_VALUE || !GetConsoleMode(out, &mode)) {
        return false;  // redirected to a file or pipe
    }
    if (!SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return false;  // very old console host without ANSI support
    }
    const char* no_color = std::getenv("NO_COLOR");
    return no_color == nullptr || no_color[0] == '\0';
}

AsyncLog::AsyncLog(std::function<void(const std::string&)> writer)
    : writer_(std::move(writer)), thread_([this] { loop(); }) {}

AsyncLog::~AsyncLog() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_one();
    thread_.join();
}

void AsyncLog::push(std::string line) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() > 10000) {
            return;  // writer is stuck (e.g. console in QuickEdit selection); drop
        }
        queue_.push_back(std::move(line));
    }
    cv_.notify_one();
}

void AsyncLog::loop() {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
        cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
        while (!queue_.empty()) {
            std::string line = std::move(queue_.front());
            queue_.pop_front();
            lock.unlock();
            writer_(line);
            lock.lock();
        }
        if (stop_) {
            return;
        }
    }
}

bool isElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    const bool ok =
        GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) != 0;
    CloseHandle(token);
    return ok && elevation.TokenIsElevated != 0;
}

bool autostartEnable(bool elevated, std::string& err_msg) {
    const std::wstring exe = trayExePath();
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        err_msg = "tffw.exe not found next to this program: " + narrow(exe);
        return false;
    }
    if (!elevated) {
        const std::wstring value = L"\"" + exe + L"\"";
        LSTATUS st = RegSetKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, REG_SZ, value.c_str(),
                                     static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        if (st != ERROR_SUCCESS) {
            err_msg = "Cannot write HKCU\\...\\Run (error " + std::to_string(st) + ")";
            return false;
        }
        return true;
    }

    if (!isElevated()) {
        err_msg =
            "--elevated needs an administrator prompt (right-click the terminal, "
            "\"Run as administrator\")";
        return false;
    }
    // A logon task created from XML: the schtasks /SC ONLOGON defaults stop the task after
    // 72 hours and on battery power.
    wchar_t user[256] = L"";
    wchar_t domain[256] = L"";
    GetEnvironmentVariableW(L"USERNAME", user, 256);
    GetEnvironmentVariableW(L"USERDOMAIN", domain, 256);
    const std::wstring user_id = xmlEscape(std::wstring(domain) + L"\\" + user);
    const std::wstring xml =
        L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n"
        L"<Task version=\"1.2\" "
        L"xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
        L"  <RegistrationInfo><Description>Ten Flying Fingers keyboard remapper"
        L"</Description></RegistrationInfo>\n"
        L"  <Triggers><LogonTrigger><Enabled>true</Enabled><UserId>" +
        user_id +
        L"</UserId></LogonTrigger></Triggers>\n"
        L"  <Principals><Principal id=\"Author\"><UserId>" +
        user_id +
        L"</UserId><LogonType>InteractiveToken</LogonType>"
        L"<RunLevel>HighestAvailable</RunLevel></Principal></Principals>\n"
        L"  <Settings>\n"
        L"    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>\n"
        L"    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\n"
        L"    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\n"
        L"    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>\n"
        L"    <Priority>4</Priority>\n"
        L"    <IdleSettings><StopOnIdleEnd>false</StopOnIdleEnd></IdleSettings>\n"
        L"  </Settings>\n"
        L"  <Actions Context=\"Author\"><Exec><Command>" +
        xmlEscape(exe) +
        L"</Command></Exec></Actions>\n"
        L"</Task>\n";

    wchar_t tmp_dir[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp_dir);
    const std::wstring xml_path = std::wstring(tmp_dir) + L"tff2-task.xml";
    {
        std::ofstream f(std::filesystem::path(xml_path), std::ios::binary);
        const unsigned char bom[] = {0xFF, 0xFE};
        f.write(reinterpret_cast<const char*>(bom), sizeof(bom));
        f.write(reinterpret_cast<const char*>(xml.data()),
                static_cast<std::streamsize>(xml.size() * sizeof(wchar_t)));
    }
    const int rc = runHidden(L"schtasks.exe /Create /F /TN " + std::wstring(kTaskName) +
                             L" /XML \"" + xml_path + L"\"");
    DeleteFileW(xml_path.c_str());
    if (rc != 0) {
        err_msg = "schtasks /Create failed (exit code " + std::to_string(rc) + ")";
        return false;
    }
    // One autostart is enough: the task replaces the Run key.
    RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue);
    return true;
}

bool autostartDisable(std::string& err_msg) {
    LSTATUS st = RegDeleteKeyValueW(HKEY_CURRENT_USER, kRunKey, kRunValue);
    if (st != ERROR_SUCCESS && st != ERROR_FILE_NOT_FOUND) {
        err_msg = "Cannot remove HKCU\\...\\Run entry (error " + std::to_string(st) + ")";
        return false;
    }
    if (autostartStatus().task) {
        if (runHidden(L"schtasks.exe /Delete /F /TN " + std::wstring(kTaskName)) != 0) {
            err_msg = "Removing the elevated logon task needs an administrator prompt";
            return false;
        }
    }
    return true;
}

void* acquireInstanceMutex(bool& already_running) {
    HANDLE h = CreateMutexW(nullptr, TRUE, kInstanceMutexName);
    const DWORD err = GetLastError();
    already_running = (h != nullptr && err == ERROR_ALREADY_EXISTS) ||
                      (h == nullptr && err == ERROR_ACCESS_DENIED);
    return h;
}

AutostartStatus autostartStatus(bool query_task) {
    AutostartStatus s;
    wchar_t buf[1024];
    DWORD size = sizeof(buf);
    if (RegGetValueW(HKEY_CURRENT_USER, kRunKey, kRunValue, RRF_RT_REG_SZ, nullptr, buf, &size) ==
        ERROR_SUCCESS) {
        s.run_key = true;
        s.run_command = narrow(buf);
    }
    s.task = query_task && runHidden(L"schtasks.exe /Query /TN " + std::wstring(kTaskName)) == 0;
    return s;
}

}  // namespace win
}  // namespace tff
