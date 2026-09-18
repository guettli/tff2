#include "tff_udev.h"

#if !defined(PICO_BUILD)

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <filesystem>

namespace tff {
namespace udev {

std::string getUdevRuleContent() {
    return "# Ten Flying Fingers (TFF) - User permissions for evdev and uinput\n"
           "# Allows members of the 'input' group and active console sessions (uaccess)\n"
           "# to access keyboard devices and virtual input injection without sudo/root.\n"
           "\n"
           "# /dev/uinput: virtual keyboard and mouse event emission\n"
           "KERNEL==\"uinput\", SUBSYSTEM==\"misc\", TAG+=\"uaccess\", "
           "OPTIONS+=\"static_node=uinput\", MODE=\"0660\", GROUP=\"input\"\n"
           "\n"
           "# /dev/input/event*: physical keyboard event grabbing\n"
           "KERNEL==\"event*\", SUBSYSTEM==\"input\", MODE=\"0660\", GROUP=\"input\"\n";
}

std::string getModulesLoadContent() {
    return "# Load uinput kernel module on boot for Ten Flying Fingers (TFF)\n"
           "uinput\n";
}

PermissionCheckResult checkPermissions(const std::string& rule_path) {
    PermissionCheckResult res;

    // 1. User identification
    res.uid = getuid();
    res.gid = getgid();
    res.is_root = (res.uid == 0);

    const char* sudo_user_env = std::getenv("SUDO_USER");
    if (sudo_user_env != nullptr && sudo_user_env[0] != '\0') {
        res.sudo_user = sudo_user_env;
    }

    const struct passwd* pw = getpwuid(res.uid);
    if (pw != nullptr && pw->pw_name != nullptr) {
        res.username = pw->pw_name;
    } else {
        res.username = std::to_string(res.uid);
    }

    // 2. Input group membership
    const struct group* grp = getgrnam("input");
    gid_t input_gid = (grp != nullptr) ? grp->gr_gid : static_cast<gid_t>(-1);

    if (input_gid != static_cast<gid_t>(-1)) {
        if (res.gid == input_gid) {
            res.in_input_group_active = true;
        } else {
            int ngroups = getgroups(0, nullptr);
            if (ngroups > 0) {
                std::vector<gid_t> groups(static_cast<size_t>(ngroups));
                if (getgroups(ngroups, groups.data()) != -1) {
                    if (std::find(groups.begin(), groups.end(), input_gid) != groups.end()) {
                        res.in_input_group_active = true;
                    }
                }
            }
        }

        // Check configured membership in database
        if (!res.username.empty()) {
            int n_conf = 0;
            getgrouplist(res.username.c_str(), res.gid, nullptr, &n_conf);
            if (n_conf > 0) {
                std::vector<gid_t> conf_groups(static_cast<size_t>(n_conf));
                if (getgrouplist(res.username.c_str(), res.gid, conf_groups.data(), &n_conf) !=
                    -1) {
                    if (std::find(conf_groups.begin(), conf_groups.end(), input_gid) !=
                        conf_groups.end()) {
                        res.in_input_group_configured = true;
                    }
                }
            }
        }
    }

    // 3. /dev/uinput access
    res.uinput_exists = (access("/dev/uinput", F_OK) == 0);
    res.uinput_readable = (access("/dev/uinput", R_OK) == 0);
    res.uinput_writable = (access("/dev/uinput", W_OK) == 0);

    // 4. /dev/input/event* devices
    DIR* dir = opendir("/dev/input");
    if (dir != nullptr) {
        const struct dirent* entry = nullptr;
        while ((entry = readdir(dir)) != nullptr) {
            if (std::strncmp(entry->d_name, "event", 5) == 0) {
                res.event_devices_total++;
                std::string full_path = std::string("/dev/input/") + entry->d_name;
                if (access(full_path.c_str(), R_OK) == 0) {
                    res.event_devices_accessible++;
                }
            }
        }
        closedir(dir);
    }

    // 5. Installed udev rule
    auto check_rule_file = [&](const std::string& path) -> bool {
        if (access(path.c_str(), F_OK) == 0) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::stringstream ss;
                ss << file.rdbuf();
                std::string content = ss.str();
                if (content.find("uinput") != std::string::npos) {
                    res.udev_rule_installed = true;
                    res.installed_rule_path = path;
                    res.installed_rule_content = content;
                    return true;
                }
            }
        }
        return false;
    };

    if (!check_rule_file(rule_path)) {
        check_rule_file("/etc/udev/rules.d/99-tff-uinput.rules");
    }

    // 6. Kernel module status
    std::ifstream proc_modules("/proc/modules");
    if (proc_modules.is_open()) {
        std::string line;
        while (std::getline(proc_modules, line)) {
            if (line.rfind("uinput ", 0) == 0) {
                res.uinput_module_loaded = true;
                break;
            }
        }
    }
    if (!res.uinput_module_loaded && res.uinput_exists) {
        // Built into kernel (CONFIG_INPUT_UINPUT=y) or devtmpfs static node present
        res.uinput_module_loaded = true;
    }

    // 7. Overall readiness
    bool uinput_ok = res.uinput_exists && res.uinput_readable && res.uinput_writable;
    bool event_ok = (res.event_devices_total == 0 || res.event_devices_accessible > 0);
    res.can_run_non_root = res.is_root || (uinput_ok && event_ok);

    return res;
}

std::string formatDiagnosticReport(const PermissionCheckResult& res, const SetupUdevOptions& opts) {
    std::ostringstream oss;

    auto tag_ok = [&](const std::string& text) {
        if (opts.color)
            return "\033[1;32m[OK]\033[0m " + text;
        return "[OK] " + text;
    };
    auto tag_missing = [&](const std::string& text) {
        if (opts.color)
            return "\033[1;31m[MISSING]\033[0m " + text;
        return "[MISSING] " + text;
    };
    auto tag_warn = [&](const std::string& text) {
        if (opts.color)
            return "\033[1;33m[WARNING]\033[0m " + text;
        return "[WARNING] " + text;
    };

    oss << "Ten Flying Fingers (TFF) - Linux Permission & udev Setup\n";
    oss << "========================================================\n\n";

    // 1. User info
    oss << "Current User:      " << res.username << " (UID: " << res.uid << ", GID: " << res.gid
        << ")";
    if (res.is_root && !res.sudo_user.empty()) {
        oss << " [invoked via sudo by '" << res.sudo_user << "']";
    }
    oss << "\n";

    // 2. Group membership
    oss << "'input' group:     ";
    if (res.is_root) {
        oss << tag_ok("Root user bypasses group restrictions") << "\n";
    } else if (res.in_input_group_active) {
        oss << tag_ok("Active member of 'input' group") << "\n";
    } else if (res.in_input_group_configured) {
        oss << tag_warn("Configured in 'input' group, but session token is not yet active") << "\n";
    } else {
        oss << tag_missing("User is NOT in 'input' group") << "\n";
    }

    // 3. /dev/uinput status
    oss << "/dev/uinput:       ";
    if (!res.uinput_exists) {
        oss << tag_missing("Device node does not exist (kernel module 'uinput' not loaded)")
            << "\n";
    } else if (res.uinput_readable && res.uinput_writable) {
        oss << tag_ok("Accessible (read/write)") << "\n";
    } else {
        oss << tag_missing("Permission denied (cannot read/write /dev/uinput)") << "\n";
    }

    // 4. /dev/input/event* status
    oss << "Event devices:     ";
    if (res.event_devices_total == 0) {
        oss << tag_warn("No /dev/input/event* devices currently found") << "\n";
    } else if (res.event_devices_accessible == res.event_devices_total) {
        oss << tag_ok("All " + std::to_string(res.event_devices_total) + " device(s) accessible")
            << "\n";
    } else if (res.event_devices_accessible > 0) {
        oss << tag_warn(std::to_string(res.event_devices_accessible) + " of " +
                        std::to_string(res.event_devices_total) + " device(s) accessible")
            << "\n";
    } else {
        oss << tag_missing("0 of " + std::to_string(res.event_devices_total) +
                           " device(s) accessible")
            << "\n";
    }

    // 5. udev rule status
    oss << "udev rule:         ";
    if (res.udev_rule_installed) {
        oss << tag_ok("Installed at " + res.installed_rule_path) << "\n";
    } else {
        oss << tag_missing(opts.rule_path + " not found") << "\n";
    }

    oss << "\n--------------------------------------------------------\n";
    oss << "Status: ";
    if (res.can_run_non_root) {
        if (opts.color) {
            oss << "\033[1;32mREADY FOR NON-ROOT EXECUTION\033[0m\n";
        } else {
            oss << "READY FOR NON-ROOT EXECUTION\n";
        }
        oss << "TFF can run seamlessly under your user account without sudo.\n";
    } else {
        if (opts.color) {
            oss << "\033[1;31mNON-ROOT EXECUTION NOT READY\033[0m\n";
        } else {
            oss << "NON-ROOT EXECUTION NOT READY\n";
        }
        oss << "Permissions are insufficient to run TFF without sudo.\n";

        oss << "\nRecommended Actions:\n";
        int step = 1;

        if (!res.in_input_group_configured && !res.is_root) {
            oss << "  " << step++ << ". Add your user to the 'input' group:\n";
            oss << "     sudo usermod -aG input " << res.username << "\n\n";
        }

        if (!res.udev_rule_installed) {
            oss << "  " << step++ << ". Install udev rules for /dev/uinput and /dev/input:\n";
            oss << "     sudo tff setup-udev --install\n\n";
        }

        if (res.in_input_group_configured && !res.in_input_group_active && !res.is_root) {
            oss << "  " << step++ << ". Activate your new group membership:\n";
            oss << "     newgrp input    # (for current shell)\n";
            oss << "     Or log out and log back in to your desktop session.\n\n";
        }
    }

    return oss.str();
}

static bool runCommand(const std::string& cmd, std::string& err_msg) {
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        int exit_code = WIFEXITED(ret) ? WEXITSTATUS(ret) : ret;
        err_msg = "Command failed with exit code " + std::to_string(exit_code) + ": " + cmd;
        return false;
    }
    return true;
}

bool reloadUdevRules(std::string& err_msg) {
    // 1. Ensure kernel module uinput is loaded
    std::string mod_err;
    runCommand("modprobe uinput 2>/dev/null || true", mod_err);

    // 2. Reload udev control rules and trigger
    std::string udev_cmd = "udevadm control --reload-rules && udevadm trigger";
    if (!runCommand(udev_cmd, err_msg)) {
        return false;
    }
    return true;
}

bool installUdevRule(const SetupUdevOptions& opts, std::string& err_msg) {
    // 1. Ensure parent directory of target rule path exists
    std::filesystem::path rule_p(opts.rule_path);
    if (rule_p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(rule_p.parent_path(), ec);
        if (ec) {
            err_msg =
                "Failed to create directory " + rule_p.parent_path().string() + ": " + ec.message();
            return false;
        }
    }

    // 2. Write udev rule file
    std::ofstream rule_file(opts.rule_path);
    if (!rule_file.is_open()) {
        err_msg = "Failed to open " + opts.rule_path + " for writing: " + std::strerror(errno);
        return false;
    }
    rule_file << getUdevRuleContent();
    rule_file.close();

    chmod(opts.rule_path.c_str(), 0644);

    // 3. Optional: Write modules-load.d entry if directory exists
    std::filesystem::path mod_p(opts.modules_load_path);
    if (mod_p.has_parent_path() && std::filesystem::exists(mod_p.parent_path())) {
        std::ofstream mod_file(opts.modules_load_path);
        if (mod_file.is_open()) {
            mod_file << getModulesLoadContent();
            mod_file.close();
            chmod(opts.modules_load_path.c_str(), 0644);
        }
    }

    // 4. Reload udev if requested
    if (opts.reload) {
        return reloadUdevRules(err_msg);
    }
    return true;
}

}  // namespace udev
}  // namespace tff

#endif  // !defined(PICO_BUILD)
