#ifndef TFF_UDEV_H
#define TFF_UDEV_H

#include <string>
#include <vector>
#include <sys/types.h>

namespace tff {
namespace udev {

struct PermissionCheckResult {
    std::string username;
    uid_t uid = 0;
    gid_t gid = 0;
    bool is_root = false;

    // Group membership
    bool in_input_group_active = false;      // Active in current process tokens
    bool in_input_group_configured = false;  // Configured in /etc/group database

    // Device node access
    bool uinput_exists = false;
    bool uinput_readable = false;
    bool uinput_writable = false;

    // Event nodes access
    size_t event_devices_total = 0;
    size_t event_devices_accessible = 0;

    // Udev rules
    bool udev_rule_installed = false;
    std::string installed_rule_path;
    std::string installed_rule_content;

    // Kernel module
    bool uinput_module_loaded = false;

    // Overall readiness for non-root execution
    bool can_run_non_root = false;
};

struct SetupUdevOptions {
    bool install = false;
    bool print_only = false;
    bool check_only = false;
    bool reload = true;
    bool color = true;
    std::string rule_path = "/etc/udev/rules.d/99-tff.rules";
    std::string modules_load_path = "/etc/modules-load.d/uinput.conf";
};

// Returns standard udev rule content for /dev/uinput and /dev/input/event*
std::string getUdevRuleContent();

// Returns standard modules-load.d content for uinput
std::string getModulesLoadContent();

// Inspects current system permissions and device status
PermissionCheckResult checkPermissions(const std::string& rule_path = "/etc/udev/rules.d/99-tff.rules");

// Formats a human-readable diagnostic report
std::string formatDiagnosticReport(const PermissionCheckResult& res, const SetupUdevOptions& opts);

// Installs udev rule and modules-load file. Returns true on success, false on error with message.
bool installUdevRule(const SetupUdevOptions& opts, std::string& err_msg);

// Triggers udevadm reload and uinput modprobe. Returns true on success, false on error with message.
bool reloadUdevRules(std::string& err_msg);

} // namespace udev
} // namespace tff

#endif // TFF_UDEV_H
