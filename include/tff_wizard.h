#ifndef TFF_WIZARD_H
#define TFF_WIZARD_H

#include "tff_types.h"
#include <string>
#include <vector>
#include <iostream>

namespace tff {
namespace wizard {

struct WizardPreset {
    std::string name;
    std::string description;
    std::string yaml_content;
};

struct WizardOptions {
    std::string preset_name;
    std::string output_path;
    bool force = false;
    bool print_only = false;
    bool list_presets = false;
    bool non_interactive = false;
};

struct WizardAnswers {
    bool enable_home_row_combos = true;
    bool enable_caps_lock_dual_role = true;
    bool enable_space_nav_layer = true;
    bool enable_home_row_mods = false;
    bool enable_auto_shift = false;
    bool enable_leader_sequences = false;
    int combo_timeout_ms = 40;
    int tap_hold_timeout_ms = 200;
};

// Available presets
const std::vector<WizardPreset>& getPresets();
const WizardPreset* findPreset(const std::string& name);

// Generate YAML from interview answers
std::string generateConfigFromAnswers(const WizardAnswers& answers);

// Run interactive wizard on streams
// Returns 0 on success, non-zero on error or user cancel.
int runWizard(const WizardOptions& options, std::istream& in = std::cin,
              std::ostream& out = std::cout);

// Get default output file path (~/.config/tff/tff-combos.yaml or config/tff-combos.yaml)
std::string getDefaultOutputPath();

}  // namespace wizard
}  // namespace tff

#endif  // TFF_WIZARD_H
