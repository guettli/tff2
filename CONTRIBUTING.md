# Contributing to Ten Flying Fingers (TFF)

Thank you for contributing to Ten Flying Fingers! This document outlines our development workflow, coding standards, and verification guidelines.

---

## Architecture Overview

TFF is structured into modular layers:
- **`src/core/`**: Platform-agnostic core (`TFFEngine`, YAML parser, cheat sheet generator, live monitor). This core runs identically across Linux and embedded microcontrollers (RP2040).
- **`src/platform/linux/`**: Linux daemon using Linux `evdev` for physical input (`/dev/input/event*`), inotify for dynamic hotplugging, and `uinput` (`/dev/uinput`) for emitting virtual keyboard/mouse events.
- **`src/platform/rp2040/`**: Embedded firmware running on Raspberry Pi Pico / RP2040 with TinyUSB Host (reading keyboard input) and TinyUSB Device (presenting as a standard USB HID keyboard).
- **`tests/`**: Fast, hardware-independent C++ test suites running in under 2 seconds.

---

## Development Workflow

We follow a strict, quality-focused development workflow:

1. **Feature Branching**:
   - Create a dedicated branch off `main` for each feature or bug fix:
     ```bash
     git checkout -b feature/issue-<number>-<short-description>
     # or
     git checkout -b fix/issue-<number>-<short-description>
     ```

2. **Commit Style**:
   - Follow Conventional Commits format:
     - `feat(scope): add feature description (fixes #123)`
     - `fix(scope): resolve issue description (fixes #123)`
     - `test(scope): add test coverage`
     - `docs(scope): update documentation`
     - `refactor(scope): internal cleanup`

3. **Pull Requests & Reviews**:
   - Keep Pull Requests focused and atomic.
   - Review changes thoroughly (or request subagent review).
   - Ensure all CI workflow checks pass before merging.
   - Squash-and-merge into `main` and delete the feature branch.

---

## Code Quality & Formatting

Code formatting is strictly enforced via `.clang-format` (LLVM/Google base, C++17, 4-space indentation, 100-column line limit, with include sorting disabled to safeguard Linux input header macros).

```bash
# Format all C/C++ source and header files in-place:
cmake --build build --target format

# Verify formatting without modifying files:
cmake --build build --target format-check
```

---

## Local Verification (`scripts/check.sh`)

Before pushing your changes or opening a PR, run the unified developer check script:

```bash
# Run all pre-push checks:
# 1. clang-format check
# 2. Strict CMake compilation (-Wall -Wextra -Wpedantic -Werror)
# 3. All 15 CTest unit test suites
# 4. Cppcheck static code analysis
# 5. CLI smoke tests (help, list, validate, cheatsheet, setup-udev)
./scripts/check.sh

# Run with AddressSanitizer and UndefinedBehaviorSanitizer enabled:
./scripts/check.sh --sanitizers
```

---

## Testing Guidelines

### Unit & Integration Tests (Hardware-Independent)
- Located in `tests/test_*.cpp`.
- Must execute quickly without requiring root privileges or physical hardware.
- If you add new engine features (e.g. new chording mechanisms, layer modifiers, or timing logic), add a dedicated test file or suite under `tests/`.
- Register the new test target in `CMakeLists.txt` under `add_test(...)`.

### Hardware Testing (RP2040 USB-OTG Loop)
- If you have an UpBoard connected to an RP2040 host rig, run the 30-case automated hardware test suite:
  ```bash
  ./test_tff_automated.sh
  ```
- See [`docs/hardware_testing.md`](docs/hardware_testing.md) for full architectural details.

---

## CI Pipeline

Every push and Pull Request triggers the GitHub Actions CI pipeline:
1. **Code Formatting (`clang-format`)**: Verifies all C/C++ files adhere to `.clang-format`.
2. **Build & Test (Linux GCC)**: Builds with `-Werror`, runs all tests, verifies CLI commands, tests `install.sh`, and validates release archive packaging.
3. **Sanitizers (ASan + UBSan)**: Builds with AddressSanitizer and UndefinedBehaviorSanitizer, asserting zero memory leaks or undefined behavior.
4. **Static Analysis (Cppcheck)**: Runs deep static code analysis with `--enable=warning,style,performance,portability --error-exitcode=1`.
5. **RP2040 Firmware Cross-Compilation**: Cross-compiles the embedded firmware with `arm-none-eabi-gcc` and Raspberry Pi Pico SDK.

---

## Documentation Standards

- Update [`README.md`](README.md), [`docs/configuration.md`](docs/configuration.md), and [`docs/cookbook.md`](docs/cookbook.md) when introducing user-facing syntax or CLI flags.
- Consult [`docs/troubleshooting.md`](docs/troubleshooting.md) for diagnostic procedures and common setup issues.
- Update [`docs/man/tff.1`](docs/man/tff.1) for any new subcommands or options.
- Maintain Mermaid diagrams in documentation for visual clarity.
