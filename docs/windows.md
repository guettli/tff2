# TFF on Windows

TFF runs natively on Windows 10 and 11. It uses the same engine and YAML configuration as on
Linux. It needs no driver, and running it needs no administrator rights.

There are two programs:

| Program    | Use                                                                          |
|------------|------------------------------------------------------------------------------|
| `tffw.exe` | Remapper with a tray icon, no console window. Use this for everyday use and autostart. |
| `tff.exe`  | Command line: `init`, `validate`, `cheatsheet`, `monitor`, `autostart`, or run the remapper in a console. |

Only one remapper (`tffw.exe` or `tff.exe`) can run at a time.

## Install

Download from [GitHub Releases](https://github.com/guettli/tff2/releases):

- `tff2-<version>-windows-x64.msi`: installs into `C:\Program Files\TFF2` and adds a Start
  menu entry (asks for administrator rights once, for the installation only).
- `tff2-<version>-windows-x64.zip`: portable, no administrator rights needed; unpack anywhere
  (e.g. `%LOCALAPPDATA%\Programs\TFF2`) and start `tffw.exe`.

To update, install the new MSI; TFF can keep running. The running `tffw.exe` notices within a
few seconds that its program file was replaced and starts the new version (with the same
rights, e.g. elevated). When TFF is uninstalled, it exits. To update the ZIP, quit TFF first.

The programs are not code-signed yet, so Windows SmartScreen may warn on first start ("More
info" → "Run anyway"). Some antivirus products are suspicious of any program that hooks the
keyboard.

## First start

Start **TFF2** from the Start menu (or `tffw.exe`). On the first start TFF:

1. copies the default configuration to `%APPDATA%\tff\tff-combos.yaml` (if it doesn't exist),
2. asks whether TFF should start when you sign in.

Right-click (or click) the tray icon for the menu: pause/resume, reload or edit the
configuration, show the cheat sheet, layer notifications, start at sign-in, open the log, quit.

Changes to the configuration file are applied automatically when you save it. If the file has
an error, TFF shows a notification and keeps the previous configuration.

## Configuration

The configuration format is identical to Linux; see [configuration.md](configuration.md).
The file is looked up in this order:

1. `-c <file>` / `--config <file>` on the command line,
2. `config\tff-combos.yaml` relative to the working directory (development checkout),
3. `%APPDATA%\tff\tff-combos.yaml`,
4. `tff-combos.yaml` next to the programs (default shipped with TFF).

```powershell
tff init                         # interactive wizard, writes %APPDATA%\tff\tff-combos.yaml
tff init --preset vim-nav        # or start from a preset
tff validate                     # check the configuration
tff cheatsheet                   # show what is mapped
tff monitor                      # watch keys and chord decisions live (keys are not remapped)
tff --watch-config               # run the remapper in this console (Ctrl+C stops it)
```

Output keys are injected by scan code, so they refer to physical key positions, like on
Linux. Text snippets (`text "..."`) are typed as characters instead, so they come out right
with every keyboard layout, including characters that need AltGr (`@`, `{`, `\` on a German
keyboard). `settings: layout` isn't needed for them on Windows, and snippets may contain any
character (umlauts, `€`, emoji), not only those on `settings.layout`. Some programs (games, a
few old ones) ignore text typed as characters.

## Autostart

```powershell
tff autostart enable             # HKCU Run key, starts tffw.exe at sign-in
tff autostart status
tff autostart disable
```

Windows doesn't let a normal program send keys to programs running as administrator (UIPI).
If you want remapping inside elevated windows (admin terminals, installers, Task Manager), run
this once from an administrator terminal:

```powershell
tff autostart enable --elevated  # logon task with highest privileges, replaces the Run key
```

To go back to the Run key, run `tff autostart disable` from an administrator terminal first
(removing the logon task needs administrator rights), then `tff autostart enable`.

Uninstalling the MSI removes the Run key entry of the user who uninstalls and the logon task,
if they start the installed `tffw.exe` (not a portable copy).

## Layer notifications

When a layer is toggled on or off (`toggle_layer(...)`), TFF can show it. Layers that are only
active while a key is held are not announced. Tray menu → *Layer notifications*:

- **On-screen indicator** (default): a small box at the bottom of the screen for about a second;
  it never takes focus.
- **Windows notification**: a toast in the notification center (respects Focus Assist).
- **Off**.

## Differences to Linux

| Topic | Windows |
|---|---|
| Input | Low-level keyboard hook (`WH_KEYBOARD_LL`); all keyboards are remapped. `--device`, `--grab`, `--hotplug` and `setup-udev` don't exist. |
| Lock screen, Ctrl+Alt+Del, UAC prompts | Not remapped (Windows doesn't allow it). TFF resets its state when the session is locked or unlocked. |
| Elevated windows | Only remapped with `tff autostart enable --elevated` (or running TFF as administrator). |
| Input from other software | Remapped like physical input (e.g. AutoHotkey). Use `--ignore-injected` to leave it alone. |
| Key repeat | Holding a key repeats the remapped key with your Windows repeat delay and rate. |
| Notifications | On-screen indicator or Windows notification instead of `notify-send`. |
| Diagnostic dump | `F12` / `Pause` type the event log into the active window, as on Linux. It is also written to the log file `%LOCALAPPDATA%\tff\tff.log` (tray app). Windows 11 Notepad may lose characters of a long dump; the log file has it complete. |
| RP2040 `tff dump` | Not available on Windows yet. The RP2040 adapter itself works with Windows without any software. |

## Troubleshooting

- **Keys are typed twice / not remapped**: make sure only one TFF runs (`tff.exe` refuses to
  start while `tffw.exe` runs). Other remappers (AutoHotkey, PowerToys Keyboard Manager) may
  interfere. TFF processes their output by default; try `--ignore-injected`.
- **Remapping stops working**: Windows removes keyboard hooks that respond too slowly. TFF
  detects this and reinstalls its hook (see the log). Please report it if it happens often.
- **Stuck modifier**: hold `Esc` for 3 seconds (emergency reset), or pause/resume from the tray.
- **Logs**: `%LOCALAPPDATA%\tff\tff.log`; run `tff -v` in a console for per-key logs.

## Building from source

Needs Visual Studio 2022 or newer (or the Build Tools) with the C++ workload and CMake ≥ 3.16:

```powershell
cmake -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cd build; cpack -C Release -G ZIP     # MSI: install the WiX .NET tool, then -G WIX
```

`tff_e2e.exe` (test `test_windows_e2e`) starts `tff.exe`, types keys with `SendInput` and
checks the remapped output. It needs an interactive desktop session; run it on a real machine
or VM (`ctest -C Release -R test_windows_e2e`), or look at the "Windows End-to-End" CI job.
