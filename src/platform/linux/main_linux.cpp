#include "linux_platform.h"
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <atomic>
#include <unistd.h>

namespace {
std::atomic<bool> g_should_stop{false};

void signalHandler(int sig) {
    (void)sig;
    g_should_stop.store(true);
}

void printHelp(const char* prog) {
    std::cout << "Ten Flying Fingers (TFF) - Linux Keyboard Remapper\n\n"
              << "Usage:\n"
              << "  " << prog << " [options] [combos.yaml] [device1 device2 ...]\n\n"
              << "Options:\n"
              << "  -c, --config <file>     Path to combos YAML configuration file\n"
              << "                          (default: config/tff-combos.yaml)\n"
              << "  -d, --device <path>     Path to input evdev device (e.g. /dev/input/event8)\n"
              << "                          (default: auto-discover keyboards)\n"
              << "  -g, --grab              Exclusively grab input device (default: true)\n"
              << "  --no-grab               Do not grab device (events still pass to OS)\n"
              << "  -l, --list              List all discovered keyboards and exit\n"
              << "  -v, --verbose           Print detailed key down/up event logs\n"
              << "  -h, --help              Show this help message\n\n"
              << "Examples:\n"
              << "  " << prog << " config/tff-combos.yaml\n"
              << "  " << prog << " --list\n"
              << "  " << prog << " -c config/tff-combos.yaml -d /dev/input/event8\n";
}
} // anonymous namespace

int main(int argc, char* argv[]) {
    std::string config_file = "config/tff-combos.yaml";
    std::vector<std::string> device_paths;
    bool grab = true;
    bool list_only = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "-l" || arg == "--list") {
            list_only = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-g" || arg == "--grab") {
            grab = true;
        } else if (arg == "--no-grab") {
            grab = false;
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
            if (config_file == "config/tff-combos.yaml" &&
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
            std::cout << "Found " << keyboards.size() << " keyboard device(s):\n";
            for (const auto& dev : keyboards) {
                std::cout << "  " << dev << "\n";
            }
        }
        return 0;
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

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
        // Check fallback location
        if (config_file == "config/tff-combos.yaml" && platform.loadConfiguration("../config/tff-combos.yaml")) {
            std::cout << "Loaded configuration from ../config/tff-combos.yaml\n";
        } else {
            std::cerr << "Failed to load combo configuration file: " << config_file << "\n";
            return 1;
        }
    }

    const auto& combos = platform.getEngine().getCombos();
    std::cout << "Loaded " << combos.size() << " combo mapping(s)\n";

    if (device_paths.empty()) {
        std::cout << "Auto-discovering keyboards...\n";
        device_paths = LinuxPlatform::discoverKeyboards();
        if (device_paths.empty()) {
            std::cerr << "Error: No keyboard devices found in /dev/input/\n";
            return 1;
        }
    }

    std::cout << "Opening " << device_paths.size() << " input keyboard device(s) (grab="
              << (grab ? "yes" : "no") << "):\n";
    for (const auto& p : device_paths) {
        std::cout << "  -> " << p << "\n";
    }

    if (!platform.openInputDevices(device_paths, grab)) {
        std::cerr << "Error: Could not open any input devices\n";
        return 1;
    }

    std::cout << "Virtual keyboard created via /dev/uinput\n";
    std::cout << "Running event loop. Press Ctrl+C to stop.\n\n";

    platform.run(g_should_stop);

    std::cout << "\nStopping TFF Linux Remapper...\n";
    platform.cleanup();
    std::cout << "Restored keyboard devices. Clean shutdown complete.\n";

    return 0;
}
