// tff.exe: Windows command line tool and console remapper.
#include "windows_platform.h"
#include "win_util.h"
#include "tff_cheatsheet.h"
#include "tff_key_codes.h"
#include "tff_parser.h"
#include "tff_version.h"
#include "tff_wizard.h"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

std::atomic<WindowsPlatform*> g_platform{nullptr};

BOOL WINAPI consoleCtrlHandler(DWORD type) {
    switch (type) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            if (WindowsPlatform* p = g_platform.load()) {
                p->requestStop();
            }
            // Give the engine thread time to release injected keys before Windows kills us.
            if (type == CTRL_CLOSE_EVENT || type == CTRL_LOGOFF_EVENT ||
                type == CTRL_SHUTDOWN_EVENT) {
                Sleep(500);
            }
            return TRUE;
        default:
            return FALSE;
    }
}

bool isYaml(const std::string& arg) {
    return arg.find(".yaml") != std::string::npos || arg.find(".yml") != std::string::npos;
}

void printHelp(const char* prog) {
    std::cout
        << "Ten Flying Fingers (TFF) - Windows Keyboard Remapper\n"
        << "https://github.com/guettli/tff2\n\n"
        << "Usage:\n"
        << "  " << prog << " [options] [combos.yaml]\n"
        << "  " << prog << " init [options]\n"
        << "  " << prog << " monitor [options] [combos.yaml]\n"
        << "  " << prog << " cheatsheet [options] [combos.yaml]\n"
        << "  " << prog << " validate combos.yaml\n"
        << "  " << prog << " autostart enable|disable|status [--elevated]\n"
        << "  " << prog << " list\n\n"
        << "Commands:\n"
        << "  init                    Interactive configuration wizard and preset installer\n"
        << "  combos                  Run remapper with the specified combos file\n"
        << "  monitor                 Live event monitor and chord debugger (keys are not "
           "remapped)\n"
        << "  cheatsheet              Display visual terminal cheat sheet or markdown table\n"
        << "  validate                Validate a combos YAML configuration file\n"
        << "  autostart               Start the tray app (tffw.exe) when you sign in\n"
        << "  list                    List keyboards known to Windows\n"
        << "  help                    Show this help message\n\n"
        << "Options:\n"
        << "  -p, --preset <name>     Preset configuration (minimal, vim-nav, home-row-mods, "
           "full)\n"
        << "  --list-presets          List all available configuration presets and exit\n"
        << "  -o, --output <file>     Destination file for generated configuration (default: "
           "%APPDATA%\\tff\\tff-combos.yaml)\n"
        << "  -f, --force             Overwrite existing destination file without confirmation\n"
        << "  --print                 Print generated config to stdout and exit\n"
        << "  -s, --cheatsheet        Display cheat sheet of configured keys and layers\n"
        << "  --markdown, --md        Output cheat sheet formatted as GitHub Markdown tables\n"
        << "  --plain, --no-color     Disable ANSI colors in cheat sheet and monitor output\n"
        << "  --no-deltas             Disable timing deltas in live event monitor\n"
        << "  --no-emitted            Disable emitted virtual key lines in live event monitor\n"
        << "  -c, --config <file>     Path to combos YAML configuration file\n"
        << "                          (default: config\\tff-combos.yaml, then "
           "%APPDATA%\\tff\\tff-combos.yaml)\n"
        << "  -w, --watch-config      Reload the configuration file when it changes\n"
        << "  --ignore-injected       Don't remap input injected by other software\n"
        << "  --elevated              autostart: use an elevated logon task, so remapping also\n"
        << "                          works in windows running as administrator\n"
        << "  -v, --verbose           Print detailed key down/up event logs\n"
        << "  -V, --version           Show program version information and exit\n"
        << "  -h, --help              Show this help message\n\n"
        << "Press Ctrl+C to stop the remapper. For a background remapper with a tray icon, "
           "run tffw.exe.\n\n"
        << "Examples:\n"
        << "  " << prog << " init --preset vim-nav\n"
        << "  " << prog << " config\\tff-combos.yaml\n"
        << "  " << prog << " --watch-config\n"
        << "  " << prog << " monitor\n"
        << "  " << prog << " cheatsheet --markdown\n"
        << "  " << prog << " validate config\\tff-combos.yaml\n"
        << "  " << prog << " autostart enable\n";
}

bool loadConfigFile(const std::string& config_file, tff::Config& config) {
    if (config_file.empty()) {
        std::cerr << "Error: Configuration file is missing. Please specify a config file via "
                     "--config or create %APPDATA%\\tff\\tff-combos.yaml (tff init)\n";
        return false;
    }
    std::string content;
    if (!tff::win::readTextFile(config_file, content)) {
        std::cerr << "Error: Failed to open config file: " << config_file << "\n";
        return false;
    }
    std::string err_msg;
    if (!tff::loadYamlConfig(content, config, err_msg)) {
        std::cerr << "Validation error: " << err_msg << "\n";
        return false;
    }
    return true;
}

int runValidate(const std::string& config_file) {
    tff::Config config;
    if (!loadConfigFile(config_file, config)) {
        return 1;
    }
    std::cout << "Configuration is valid! Loaded " << config.combos.size() << " combo(s), "
              << config.tap_hold_keys.size() << " tap-hold key(s), " << config.tap_dances.size()
              << " tap-dance key(s), " << config.one_shot_keys.size() << " one-shot key(s), "
              << config.leader.sequences.size() << " leader sequence(s), "
              << (config.auto_shift.enabled
                      ? ("auto-shift (" + std::to_string(config.auto_shift.keys.size()) +
                         " keys), ")
                      : "")
              << config.layers.size()
              << " layer(s), and settings (combo: " << config.settings.combo_timeout_ms
              << "ms, tap-hold: " << config.settings.tap_hold_timeout_ms << "ms) from "
              << config_file << "\n";
    return 0;
}

int runCheatsheet(const std::string& config_file, bool markdown, bool color) {
    tff::Config config;
    if (!loadConfigFile(config_file, config)) {
        return 1;
    }
    tff::CheatsheetOptions opts;
    opts.color = color;
    opts.markdown = markdown;
    std::cout << tff::Cheatsheet::generate(config, opts);
    return 0;
}

int runList() {
    UINT count = 0;
    GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST));
    std::vector<RAWINPUTDEVICELIST> devices(count);
    if (count > 0 &&
        GetRawInputDeviceList(devices.data(), &count, sizeof(RAWINPUTDEVICELIST)) == UINT(-1)) {
        std::cerr << "Error: GetRawInputDeviceList failed\n";
        return 1;
    }
    std::cout << "Keyboards known to Windows:\n\n";
    int n = 0;
    for (UINT i = 0; i < count; ++i) {
        if (devices[i].dwType != RIM_TYPEKEYBOARD) {
            continue;
        }
        UINT size = 0;
        GetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICENAME, nullptr, &size);
        std::wstring name(size, L'\0');
        if (size > 0) {
            GetRawInputDeviceInfoW(devices[i].hDevice, RIDI_DEVICENAME, name.data(), &size);
            name.resize(wcslen(name.c_str()));
        }
        std::cout << "  [" << ++n << "] " << tff::win::narrow(name) << "\n";
    }
    if (n == 0) {
        std::cout << "  (none)\n";
    }
    std::cout << "\nNote: on Windows TFF remaps all keyboards; selecting devices is not "
                 "supported.\n";
    return 0;
}

int runAutostart(const std::string& action, bool elevated) {
    std::string err;
    if (action == "enable") {
        if (!tff::win::autostartEnable(elevated, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        std::cout << "Autostart enabled"
                  << (elevated ? " (elevated logon task)" : " (HKCU Run key)") << ".\n";
        return 0;
    }
    if (action == "disable") {
        if (!tff::win::autostartDisable(err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        std::cout << "Autostart disabled.\n";
        return 0;
    }
    if (action == "status" || action.empty()) {
        auto s = tff::win::autostartStatus();
        std::cout << "Run key (HKCU):      "
                  << (s.run_key ? "enabled -> " + s.run_command : std::string("disabled")) << "\n"
                  << "Elevated logon task: " << (s.task ? "enabled" : "disabled") << "\n";
        return 0;
    }
    std::cerr << "Error: unknown autostart action '" << action
              << "' (use enable, disable or status)\n";
    return 1;
}

/// Holds the single-instance mutex while a remapper runs.
class InstanceLock {
public:
    InstanceLock()
        : handle_(CreateMutexW(nullptr, TRUE, tff::win::kInstanceMutexName)),
          already_running_(handle_ != nullptr && GetLastError() == ERROR_ALREADY_EXISTS) {}
    ~InstanceLock() {
        if (handle_ != nullptr) {
            CloseHandle(handle_);
        }
    }
    InstanceLock(const InstanceLock&) = delete;
    InstanceLock& operator=(const InstanceLock&) = delete;
    bool alreadyRunning() const { return already_running_; }

private:
    HANDLE handle_ = nullptr;
    bool already_running_ = false;
};

}  // anonymous namespace

// cppcheck-suppress constParameter ; standard main() signature
int main(int argc, char* argv[]) {
    const bool color_console = tff::win::setupConsole();

    std::string config_file;
    bool config_explicit = false;
    bool watch_config = false;
    bool validate_only = false;
    bool cheatsheet_only = false;
    bool monitor_only = false;
    bool init_mode = false;
    bool init_print = false;
    bool list_only = false;
    bool autostart_mode = false;
    bool autostart_elevated = false;
    std::string autostart_action;
    tff::wizard::WizardOptions wizard_opts;
    bool monitor_show_deltas = true;
    bool monitor_show_emitted = true;
    bool cheatsheet_markdown = false;
    bool color = color_console;
    bool notify = false;
    bool notify_explicit = false;
    bool ignore_injected = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help" || arg == "help") {
            printHelp("tff");
            return 0;
        } else if (arg == "-V" || arg == "--version" || arg == "version") {
            std::cout << "tff " << TFF_VERSION_STRING << " (Windows)\n";
            return 0;
        } else if (arg == "-l" || arg == "--list" || arg == "list") {
            list_only = true;
        } else if (arg == "--list-presets") {
            init_mode = true;
            wizard_opts.list_presets = true;
        } else if (arg == "-p" || arg == "--preset") {
            init_mode = true;
            if (i + 1 < argc) {
                wizard_opts.preset_name = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires a preset name\n";
                return 1;
            }
        } else if (arg == "-f" || arg == "--force") {
            wizard_opts.force = true;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                wizard_opts.output_path = argv[++i];
            } else {
                std::cerr << "Error: --output requires a file path argument\n";
                return 1;
            }
        } else if (arg == "init") {
            init_mode = true;
        } else if (arg == "--print") {
            init_print = true;
        } else if (arg == "cheatsheet" || arg == "--cheatsheet" || arg == "-s") {
            cheatsheet_only = true;
        } else if (arg == "validate" || arg == "--validate") {
            validate_only = true;
        } else if (arg == "monitor" || arg == "--monitor") {
            monitor_only = true;
        } else if (arg == "combos") {
            // explicit run command; the config file follows as positional argument
        } else if (arg == "autostart") {
            autostart_mode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                autostart_action = argv[++i];
            }
        } else if (arg == "--elevated") {
            autostart_elevated = true;
        } else if (arg == "--markdown" || arg == "--md") {
            cheatsheet_markdown = true;
        } else if (arg == "--plain" || arg == "--no-color") {
            color = false;
        } else if (arg == "--no-deltas") {
            monitor_show_deltas = false;
        } else if (arg == "--no-emitted") {
            monitor_show_emitted = false;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-w" || arg == "--watch-config") {
            watch_config = true;
        } else if (arg == "--notify") {
            notify = true;
            notify_explicit = true;
        } else if (arg == "--no-notify") {
            notify = false;
            notify_explicit = true;
        } else if (arg == "--ignore-injected") {
            ignore_injected = true;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
                config_explicit = true;
            } else {
                std::cerr << "Error: --config requires a file path argument\n";
                return 1;
            }
        } else if (arg == "setup-udev" || arg == "dump" || arg == "-d" || arg == "--device" ||
                   arg == "-g" || arg == "--grab" || arg == "--no-grab" || arg == "--hotplug" ||
                   arg == "--no-hotplug" || arg == "--port" || arg == "-i" || arg == "--install" ||
                   arg == "--check" || arg == "--rule-path") {
            std::cerr << "Error: '" << arg << "' is not supported on Windows (Linux only).\n";
            return 2;
        } else if (arg.rfind("-", 0) == 0) {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            printHelp("tff");
            return 1;
        } else if (init_mode) {
            if (wizard_opts.preset_name.empty() && tff::wizard::findPreset(arg) != nullptr) {
                wizard_opts.preset_name = arg;
            } else if (isYaml(arg)) {
                wizard_opts.output_path = arg;
            }
        } else if (isYaml(arg) || cheatsheet_only || validate_only) {
            config_file = arg;
            config_explicit = true;
        } else {
            std::cerr << "Error: Unexpected argument: " << arg
                      << " (selecting keyboard devices is not supported on Windows)\n";
            return 2;
        }
    }

    if (init_mode) {
        if (init_print) {
            wizard_opts.print_only = true;
        }
        return tff::wizard::runWizard(wizard_opts);
    }
    if (list_only) {
        return runList();
    }
    if (autostart_mode) {
        return runAutostart(autostart_action, autostart_elevated);
    }
    if (!config_explicit) {
        config_file = tff::win::resolveConfigFile();
    }
    if (validate_only) {
        if (config_file.empty()) {
            std::cerr << "Error: Configuration file is missing. Please specify a config file to "
                         "validate.\n";
            return 1;
        }
        return runValidate(config_file);
    }
    if (cheatsheet_only) {
        return runCheatsheet(config_file, cheatsheet_markdown, color);
    }

    if (config_file.empty()) {
        std::cerr << "Error: Configuration file is missing. Please provide a configuration file "
                     "via --config or create %APPDATA%\\tff\\tff-combos.yaml (tff init)\n";
        return 1;
    }

    InstanceLock lock;
    if (!monitor_only && lock.alreadyRunning()) {
        std::cerr << "Error: TFF is already running (tff.exe or the tray app tffw.exe). Stop "
                     "it first, otherwise keys would be remapped twice.\n";
        return 1;
    }

    tff::win::AsyncLog out([](const std::string& line) {
        std::fwrite(line.data(), 1, line.size(), stdout);
        if (line.empty() || line.back() != '\n') {
            std::fputc('\n', stdout);
        }
        std::fflush(stdout);
    });

    WindowsPlatform platform;
    platform.setVerbose(verbose);
    platform.setIgnoreInjected(ignore_injected);
    platform.setWatchConfig(watch_config);
    platform.setLogSink([&out](const std::string& line) { out.push(line); });
    if (monitor_only) {
        tff::MonitorOptions opts;
        opts.color = color;
        opts.show_deltas = monitor_show_deltas;
        opts.show_emitted = monitor_show_emitted;
        platform.setMonitorMode(opts);
    }

    std::string err;
    if (!platform.loadConfiguration(config_file, err)) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    if (!notify_explicit) {
        notify = platform.getSettings().notifications;
    }
    if (notify && !monitor_only) {
        platform.setLayerCallback([&out](const std::string& layer, bool active) {
            out.push("[TFF Layer] '" + layer + "' " + (active ? "ON" : "OFF"));
        });
    }

    const auto& engine = platform.getEngine();
    if (monitor_only) {
        std::cout << "Ten Flying Fingers (TFF) - Live Event Monitor & Debugger\n"
                  << "========================================================\n"
                  << "Configuration: " << platform.getConfigFile() << " ("
                  << engine.getCombos().size() << " combos, " << engine.getTapHoldKeys().size()
                  << " tap-hold, " << engine.getLayers().size() << " layers)\n"
                  << "Observe-only mode: keys are shown but not remapped.\n"
                  << "\nPress keys on your keyboard to monitor events. Press Ctrl+C to exit.\n"
                  << "-----------------------------------------------------------------------"
                     "---------\n";
    } else {
        std::cout << "Ten Flying Fingers (TFF) - Windows Remapper\n"
                  << "==========================================\n"
                  << "Loading configuration: " << platform.getConfigFile() << "\n"
                  << "Loaded " << engine.getCombos().size() << " combo(s), "
                  << engine.getTapHoldKeys().size() << " tap-hold key(s), "
                  << engine.getTapDances().size() << " tap-dance key(s), "
                  << engine.getOneShotKeys().size() << " one-shot key(s), "
                  << engine.getLeaderConfig().sequences.size() << " leader sequence(s), "
                  << engine.getLayers().size()
                  << " layer(s), settings (combo: " << platform.getSettings().combo_timeout_ms
                  << "ms, tap-hold: " << platform.getSettings().tap_hold_timeout_ms << "ms)\n";
        if (!tff::win::isElevated()) {
            std::cout << "Note: not running as administrator; keys typed into elevated windows "
                         "are not remapped.\n";
        }
        std::cout << "Press Ctrl+C to stop.\n" << std::flush;
    }

    g_platform = &platform;
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
    const int rc = platform.run();
    g_platform = nullptr;
    if (rc != 0) {
        std::cerr << "Error: could not install the keyboard hook.\n";
        return rc;
    }
    std::cout << "\nStopping TFF... Clean shutdown complete.\n";
    return 0;
}
