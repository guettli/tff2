#include "tff_wizard.h"
#include "tff_engine.h"
#include "tff_parser.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

void testPresetsValidation() {
    std::cout << "Test 1: Preset retrieval and YAML validation... ";
    const auto& presets = tff::wizard::getPresets();
    assert(presets.size() >= 4);

    for (const auto& p : presets) {
        assert(!p.name.empty());
        assert(!p.description.empty());
        assert(!p.yaml_content.empty());

        tff::Config cfg;
        std::string err;
        bool ok = tff::loadYamlConfig(p.yaml_content, cfg, err);
        if (!ok) {
            std::cerr << "Validation failed for preset '" << p.name << "': " << err << "\n";
        }
        assert(ok);
        assert(cfg.settings.combo_timeout_ms > 0);
        assert(cfg.settings.tap_hold_timeout_ms > 0);
    }
    std::cout << "PASSED (" << presets.size() << " presets verified)\n";
}

void testFindPresetAndAliases() {
    std::cout << "Test 2: Preset search and alias resolution... ";

    assert(tff::wizard::findPreset("minimal") != nullptr);
    assert(tff::wizard::findPreset("MINIMAL") != nullptr);
    assert(tff::wizard::findPreset("min") != nullptr);
    assert(tff::wizard::findPreset("default") != nullptr);

    assert(tff::wizard::findPreset("vim-nav") != nullptr);
    assert(tff::wizard::findPreset("vim") != nullptr);
    assert(tff::wizard::findPreset("nav") != nullptr);

    assert(tff::wizard::findPreset("home-row-mods") != nullptr);
    assert(tff::wizard::findPreset("hrm") != nullptr);
    assert(tff::wizard::findPreset("mods") != nullptr);

    assert(tff::wizard::findPreset("full") != nullptr);
    assert(tff::wizard::findPreset("all") != nullptr);
    assert(tff::wizard::findPreset("power-user") != nullptr);

    assert(tff::wizard::findPreset("non-existent-preset-12345") == nullptr);

    std::cout << "PASSED\n";
}

void testGenerateConfigFromAnswers() {
    std::cout << "Test 3: Generating YAML from answers... ";

    // Default answers
    tff::wizard::WizardAnswers a1;
    std::string yaml1 = tff::wizard::generateConfigFromAnswers(a1);
    tff::Config cfg1;
    std::string err1;
    bool ok1 = tff::loadYamlConfig(yaml1, cfg1, err1);
    assert(ok1);
    assert(cfg1.combos.size() == 3);
    assert(cfg1.tap_hold_keys.size() == 2);  // caps_lock, space
    assert(cfg1.layers.size() == 1);         // nav

    // Full answers
    tff::wizard::WizardAnswers a2;
    a2.enable_home_row_combos = true;
    a2.enable_caps_lock_dual_role = true;
    a2.enable_space_nav_layer = true;
    a2.enable_home_row_mods = true;
    a2.enable_auto_shift = true;
    a2.enable_leader_sequences = true;
    std::string yaml2 = tff::wizard::generateConfigFromAnswers(a2);
    tff::Config cfg2;
    std::string err2;
    bool ok2 = tff::loadYamlConfig(yaml2, cfg2, err2);
    assert(ok2);
    assert(cfg2.combos.empty());
    assert(cfg2.tap_hold_keys.size() == 10);  // caps_lock, space, 8 HRM keys
    assert(cfg2.layers.size() == 1);
    assert(cfg2.auto_shift.enabled);
    assert(cfg2.leader.sequences.size() == 2);

    // Minimal empty answers
    tff::wizard::WizardAnswers a3;
    a3.enable_home_row_combos = false;
    a3.enable_caps_lock_dual_role = false;
    a3.enable_space_nav_layer = false;
    a3.enable_home_row_mods = false;
    a3.enable_auto_shift = false;
    a3.enable_leader_sequences = false;
    std::string yaml3 = tff::wizard::generateConfigFromAnswers(a3);
    tff::Config cfg3;
    std::string err3;
    bool ok3 = tff::loadYamlConfig(yaml3, cfg3, err3);
    assert(ok3);
    assert(cfg3.combos.empty());
    assert(cfg3.tap_hold_keys.empty());

    std::cout << "PASSED\n";
}

void testWizardListPresets() {
    std::cout << "Test 4: Wizard list presets output... ";
    tff::wizard::WizardOptions opts;
    opts.list_presets = true;

    std::istringstream in("");
    std::ostringstream out;
    int rc = tff::wizard::runWizard(opts, in, out);
    assert(rc == 0);
    std::string text = out.str();
    assert(text.find("Available TFF Configuration Presets") != std::string::npos);
    assert(text.find("vim-nav") != std::string::npos);
    assert(text.find("home-row-mods") != std::string::npos);
    std::cout << "PASSED\n";
}

void testWizardPrintOnly() {
    std::cout << "Test 5: Wizard print-only option... ";
    tff::wizard::WizardOptions opts;
    opts.preset_name = "minimal";
    opts.print_only = true;

    std::istringstream in("");
    std::ostringstream out;
    int rc = tff::wizard::runWizard(opts, in, out);
    assert(rc == 0);
    std::string yaml = out.str();
    assert(yaml.find("keys: j f") != std::string::npos);
    assert(yaml.find("outKeys: backspace") != std::string::npos);

    tff::Config cfg;
    std::string err;
    bool valid = tff::loadYamlConfig(yaml, cfg, err);
    assert(valid);
    std::cout << "PASSED\n";
}

void testWizardFileCreationAndBackup() {
    std::cout << "Test 6: Wizard file saving, overwrite, and backup... ";
    std::string test_dir = "build/test_wizard_dir";
    std::string test_file = test_dir + "/test-combos.yaml";
    std::string backup_file = test_file + ".bak";

    std::error_code ec;
    std::filesystem::remove_all(test_dir, ec);

    // 1. Initial write
    tff::wizard::WizardOptions opts1;
    opts1.preset_name = "minimal";
    opts1.output_path = test_file;
    opts1.non_interactive = true;

    std::istringstream in1("");
    std::ostringstream out1;
    int rc1 = tff::wizard::runWizard(opts1, in1, out1);
    assert(rc1 == 0);
    bool test1_exists = std::filesystem::exists(test_file);
    assert(test1_exists);
    bool bak1_exists = std::filesystem::exists(backup_file);
    assert(!bak1_exists);

    // 2. Overwrite with backup
    tff::wizard::WizardOptions opts2;
    opts2.preset_name = "vim-nav";
    opts2.output_path = test_file;
    opts2.force = true;  // force overwrite

    std::istringstream in2("");
    std::ostringstream out2;
    int rc2 = tff::wizard::runWizard(opts2, in2, out2);
    assert(rc2 == 0);
    bool test2_exists = std::filesystem::exists(test_file);
    assert(test2_exists);
    bool bak2_exists = std::filesystem::exists(backup_file);
    assert(bak2_exists);

    // Check that backup contains minimal and test_file contains vim-nav
    std::ifstream bak_in(backup_file);
    std::string bak_content((std::istreambuf_iterator<char>(bak_in)),
                            std::istreambuf_iterator<char>());
    assert(bak_content.find("caps_lock: [esc, super, 200]") != std::string::npos);
    assert(bak_content.find("nav:") == std::string::npos);

    std::ifstream cur_in(test_file);
    std::string cur_content((std::istreambuf_iterator<char>(cur_in)),
                            std::istreambuf_iterator<char>());
    assert(cur_content.find("nav:") != std::string::npos);

    std::filesystem::remove_all(test_dir, ec);
    std::cout << "PASSED\n";
}

void testWizardInteractiveInterview() {
    std::cout << "Test 7: Simulated interactive interview... ";
    std::string test_file = "build/test_wizard_interactive.yaml";
    std::error_code ec;
    std::filesystem::remove(test_file, ec);

    tff::wizard::WizardOptions opts;
    opts.output_path = test_file;

    // Simulate input:
    // Option: 1 (interview)
    // Q1 (home-row): y
    // Q2 (caps): y
    // Q3 (space-nav): y
    // Q4 (home-row-mods): n
    // Q5 (auto-shift): y
    // Q6 (leader): n
    std::string input_script = "1\ny\ny\ny\nn\ny\nn\n";
    std::istringstream in(input_script);
    std::ostringstream out;

    int rc = tff::wizard::runWizard(opts, in, out);
    assert(rc == 0);
    bool test_exists = std::filesystem::exists(test_file);
    assert(test_exists);

    std::ifstream file_in(test_file);
    std::string content((std::istreambuf_iterator<char>(file_in)),
                        std::istreambuf_iterator<char>());

    tff::Config cfg;
    std::string err;
    bool cfg_ok = tff::loadYamlConfig(content, cfg, err);
    assert(cfg_ok);
    assert(cfg.combos.size() == 3);
    assert(cfg.tap_hold_keys.size() == 2);
    assert(cfg.layers.size() == 1);
    assert(cfg.auto_shift.enabled);
    assert(cfg.leader.sequences.empty());

    std::filesystem::remove(test_file, ec);
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== TFF Configuration Wizard Tests ===\n";
    testPresetsValidation();
    testFindPresetAndAliases();
    testGenerateConfigFromAnswers();
    testWizardListPresets();
    testWizardPrintOnly();
    testWizardFileCreationAndBackup();
    testWizardInteractiveInterview();
    std::cout << "All TFF wizard tests PASSED!\n";
    return 0;
}
