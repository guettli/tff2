#include "tff_cheatsheet.h"
#include "tff_parser.h"
#include "tff_key_codes.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace tff;

static void test_empty_config() {
    Config config;
    CheatsheetOptions opts;
    opts.color = false;
    opts.markdown = false;

    std::string out = Cheatsheet::generate(config, opts);
    assert(out.find("TEN FLYING FINGERS — CHEAT SHEET") != std::string::npos);
    assert(out.find("(No combos, tap-hold keys, one-shot keys, or layers configured)") != std::string::npos);

    opts.markdown = true;
    std::string md = Cheatsheet::generate(config, opts);
    assert(md.find("# Ten Flying Fingers — Cheat Sheet") != std::string::npos);
    assert(md.find("_No combos, tap-hold keys, one-shot keys, or layers configured._") != std::string::npos);

    std::cout << "test_empty_config: PASSED\n";
}

static void test_terminal_cheatsheet_with_and_without_colors() {
    Config config;

    TapHoldKey th;
    th.key = Keys::KEY_CAPSLOCK;
    th.tap_key = Keys::KEY_ESC;
    th.hold_key = Keys::KEY_LEFTMETA;
    th.timeout_us = 200000LL;
    config.tap_hold_keys.push_back(th);

    Combo c1;
    c1.keys = {Keys::KEY_J, Keys::KEY_F};
    c1.out_keys = {Keys::KEY_BACKSPACE};
    config.combos.push_back(c1);

    Combo c2;
    c2.keys = {Keys::KEY_F, Keys::KEY_SPACE};
    c2.out_keys = {Keys::KEY_LEFTCTRL, Keys::KEY_S};
    config.combos.push_back(c2);

    Combo c3;
    c3.keys = {Keys::KEY_P, Keys::KEY_Y};
    c3.text = "println!();";
    config.combos.push_back(c3);

    // 1. Plain text (no colors)
    CheatsheetOptions opts_plain;
    opts_plain.color = false;
    std::string plain = Cheatsheet::generate(config, opts_plain);

    assert(plain.find("\033[") == std::string::npos); // No ANSI escape codes
    assert(plain.find("capslock") != std::string::npos);
    assert(plain.find("esc") != std::string::npos);
    assert(plain.find("super") != std::string::npos);
    assert(plain.find("200ms") != std::string::npos);
    assert(plain.find("j f") != std::string::npos);
    assert(plain.find("backspace") != std::string::npos);
    assert(plain.find("f space") != std::string::npos);
    assert(plain.find("ctrl + s") != std::string::npos);
    assert(plain.find("\"println!();\"") != std::string::npos);

    // 2. Colored text
    CheatsheetOptions opts_col;
    opts_col.color = true;
    std::string colored = Cheatsheet::generate(config, opts_col);
    assert(colored.find("\033[") != std::string::npos); // Has ANSI escape codes
    assert(colored.find("capslock") != std::string::npos);

    std::cout << "test_terminal_cheatsheet_with_and_without_colors: PASSED\n";
}

static void test_markdown_cheatsheet() {
    Config config;

    TapHoldKey th1;
    th1.key = Keys::KEY_CAPSLOCK;
    th1.tap_key = Keys::KEY_ESC;
    th1.hold_key = Keys::KEY_LEFTMETA;
    th1.timeout_us = 200000LL;
    config.tap_hold_keys.push_back(th1);

    TapHoldKey th2;
    th2.key = Keys::KEY_SPACE;
    th2.tap_key = Keys::KEY_SPACE;
    th2.hold_layer = "nav";
    th2.timeout_us = 180000LL;
    config.tap_hold_keys.push_back(th2);

    Combo c1;
    c1.keys = {Keys::KEY_J, Keys::KEY_F};
    c1.out_keys = {Keys::KEY_BACKSPACE};
    config.combos.push_back(c1);

    Combo c2;
    c2.keys = {Keys::KEY_F, Keys::KEY_N};
    c2.text = "hello | world"; // contains pipe to test escaping
    config.combos.push_back(c2);

    Layer nav;
    nav.name = "nav";
    nav.mappings[Keys::KEY_H] = LayerAction({Keys::KEY_LEFT});
    nav.mappings[Keys::KEY_W] = LayerAction({Keys::KEY_LEFTCTRL, Keys::KEY_RIGHT});
    nav.mappings[Keys::KEY_C] = LayerAction({}, "cargo test");
    config.layers.push_back(nav);

    CheatsheetOptions opts;
    opts.markdown = true;
    std::string md = Cheatsheet::generate(config, opts);

    assert(md.find("# Ten Flying Fingers — Cheat Sheet") != std::string::npos);
    assert(md.find("## Tap-vs-Hold Keys (Dual-Role)") != std::string::npos);
    assert(md.find("| `capslock` | `esc` | `super` | 200ms |") != std::string::npos);
    assert(md.find("| `space` | `space` | Layer: `nav` | 180ms |") != std::string::npos);
    assert(md.find("## Home Row Combos & Chords") != std::string::npos);
    assert(md.find("| `j f` | `backspace` | Single Key |") != std::string::npos);
    assert(md.find("hello \\| world") != std::string::npos); // escaped pipe
    assert(md.find("## Modal Keyboard Layers") != std::string::npos);
    assert(md.find("### Layer: `nav`") != std::string::npos);
    assert(md.find("| `h` | `left` | Single Key |") != std::string::npos);
    assert(md.find("| `w` | `ctrl + right` | Modifier Chord |") != std::string::npos);
    assert(md.find("| `c` | `\"cargo test\"` | Text Snippet |") != std::string::npos);

    std::cout << "test_markdown_cheatsheet: PASSED\n";
}

static void test_yaml_to_cheatsheet_integration() {
    std::string yaml = R"(
layers:
  nav:
    h: left
    j: down
    k: up
    l: right
    w: ctrl+right
    c: "println!();"
  numpad:
    m: kp0
    j: kp1

tap_hold:
  capslock: [esc, super, 200]
  space:
    tap: space
    layer: nav
    timeout_ms: 180

combos:
  j f: backspace
  f j: delete
  f n: "test\n"
)";

    Config config;
    std::string err;
    assert(loadYamlConfig(yaml, config, err));

    CheatsheetOptions opts;
    opts.color = false;
    std::string out = Cheatsheet::generate(config, opts);

    assert(out.find("[ Tap-vs-Hold Keys (Dual-Role) ]") != std::string::npos);
    assert(out.find("capslock") != std::string::npos);
    assert(out.find("[layer: nav]") != std::string::npos);
    assert(out.find("[ Combos & Chords ] (3 active)") != std::string::npos);
    assert(out.find("j f") != std::string::npos);
    assert(out.find("backspace") != std::string::npos);
    assert(out.find("\"test\\n\"") != std::string::npos);
    assert(out.find("[ Modal Keyboard Layers ] (2 layers)") != std::string::npos);
    assert(out.find("* Layer: nav") != std::string::npos);
    assert(out.find("* Layer: numpad") != std::string::npos);
    assert(out.find("ctrl + right") != std::string::npos);

    std::cout << "test_yaml_to_cheatsheet_integration: PASSED\n";
}

static void test_helpers() {
    assert(Cheatsheet::formatKey(Keys::KEY_ESC) == "esc");
    assert(Cheatsheet::formatKey(Keys::KEY_CAPSLOCK) == "capslock");
    assert(Cheatsheet::formatKeys({Keys::KEY_J, Keys::KEY_F}) == "j f");

    std::cout << "test_helpers: PASSED\n";
}

int main() {
    std::cout << "Running Cheatsheet Visualizer tests...\n";
    test_empty_config();
    test_terminal_cheatsheet_with_and_without_colors();
    test_markdown_cheatsheet();
    test_yaml_to_cheatsheet_integration();
    test_helpers();
    std::cout << "All 5 Cheatsheet tests passed successfully!\n";
    return 0;
}
