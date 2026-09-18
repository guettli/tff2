# Fish completion for Ten Flying Fingers (tff, tff2, tff_linux)

for prog in tff tff2 tff_linux
    complete -c $prog -e

    # Subcommands
    complete -c $prog -n "__fish_use_subcommand" -a monitor -d "Interactive live event monitor and chord debugger"
    complete -c $prog -n "__fish_use_subcommand" -a cheatsheet -d "Display visual terminal cheat sheet or markdown table"
    complete -c $prog -n "__fish_use_subcommand" -a validate -d "Validate a combos YAML configuration file"
    complete -c $prog -n "__fish_use_subcommand" -a setup-udev -d "Inspect permissions or install udev rules for non-root execution"
    complete -c $prog -n "__fish_use_subcommand" -a combos -d "Run remapper with specified combos and devices"
    complete -c $prog -n "__fish_use_subcommand" -a list -d "List all discovered keyboards and exit"
    complete -c $prog -n "__fish_use_subcommand" -a help -d "Show help message"
    complete -c $prog -n "__fish_use_subcommand" -a version -d "Show program version"

    # Global options
    complete -c $prog -s c -l config -r -d "Path to combos YAML configuration file"
    complete -c $prog -s d -l device -r -d "Path to input evdev device (/dev/input/event*)"
    complete -c $prog -s g -l grab -d "Exclusively grab input keyboards"
    complete -c $prog -l no-grab -d "Do not grab device (events still pass to OS)"
    complete -c $prog -l hotplug -d "Enable dynamic inotify keyboard hotplugging"
    complete -c $prog -l no-hotplug -d "Disable dynamic inotify keyboard hotplugging"
    complete -c $prog -s w -l watch-config -d "Watch configuration file for live changes via inotify"
    complete -c $prog -s l -l list -d "List all discovered keyboards and exit"
    complete -c $prog -s v -l verbose -d "Print detailed key down/up event logs"
    complete -c $prog -l plain -d "Disable ANSI color codes and formatting"
    complete -c $prog -l no-color -d "Disable ANSI color codes"
    complete -c $prog -s h -l help -d "Show help message and exit"
    complete -c $prog -s V -l version -d "Show program version and exit"

    # Subcommand: monitor
    complete -c $prog -n "__fish_seen_subcommand_from monitor" -l no-deltas -d "Disable timing deltas in live event monitor"
    complete -c $prog -n "__fish_seen_subcommand_from monitor" -l no-emitted -d "Disable emitted virtual key lines in live event monitor"

    # Subcommand: cheatsheet
    complete -c $prog -n "__fish_seen_subcommand_from cheatsheet" -l markdown -d "Output cheat sheet formatted as GitHub Markdown"
    complete -c $prog -n "__fish_seen_subcommand_from cheatsheet" -l md -d "Output cheat sheet formatted as GitHub Markdown"

    # Subcommand: setup-udev
    complete -c $prog -n "__fish_seen_subcommand_from setup-udev" -s i -l install -d "Install udev rules to /etc/udev/rules.d/ (requires sudo)"
    complete -c $prog -n "__fish_seen_subcommand_from setup-udev" -s p -l print -d "Print recommended udev rules to stdout"
    complete -c $prog -n "__fish_seen_subcommand_from setup-udev" -l check -d "Check user permissions and device status"
    complete -c $prog -n "__fish_seen_subcommand_from setup-udev" -l rule-path -r -d "Custom destination path for udev rules"
end
