// tffw.exe: TFF remapper with a tray icon (no console window).
//
// Two threads: this UI thread owns the tray icon, menu and on-screen layer indicator; the
// engine thread runs WindowsPlatform::run() (hook + timers). The tray menu runs a modal loop
// while open, which would stall engine timers if both shared a thread.
#include "windows_platform.h"
#include "win_util.h"
#include "tff_cheatsheet.h"
#include "tff_parser.h"
#include "tff_resource.h"
#include "tff_version.h"
#include <shellapi.h>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

namespace {

constexpr UINT WM_TFF_TRAY = WM_APP + 10;
constexpr UINT WM_TFF_LAYER = WM_APP + 11;     // wparam: active, lparam: std::wstring* name
constexpr UINT WM_TFF_RELOADED = WM_APP + 12;  // wparam: ok, lparam: std::wstring* message
constexpr UINT WM_TFF_ENGINE_EXIT = WM_APP + 13;
constexpr UINT kTrayId = 1;
constexpr UINT_PTR kOsdTimer = 1;
constexpr UINT_PTR kTrayRetryTimer = 2;
constexpr UINT kTrayRetryMs = 2000;
constexpr int kTrayRetries = 90;  // 3 minutes: the taskbar may come up late after sign-in
constexpr UINT kOsdMs = 1200;

enum MenuId : UINT {
    ID_PAUSE = 100,
    ID_RELOAD,
    ID_EDIT_CONFIG,
    ID_OPEN_CONFIG_DIR,
    ID_CHEATSHEET,
    ID_NOTIFY_OFF,
    ID_NOTIFY_OSD,
    ID_NOTIFY_TOAST,
    ID_AUTOSTART,
    ID_OPEN_LOG,
    ID_QUIT,
};

enum class NotifyMode : DWORD { Off = 0, Osd = 1, Toast = 2 };

const wchar_t kSettingsKey[] = L"Software\\TFF2";
const wchar_t kTrayClass[] = L"TffTrayWindow";
const wchar_t kOsdClass[] = L"TffOsdWindow";

struct App {
    HINSTANCE instance = nullptr;
    HWND tray_window = nullptr;
    HWND osd_window = nullptr;
    HICON icon_active = nullptr;
    HICON icon_paused = nullptr;
    UINT taskbar_created_msg = 0;
    int tray_retries_left = 0;
    NotifyMode notify_mode = NotifyMode::Osd;
    std::wstring osd_text;
    std::wstring active_layer;
    std::string config_file;
    std::string log_file;
    std::unique_ptr<tff::win::AsyncLog> log;
    std::unique_ptr<WindowsPlatform> platform;
    std::thread engine_thread;
    std::atomic<int> engine_rc{0};
    std::thread task_query_thread;
    std::atomic<bool> autostart_task{false};  ///< cached: schtasks /Query is slow
};

App g_app;

DWORD readSetting(const wchar_t* name, DWORD fallback) {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kSettingsKey, name, RRF_RT_REG_DWORD, nullptr, &value,
                     &size) == ERROR_SUCCESS) {
        return value;
    }
    return fallback;
}

void writeSetting(const wchar_t* name, DWORD value) {
    RegSetKeyValueW(HKEY_CURRENT_USER, kSettingsKey, name, REG_DWORD, &value, sizeof(value));
}

void logLine(const std::string& line) {
    if (g_app.log) {
        g_app.log->push(line);
    }
}

std::string timestamp() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth,
                  st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

void openLog() {
    g_app.log_file = tff::win::localDataDir() + "\\tff.log";
    // Keep one old log; start fresh when the current one gets large.
    std::error_code ec;
    auto path = std::filesystem::u8path(g_app.log_file);
    if (std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 1024 * 1024) {
        std::filesystem::rename(path, std::filesystem::u8path(g_app.log_file + ".1"), ec);
    }
    auto file = std::make_shared<std::ofstream>(path, std::ios::app);
    g_app.log = std::make_unique<tff::win::AsyncLog>([file](const std::string& line) {
        *file << timestamp() << " " << line;
        if (line.empty() || line.back() != '\n') {
            *file << "\n";
        }
        file->flush();
    });
}

/// Posts a heap string to the UI thread; frees it if the window is already gone.
void postString(HWND hwnd, UINT msg, WPARAM wparam, const std::string& text) {
    auto* str = new std::wstring(tff::win::widen(text));
    if (!PostMessageW(hwnd, msg, wparam, reinterpret_cast<LPARAM>(str))) {
        delete str;
    }
}

/// Stops the engine thread (releases injected keys, removes the hook) and waits for it.
void stopEngine() {
    if (g_app.platform) {
        g_app.platform->requestStop();
    }
    if (g_app.engine_thread.joinable()) {
        g_app.engine_thread.join();
    }
}

void openWithNotepad(const std::string& path) {
    ShellExecuteW(nullptr, L"open", L"notepad.exe", (L"\"" + tff::win::widen(path) + L"\"").c_str(),
                  nullptr, SW_SHOWNORMAL);
}

// ---------------------------------------------------------------------------
// Tray icon
// ---------------------------------------------------------------------------

std::wstring tooltipText() {
    std::wstring tip = L"TFF2 " + tff::win::widen(TFF_VERSION_STRING);
    if (g_app.platform && g_app.platform->isPaused()) {
        tip += L" - paused";
    } else if (!g_app.active_layer.empty()) {
        tip += L" - layer: " + g_app.active_layer;
    } else {
        tip += L" - active";
    }
    return tip.substr(0, 127);
}

bool updateTrayIcon(DWORD action) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_app.tray_window;
    nid.uID = kTrayId;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TFF_TRAY;
    const bool paused = g_app.platform && g_app.platform->isPaused();
    nid.hIcon = paused ? g_app.icon_paused : g_app.icon_active;
    wcsncpy_s(nid.szTip, tooltipText().c_str(), _TRUNCATE);
    const bool ok = Shell_NotifyIconW(action, &nid) != FALSE;
    if (ok && action == NIM_ADD) {
        nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &nid);
    }
    return ok;
}

/// Adds the tray icon; if the taskbar isn't there yet (TFF started at sign-in before
/// Explorer), retries with a timer.
void addTrayIcon() {
    KillTimer(g_app.tray_window, kTrayRetryTimer);
    if (updateTrayIcon(NIM_ADD)) {
        return;
    }
    // An icon left over from before an Explorer restart makes NIM_ADD fail: replace it
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_app.tray_window;
    nid.uID = kTrayId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    if (updateTrayIcon(NIM_ADD)) {
        return;
    }
    g_app.tray_retries_left = kTrayRetries;
    SetTimer(g_app.tray_window, kTrayRetryTimer, kTrayRetryMs, nullptr);
}

void showBalloon(const std::wstring& title, const std::wstring& text, DWORD icon_flags) {
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_app.tray_window;
    nid.uID = kTrayId;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = icon_flags | NIIF_RESPECT_QUIET_TIME;
    wcsncpy_s(nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nid.szInfo, text.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

// ---------------------------------------------------------------------------
// On-screen layer indicator: a small click-through window that never takes focus
// ---------------------------------------------------------------------------

LRESULT CALLBACK osdProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH bg = CreateSolidBrush(RGB(32, 32, 36));
            FillRect(dc, &rc, bg);
            DeleteObject(bg);
            const int dpi = static_cast<int>(GetDpiForWindow(hwnd));
            HFONT font =
                CreateFontW(-MulDiv(16, dpi, 72), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HGDIOBJ old = SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 255, 255));
            DrawTextW(dc, g_app.osd_text.c_str(), -1, &rc,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(dc, old);
            DeleteObject(font);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER:
            if (wparam == kOsdTimer) {
                KillTimer(hwnd, kOsdTimer);
                ShowWindow(hwnd, SW_HIDE);
            }
            return 0;
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

void showOsd(const std::wstring& text) {
    if (g_app.osd_window == nullptr) {
        g_app.osd_window = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED,
            kOsdClass, L"TFF layer", WS_POPUP, 0, 0, 10, 10, nullptr, nullptr, g_app.instance,
            nullptr);
        if (g_app.osd_window == nullptr) {
            return;
        }
        SetLayeredWindowAttributes(g_app.osd_window, 0, 225, LWA_ALPHA);
    }
    g_app.osd_text = text;

    // Bottom center of the monitor the mouse is on, above the taskbar.
    POINT pt;
    GetCursorPos(&pt);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY), &mi);
    const int dpi = static_cast<int>(GetDpiForWindow(g_app.osd_window));
    const int w = MulDiv(280, dpi, 96);
    const int h = MulDiv(56, dpi, 96);
    const RECT& wa = mi.rcWork;
    const int x = wa.left + (wa.right - wa.left - w) / 2;
    const int y = wa.bottom - h - MulDiv(48, dpi, 96);
    SetWindowPos(g_app.osd_window, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(g_app.osd_window, nullptr, TRUE);
    SetTimer(g_app.osd_window, kOsdTimer, kOsdMs, nullptr);
}

void notifyUser(const std::wstring& text) {
    switch (g_app.notify_mode) {
        case NotifyMode::Off:
            break;
        case NotifyMode::Osd:
            showOsd(text);
            break;
        case NotifyMode::Toast:
            showBalloon(L"TFF", text, NIIF_INFO | NIIF_NOSOUND);
            break;
    }
}

// ---------------------------------------------------------------------------
// Menu actions
// ---------------------------------------------------------------------------

void showCheatsheet() {
    std::string content;
    tff::Config config;
    std::string err;
    if (!tff::win::readTextFile(g_app.config_file, content) ||
        !tff::loadYamlConfig(content, config, err)) {
        MessageBoxW(g_app.tray_window,
                    tff::win::widen("Cannot read " + g_app.config_file + "\n" + err).c_str(),
                    L"TFF cheat sheet", MB_OK | MB_ICONWARNING);
        return;
    }
    tff::CheatsheetOptions opts;
    opts.color = false;
    opts.markdown = true;
    const std::string path = tff::win::localDataDir() + "\\cheatsheet.md";
    {
        std::ofstream f(std::filesystem::u8path(path), std::ios::binary);
        f << tff::Cheatsheet::generate(config, opts);
    }
    openWithNotepad(path);
}

void toggleAutostart() {
    std::string err;
    if (g_app.task_query_thread.joinable()) {
        g_app.task_query_thread.join();  // its (older) result must not overwrite ours
    }
    const auto status = tff::win::autostartStatus();
    const bool ok = (status.run_key || status.task) ? tff::win::autostartDisable(err)
                                                    : tff::win::autostartEnable(false, err);
    g_app.autostart_task = tff::win::autostartStatus().task;
    if (!ok) {
        MessageBoxW(g_app.tray_window, tff::win::widen(err).c_str(), L"TFF autostart",
                    MB_OK | MB_ICONWARNING);
    }
}

void setNotifyMode(NotifyMode mode) {
    g_app.notify_mode = mode;
    writeSetting(L"LayerNotifications", static_cast<DWORD>(mode));
}

void showMenu(HWND hwnd) {
    const bool paused = g_app.platform->isPaused();
    const auto autostart = tff::win::autostartStatus(false);
    const bool autostart_on = autostart.run_key || g_app.autostart_task.load();

    HMENU notify = CreatePopupMenu();
    AppendMenuW(notify,
                MF_STRING | (g_app.notify_mode == NotifyMode::Off ? MF_CHECKED : MF_UNCHECKED),
                ID_NOTIFY_OFF, L"Off");
    AppendMenuW(notify,
                MF_STRING | (g_app.notify_mode == NotifyMode::Osd ? MF_CHECKED : MF_UNCHECKED),
                ID_NOTIFY_OSD, L"On-screen indicator");
    AppendMenuW(notify,
                MF_STRING | (g_app.notify_mode == NotifyMode::Toast ? MF_CHECKED : MF_UNCHECKED),
                ID_NOTIFY_TOAST, L"Windows notification");

    HMENU menu = CreatePopupMenu();
    const std::wstring title = L"TFF2 " + tff::win::widen(TFF_VERSION_STRING);
    AppendMenuW(menu, MF_STRING | MF_GRAYED, 0, title.c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_PAUSE, paused ? L"&Resume remapping" : L"&Pause remapping");
    AppendMenuW(menu, MF_STRING, ID_RELOAD, L"Re&load configuration");
    AppendMenuW(menu, MF_STRING, ID_EDIT_CONFIG, L"&Edit configuration");
    AppendMenuW(menu, MF_STRING, ID_OPEN_CONFIG_DIR, L"Open configuration &folder");
    AppendMenuW(menu, MF_STRING, ID_CHEATSHEET, L"Show &cheat sheet");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(notify), L"Layer &notifications");
    AppendMenuW(menu, MF_STRING | (autostart_on ? MF_CHECKED : MF_UNCHECKED), ID_AUTOSTART,
                L"Start at &sign-in");
    AppendMenuW(menu, MF_STRING, ID_OPEN_LOG, L"Open lo&g");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_QUIT, L"&Quit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);  // so the menu closes when clicking elsewhere
    const UINT cmd = static_cast<UINT>(TrackPopupMenu(
        menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr));
    DestroyMenu(menu);  // also destroys the submenu
    PostMessageW(hwnd, WM_NULL, 0, 0);

    switch (cmd) {
        case ID_PAUSE:
            g_app.platform->setPaused(!paused);
            logLine(paused ? "[TFF] Resumed" : "[TFF] Paused");
            updateTrayIcon(NIM_MODIFY);
            break;
        case ID_RELOAD:
            g_app.platform->requestReload();
            break;
        case ID_EDIT_CONFIG:
            openWithNotepad(g_app.config_file);
            break;
        case ID_OPEN_CONFIG_DIR:
            ShellExecuteW(nullptr, L"open",
                          std::filesystem::u8path(g_app.config_file).parent_path().c_str(), nullptr,
                          nullptr, SW_SHOWNORMAL);
            break;
        case ID_CHEATSHEET:
            showCheatsheet();
            break;
        case ID_NOTIFY_OFF:
            setNotifyMode(NotifyMode::Off);
            break;
        case ID_NOTIFY_OSD:
            setNotifyMode(NotifyMode::Osd);
            break;
        case ID_NOTIFY_TOAST:
            setNotifyMode(NotifyMode::Toast);
            break;
        case ID_AUTOSTART:
            toggleAutostart();
            break;
        case ID_OPEN_LOG:
            openWithNotepad(g_app.log_file);
            break;
        case ID_QUIT:
            stopEngine();  // before the window goes away: shutdown may still post layer events
            DestroyWindow(hwnd);
            break;
        default:
            break;
    }
}

LRESULT CALLBACK trayProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == g_app.taskbar_created_msg && msg != 0) {
        addTrayIcon();  // Explorer (re)started
        return 0;
    }
    switch (msg) {
        case WM_TIMER:
            if (wparam == kTrayRetryTimer &&
                (updateTrayIcon(NIM_ADD) || --g_app.tray_retries_left <= 0)) {
                KillTimer(hwnd, kTrayRetryTimer);
            }
            return 0;
        case WM_TFF_TRAY:
            switch (LOWORD(lparam)) {
                case WM_CONTEXTMENU:
                case WM_RBUTTONUP:
                case NIN_SELECT:
                case NIN_KEYSELECT:
                    showMenu(hwnd);
                    break;
                default:
                    break;
            }
            return 0;
        case WM_TFF_LAYER: {
            std::unique_ptr<std::wstring> name(reinterpret_cast<std::wstring*>(lparam));
            const bool active = wparam != 0;
            if (active) {
                g_app.active_layer = *name;
            } else if (g_app.active_layer == *name) {
                g_app.active_layer.clear();
            }
            updateTrayIcon(NIM_MODIFY);
            notifyUser(L"Layer " + *name + (active ? L" ON" : L" OFF"));
            return 0;
        }
        case WM_TFF_RELOADED: {
            std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lparam));
            if (wparam != 0) {
                if (g_app.notify_mode != NotifyMode::Off) {
                    showOsd(L"Configuration reloaded");
                }
            } else {
                showBalloon(L"TFF: configuration error", *text, NIIF_WARNING);
            }
            return 0;
        }
        case WM_TFF_ENGINE_EXIT:
            if (g_app.engine_rc.load() != 0) {
                MessageBoxW(hwnd, L"Could not install the keyboard hook. TFF will exit.", L"TFF",
                            MB_OK | MB_ICONERROR);
            }
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            updateTrayIcon(NIM_DELETE);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

bool registerClasses() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = trayProc;
    wc.hInstance = g_app.instance;
    wc.hIcon = g_app.icon_active;
    wc.lpszClassName = kTrayClass;
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    WNDCLASSEXW osd{};
    osd.cbSize = sizeof(osd);
    osd.lpfnWndProc = osdProc;
    osd.hInstance = g_app.instance;
    osd.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    osd.lpszClassName = kOsdClass;
    return RegisterClassExW(&osd) != 0;
}

HICON loadIcon(int id) {
    HICON icon = static_cast<HICON>(LoadImageW(g_app.instance, MAKEINTRESOURCEW(id), IMAGE_ICON,
                                               GetSystemMetrics(SM_CXSMICON),
                                               GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    return icon != nullptr ? icon : LoadIconW(nullptr, IDI_APPLICATION);
}

/// Parses `tffw.exe [-c|--config file] [file.yaml]`.
std::string configFromCommandLine() {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::string config;
    for (int i = 1; argv != nullptr && i < argc; ++i) {
        const std::wstring arg = argv[i];
        if ((arg == L"-c" || arg == L"--config") && i + 1 < argc) {
            config = tff::win::narrow(argv[++i]);
        } else if (arg.size() > 5 && arg.find(L".yaml") != std::wstring::npos) {
            config = tff::win::narrow(arg);
        }
    }
    LocalFree(argv);
    return config;
}

void firstRun() {
    if (readSetting(L"FirstRunDone", 0) != 0) {
        return;
    }
    writeSetting(L"FirstRunDone", 1);
    const auto status = tff::win::autostartStatus();
    if (status.run_key || status.task) {
        return;
    }
    const std::wstring text = L"TFF is running in the notification area.\n\nConfiguration:\n" +
                              tff::win::widen(g_app.config_file) +
                              L"\n\nStart TFF automatically when you sign in?";
    if (MessageBoxW(nullptr, text.c_str(), L"Ten Flying Fingers", MB_YESNO | MB_ICONQUESTION) ==
        IDYES) {
        std::string err;
        if (!tff::win::autostartEnable(false, err)) {
            MessageBoxW(nullptr, tff::win::widen(err).c_str(), L"TFF autostart",
                        MB_OK | MB_ICONWARNING);
        }
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    g_app.instance = instance;

    bool already_running = false;
    HANDLE mutex = tff::win::acquireInstanceMutex(already_running);
    if (already_running) {
        MessageBoxW(nullptr,
                    L"TFF is already running (see the notification area, or a tff.exe console).",
                    L"Ten Flying Fingers", MB_OK | MB_ICONINFORMATION);
        if (mutex != nullptr) {
            CloseHandle(mutex);
        }
        return 0;
    }

    openLog();
    logLine("[TFF] tffw " + std::string(TFF_VERSION_STRING) + " starting");

    std::string err;
    g_app.config_file = configFromCommandLine();
    if (g_app.config_file.empty() && !tff::win::ensureUserConfig(g_app.config_file, err)) {
        logLine("[TFF] " + err);
        MessageBoxW(nullptr, tff::win::widen(err).c_str(), L"Ten Flying Fingers",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    g_app.platform = std::make_unique<WindowsPlatform>();
    WindowsPlatform& platform = *g_app.platform;
    platform.setWatchConfig(true);
    platform.setLogSink(logLine);
    if (!platform.loadConfiguration(g_app.config_file, err)) {
        logLine("[TFF] " + err);
        MessageBoxW(nullptr, tff::win::widen(err + "\n\nFix the file and start TFF again.").c_str(),
                    L"TFF: configuration error", MB_OK | MB_ICONERROR);
        return 1;
    }
    logLine("[TFF] Loaded " + platform.getConfigFile());

    g_app.icon_active = loadIcon(IDI_TFF_ACTIVE);
    g_app.icon_paused = loadIcon(IDI_TFF_PAUSED);
    g_app.notify_mode = static_cast<NotifyMode>(readSetting(L"LayerNotifications", 1));
    g_app.taskbar_created_msg = RegisterWindowMessageW(L"TaskbarCreated");
    if (!registerClasses()) {
        return 1;
    }
    g_app.tray_window =
        CreateWindowExW(0, kTrayClass, L"TFF2", 0, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (g_app.tray_window == nullptr) {
        return 1;
    }
    // Explorer announces a new taskbar with TaskbarCreated. When TFF runs elevated (logon task
    // with highest privileges), UIPI drops that message from the non-elevated Explorer unless
    // it is allowed explicitly.
    ChangeWindowMessageFilterEx(g_app.tray_window, g_app.taskbar_created_msg, MSGFLT_ALLOW,
                                nullptr);

    // Callbacks run on the engine thread: hand the data to the UI thread.
    HWND tray = g_app.tray_window;
    platform.setLayerCallback([tray](const std::string& layer, bool active) {
        postString(tray, WM_TFF_LAYER, active ? 1 : 0, layer);
    });
    platform.setReloadCallback([tray](bool ok, const std::string& msg) {
        postString(tray, WM_TFF_RELOADED, ok ? 1 : 0, msg);
    });

    // Keystrokes must never wait behind other processes' work.
    SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    g_app.engine_thread = std::thread([tray, &platform] {
        g_app.engine_rc = platform.run();
        PostMessageW(tray, WM_TFF_ENGINE_EXIT, 0, 0);
    });

    g_app.task_query_thread =
        std::thread([] { g_app.autostart_task = tff::win::autostartStatus().task; });
    addTrayIcon();
    firstRun();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    stopEngine();
    if (g_app.task_query_thread.joinable()) {
        g_app.task_query_thread.join();
    }
    logLine("[TFF] tffw stopped");
    g_app.platform.reset();
    g_app.log.reset();
    if (mutex != nullptr) {
        CloseHandle(mutex);
    }
    return g_app.engine_rc.load();
}
