readme: in C++ for better performance and lower latency. --> no. It was done to support RP2040 usb adapter.
<!-- Done: Updated README.md to state that C++ was chosen to support embedded microcontrollers (RP2040 USB adapter hardware) while sharing a 100% identical state machine core with Linux. -->

---

readme: Add automated tests via usb to Go interface to test RP2040 after flashing the code on to that device.
<!-- Done: Updated README.md and docs/hardware_testing.md to describe automated USB-OTG hardware loop tests verifying the RP2040 firmware after flashing. -->

---

readme, top: Overall goal: keep fingers on the home row. Avoid to stretch the fingers too much. Keys
like Backspace or Esc are often hard to reach. Use overlapping keys from the home row.
<!-- Done: Added the ergonomics goal at the very top of README.md emphasizing keeping fingers on the home row, avoiding awkward stretches, and replacing hard-to-reach keys (Backspace, Esc) with home-row chords. -->

---

readme Dual-Platform Pipeline: Hard to read. Show arrows from top to buttom, not from left to right.
<!-- Done: Converted the Mermaid dual-platform pipeline diagrams in README.md from horizontal (flowchart LR) to top-to-bottom layout (flowchart TD). -->

---

readme Scenario 2: Dual-Role Tap-Hold (Caps Lock): Show that in second diagram.
And explain what it does.
<!-- Done: Added Scenario 2A (tap -> Escape) and Scenario 2B (hold -> Super) to the Mermaid sequence diagram in README.md, along with a dedicated explanatory section on dual-role tap-hold ergonomics. -->

---

readme Core: 100% shared tff::TFFEngine matching the Go tff state machine and CircuitPython RP2040 firmware
--> do not mention Go. The Go implementation is no longer used.
<!-- Done: Removed all mentions of Go and CircuitPython; updated README.md to describe tff::TFFEngine as the shared C++ state machine running identically across Linux and embedded microcontroller firmware. -->

---

readme: CLI Daemon: tff (or tff_linux)
--> why "or"?
<!-- Done: Removed "or tff_linux" from README.md and documentation; standardized on "tff" as the sole daemon command. -->

---

mise ubi backend:
> The ubi backend is deprecated. Use the GitHub backend instead.
Adapt. Remove that from docs.
<!-- Done: Replaced deprecated ubi:guettli/tff2 backend with mise use -g github:guettli/tff2 across README.md and docs/install.md. -->

---

readme: sudo ./install.sh          # System-wide (/usr/local/bin)
--> show curl command to install.
<!-- Done: Added quick curl one-liner (curl -fsSL https://raw.githubusercontent.com/guettli/tff2/main/install.sh | sudo bash) in README.md and docs/install.md. -->

---

readme
sudo tff setup-udev --install       # Install udev rules and reload udevadm
--> why are udev rules needed?
<!-- Done: Added explanation in README.md and docs/install.md explaining that udev rules grant non-root users read access to evdev keyboard devices and write access to /dev/uinput. -->

---

readme: RP2040 Version
--> Add intro about what rp2040 is.
<!-- Done: Added introduction in README.md describing the RP2040 as an affordable dual-core ARM Cortex-M0+ microcontroller acting as an inline USB hardware adapter. -->

---

the head of the yaml config file should provide a link to the json schema, so editors provide
validation/autocomplete.
Every config file in the repo should have the link to the json schema.
<!-- Done: Created schema/tff-schema.json and linked it via yaml-language-server header (# yaml-language-server: $schema=https://raw.githubusercontent.com/guettli/tff2/main/schema/tff-schema.json) in config/tff-combos.yaml and wizard generated configs. -->

---

readme: Or run the ported Go unit test suite directly:
--> ./build/test_tff_go_suite is included in build.sh? Then do not mention that at all.
<!-- Done: Removed separate mention of ./build/test_tff_go_suite from README.md; all 19 test suites run via ./build.sh and ctest. -->

---

readme: End-to-end hardware testing can be run using the UpBoard's micro-USB OTG port...
--> I use upBoard. But that is not a requirement. Write that more general.
<!-- Done: Generalized hardware testing documentation in README.md and docs/hardware_testing.md to refer to any Linux device with a USB-OTG controller. -->

---

readme: ./test_tff_automated.sh
--> better name. should include usb-otg.
<!-- Done: Renamed script to test_tff_usb_otg.sh, added a symlink for backwards compatibility, and updated documentation. -->

---

rp2040_implementation.md
--> how to copy the yaml config?
--> How to download the binary instead of building it?
<!-- Done: Added pre-compiled UF2 download instructions (curl and gh release download) and documented YAML config deployment and runtime loading on RP2040 in docs/rp2040_implementation.md. -->

---

configuration.md: TFF translates text strings directly into synthesized Linux kernel evdev input events:
--> configuration is generic. Do not mention Linux.
<!-- Done: Replaced Linux kernel evdev reference on line 144 of docs/configuration.md with a generic, cross-platform keyboard scancode description. -->

---

Compact Inline Format
tap_hold:
  capslock: [esc, super, 200]

Too hard to read. Remove that feature and the docs for that config format.
<!-- Done: Removed parser support for compact inline tap_hold format in src/core/tff_parser.cpp (with clear migration error message) and migrated all docs, tests, presets, and configs to the multi-line property format. -->

---

tg(nav)
--> too short. Remove that feature, only allow long function name.
<!-- Done: Removed "tg()" shorthand across src/core/tff_parser.cpp (with helpful error pointing to "toggle_layer(...)"), updated all docs and tests. -->

---

configuration.md "n p": toggle_layer(numpad)
--> why are quotes here? Other examples to not use quotes. Avoid quotes, if possible.
<!-- Done: Removed unnecessary quotes around n p in docs/configuration.md. -->

---

configuration.md: Programmatic C++ Usage
--> remove that part.
<!-- Done: Removed the "Programmatic C++ Usage" section from docs/configuration.md. -->

---

Is there a way to reset the state machine?

Like pressing ESC for 3 seconds?
This should be always on, without explicit config.
<!-- Done: Implemented automatic 3-second continuous ESC hold emergency state machine reset (ESC_RESET_TIMEOUT_US = 3000000LL). Resets all modal/toggle layers, tap-holds, one-shots, auto-shift, and leader buffers, and swallows ESC release. Covered in test_invariants.cpp and documented in README.md and docs/troubleshooting.md. -->
