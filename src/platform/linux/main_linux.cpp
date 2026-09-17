#include "linux_platform.h"
#include "tff_cheatsheet.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <atomic>
#include <unistd.h>

namespace {
std::atomic<bool> g_should_stop{false};
std::atomic<bool> g_should_reload{false};

void signalHandler(int sig) {
    if (sig == SIGHUP) {
        g_should_reload.store(true);
    } else {
        g_should_stop.store(true);
    }
}

void printHelp(const char* prog) {
    std::cout << "Ten Flying Fingers (TFF) - Linux Keyboard Remapper\n"
              << "https://github.com/guettli/tff2\n\n"
              << "Usage:\n"
              << "  " << prog << " [options] [combos.yaml] [device1 device2 ...]\n"
              << "  " << prog << " combos [options] combos.yaml [device1 device2 ...]\n"
              << "  " << prog << " cheatsheet [options] [combos.yaml]\n"
              << "  " << prog << " validate combos.yaml\n"
              << "  " << prog << " list\n\n"
              << "Commands:\n"
              << "  combos                  Run remapper with specified combos and devices\n"
              << "  cheatsheet              Display visual terminal cheat sheet or markdown table\n"
              << "  validate                Validate a combos YAML configuration file\n"
              << "  list                    List all discovered keyboards with persistent paths\n"
              << "  help                    Show this help message\n\n"
              << "Options:\n"
              << "  -s, --cheatsheet        Display cheat sheet of configured keys and layers\n"
              << "  --markdown, --md        Output cheat sheet formatted as GitHub Markdown tables\n"
              << "  --plain, --no-color     Disable ANSI color codes in cheat sheet output\n"
              << "  -c, --config <file>     Path to combos YAML configuration file\n"
              << "                          (default: config/tff-combos.yaml)\n"
              << "  -w, --watch-config      Watch configuration file for live changes via inotify\n"
              << "  -d, --device <path>     Path to input evdev device (e.g. /dev/input/event8)\n"
              << "                          (default: auto-discover all connected keyboards)\n"
              << "  -g, --grab              Exclusively grab input device (default: true)\n"
              << "  --no-grab               Do not grab device (events still pass to OS)\n"
              << "  --hotplug               Enable dynamic inotify keyboard hotplugging (default: enabled)\n"
              << "  --no-hotplug            Disable dynamic inotify keyboard hotplugging\n"
              << "  -l, --list              List all discovered keyboards and exit\n"
              << "  -v, --verbose           Print detailed key down/up event logs\n"
              << "  -h, --help              Show this help message\n\n"
              << "Signals:\n"
              << "  SIGHUP                  Hot-reload configuration file without dropping keyboard grabs\n"
              << "  SIGINT, SIGTERM         Graceful shutdown and restore keyboards\n\n"
              << "Examples:\n"
              << "  " << prog << " config/tff-combos.yaml\n"
              << "  " << prog << " cheatsheet\n"
              << "  " << prog << " cheatsheet --markdown\n"
              << "  " << prog << " --watch-config config/tff-combos.yaml\n"
              << "  " << prog << " --list\n"
              << "  " << prog << " combos my-combos.yaml /dev/input/by-id/usb-*-event-kbd\n"
              << "  " << prog << " validate config/tff-combos.yaml\n";
}
} // anonymous namespace

int main(int argc, char* argv[]) {
    std::string config_file = "config/tff-combos.yaml";
    std::vector<std::string> device_paths;
    bool grab = true;
    bool grab_explicit = false;
    bool hotplug = true;
    bool hotplug_explicit = false;
    bool watch_config = false;
    bool list_only = false;
    bool validate_only = false;
    bool cheatsheet_only = false;
    bool cheatsheet_markdown = false;
    bool cheatsheet_color = true;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help" || arg == "help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-l" || arg == "--list" || arg == "list") {
            list_only = true;
        } else if (arg == "validate") {
            validate_only = true;
        } else if (arg == "cheatsheet" || arg == "--cheatsheet" || arg == "-s") {
            cheatsheet_only = true;
        } else if (arg == "--markdown" || arg == "--md") {
            cheatsheet_markdown = true;
        } else if (arg == "--plain" || arg == "--no-color") {
            cheatsheet_color = false;
        } else if (arg == "combos") {
            // Subcommand keyword for compatibility with Go tff
            continue;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-w" || arg == "--watch-config") {
            watch_config = true;
        } else if (arg == "-g" || arg == "--grab") {
            grab = true;
            grab_explicit = true;
        } else if (arg == "--no-grab") {
            grab = false;
            grab_explicit = true;
        } else if (arg == "--hotplug") {
            hotplug = true;
            hotplug_explicit = true;
        } else if (arg == "--no-hotplug") {
            hotplug = false;
            hotplug_explicit = true;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file argument\n";
                return 1;
            }
        } else if (arg == "-d" || arg == "--device") {
            if (i + 1 < argc) {
                device_paths.push_back(argv[++i]);
            } else {
                std::cerr << "Error: --device requires a path argument\n";
                return 1;
            }
        } else if (arg.rfind("-", 0) == 0) {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            printHelp(argv[0]);
            return 1;
        } else {
            // Positional arguments
            if (cheatsheet_only || validate_only) {
                config_file = arg;
            } else if (config_file == "config/tff-combos.yaml" &&
                       (arg.find(".yaml") != std::string::npos || arg.find(".yml") != std::string::npos)) {
                config_file = arg;
            } else {
                device_paths.push_back(arg);
            }
        }
    }

    if (list_only) {
        std::cout << "Scanning for keyboards in /dev/input/...\n";
        auto keyboards = LinuxPlatform::discoverKeyboards();
        if (keyboards.empty()) {
            std::cout << "No keyboard devices found.\n";
        } else {
            std::cout << "Found " << keyboards.size() << " keyboard device(s):\n\n";
            for (size_t i = 0; i < keyboards.size(); ++i) {
                const auto& dev = keyboards[i];
                std::string name = LinuxPlatform::getDeviceName(dev);
                std::string alias = LinuxPlatform::getDeviceAlias(dev);
                std::cout << "  [" << (i + 1) << "] " << dev << "\n";
                if (!name.empty()) {
                    std::cout << "      Name:  " << name << "\n";
                }
                if (!alias.empty()) {
                    std::cout << "      Alias: " << alias << "\n";
                }
                std::cout << "\n";
            }
            std::cout << "Hint: In systemd service files, use the persistent Alias path to survive reboots/replugs.\n";
        }
        return 0;
    }

    if (validate_only) {
        std::ifstream file(config_file);
        if (!file.is_open() && config_file.rfind("../", 0) != 0) {
            file.open("../" + config_file);
        }
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open config file: " << config_file << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        tff::Config config;
        std::string err_msg;
        if (!tff::loadYamlConfig(buffer.str(), config, err_msg)) {
            std::cerr << "Validation error: " << err_msg << "\n";
            return 1;
        }
        std::cout << "Configuration is valid! Loaded " << config.combos.size() << " combo(s), "
                  << config.tap_hold_keys.size() << " tap-hold key(s), "
                  << config.one_shot_keys.size() << " one-shot key(s), "
                  << config.leader.sequences.size() << " leader sequence(s), "
                  << (config.auto_shift.enabled ? ("auto-shift (" + std::to_string(config.auto_shift.keys.size()) + " keys), ") : "")
                  << config.layers.size() << " layer(s), and settings (combo: "
                  << config.settings.combo_timeout_ms << "ms, tap-hold: "
                  << config.settings.tap_hold_timeout_ms << "ms) from " << config_file << "\n";
        return 0;
    }

    if (cheatsheet_only) {
        std::ifstream file(config_file);
        if (!file.is_open() && config_file.rfind("../", 0) != 0) {
            file.open("../" + config_file);
        }
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open config file: " << config_file << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        tff::Config config;
        std::string err_msg;
        if (!tff::loadYamlConfig(buffer.str(), config, err_msg)) {
            std::cerr << "Error parsing config file: " << err_msg << "\n";
            return 1;
        }

        const char* no_color = std::getenv("NO_COLOR");
        if (!isatty(STDOUT_FILENO) || (no_color != nullptr && no_color[0] != '\0')) {
            cheatsheet_color = false;
        }

        tff::CheatsheetOptions opts;
        opts.color = cheatsheet_color;
        opts.markdown = cheatsheet_markdown;

        std::cout << tff::Cheatsheet::generate(config, opts);
        return 0;
    }

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);

    LinuxPlatform platform;
    platform.setVerbose(verbose);

    std::cout << "Ten Flying Fingers (TFF) - Linux Remapper\n";
    std::cout << "========================================\n";

    if (!platform.initialize()) {
        std::cerr << "Failed to initialize Linux platform\n";
        return 1;
    }

    std::cout << "Loading configuration: " << config_file << "\n";
    if (!platform.loadConfiguration(config_file)) {
        if (config_file == "config/tff-combos.yaml" && platform.loadConfiguration("../config/tff-combos.yaml")) {
            std::cout << "Loaded configuration from ../config/tff-combos.yaml\n";
        } else {
            std::cerr << "Failed to load combo configuration file: " << config_file << "\n";
            return 1;
        }
    }

    const auto& engine = platform.getEngine();
    std::cout << "Loaded " << engine.getCombos().size() << " combo(s), "
              << engine.getTapHoldKeys().size() << " tap-hold key(s), "
              << engine.getOneShotKeys().size() << " one-shot key(s), "
              << engine.getLeaderConfig().sequences.size() << " leader sequence(s), "
              << engine.getLayers().size() << " layer(s), settings (combo: "
              << platform.getSettings().combo_timeout_ms << "ms, tap-hold: "
              << platform.getSettings().tap_hold_timeout_ms << "ms)\n";

    if (!grab_explicit) {
        grab = platform.getSettings().exclusive_grab;
    }
    if (!hotplug_explicit) {
        hotplug = platform.getSettings().hotplug;
    }

    platform.setGrab(grab);
    platform.enableHotplug(hotplug);
    platform.enableConfigWatch(watch_config);

    if (device_paths.empty()) {
        std::cout << "Auto-discovering keyboards...\n";
        device_paths = LinuxPlatform::discoverKeyboards();
        if (device_paths.empty()) {
            if (hotplug) {
                std::cout << "No keyboard devices currently found in /dev/input/.\n"
                          << "Dynamic hotplugging active: waiting for keyboards to be plugged in...\n";
            } else {
                std::cerr << "Error: No keyboard devices found in /dev/input/\n";
                return 1;
            }
        }
    }

    if (!device_paths.empty()) {
        std::cout << "Opening " << device_paths.size() << " input keyboard device(s) (grab="
                  << (grab ? "yes" : "no") << "):\n";
        for (const auto& p : device_paths) {
            std::string name = LinuxPlatform::getDeviceName(p);
            std::string alias = LinuxPlatform::getDeviceAlias(p);
            std::cout << "  -> " << p;
            if (!name.empty()) std::cout << " (" << name << ")";
            std::cout << "\n";
            if (!alias.empty()) {
                std::cout << "     Alias: " << alias << "\n";
            }
        }

        if (!platform.openInputDevices(device_paths, grab)) {
            if (!hotplug) {
                std::cerr << "Error: Could not open any input devices\n";
                return 1;
            }
            std::cerr << "Warning: Could not open initial devices; waiting for hotplug events.\n";
        }
    }

    std::cout << "Virtual keyboard created via /dev/uinput\n";
    std::cout << "Running event loop. Press Ctrl+C to stop.\n\n";

    platform.run(g_should_stop, &g_should_reload);

    std::cout << "\nStopping TFF Linux Remapper...\n";
    platform.cleanup();
    std::cout << "Restored keyboard devices. Clean shutdown complete.\n";

    return 0;
}
