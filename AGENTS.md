# Notes for agents

Build, test and contribution rules are in [CONTRIBUTING.md](CONTRIBUTING.md). This file holds what
is not obvious from the code.

## RP2040 firmware: test on real hardware

CI only cross-compiles the firmware. No emulator covers the PIO-USB host port, so changes to the
Pico SDK, TinyUSB, Pico-PIO-USB or `src/platform/rp2040/` need the hardware rig described in
[docs/hardware_testing.md](docs/hardware_testing.md).

The rig is in the maintainer's LAN: `ssh root@upboard.fritz.box`.

- Feather RP2040 USB Host: its USB-C port is on hub port `1-7.1` of the UpBoard, its USB-A host
  port gets the UpBoard's OTG gadget (`/dev/hidg0`, UDC `dwc3.1.auto`) as keyboard.
- The UpBoard has `arm-none-eabi-gcc` and CMake. Building there is faster than a CI round trip
  (about 4.5 minutes):
  `PICO_SDK_PATH=<sdk> scripts/build_rp2040.sh <build-dir>`. Use your own work tree, for
  example under `/root/`; leave `/home/tff2` alone.
- After an UpBoard reboot the gadget is gone: run `setup_fake_keyboard.sh`.

Test steps:

1. Flash: `scripts/reboot_rp2040_bootloader.py`, then copy the UF2 to the `RPI-RP2` drive
   (`scripts/deploy_rp2040.sh` does both).
2. Check `cat /sys/class/udc/dwc3.1.auto/state`. It must say `configured`. `addressed` means the
   RP2040 did not finish enumerating the gadget, and the test suite will hang or fail with
   `BrokenPipeError` on `/dev/hidg0`.
3. Run `python3 test_tff_automated.py` (30 cases). Compare against the firmware of `main`.
4. Cold start, to check that enumeration is reliable: `uhubctl -l 1-7 -p 1 -a cycle`.

When enumeration fails:

- Cross-check with CircuitPython (UF2 files in `/root/circuitpython/`): if it enumerates the
  gadget, the rig is fine and the firmware is at fault.
- TFF's `printf` goes to the UART, which is not connected. To see TinyUSB host logs, build with
  `CMAKE_BUILD_TYPE=Debug`, `CFG_TUH_LOG_LEVEL=1`, `CFG_TUD_LOG_LEVEL=3` and
  `CFG_TUSB_DEBUG_PRINTF` pointing to a function that buffers the text (core 1 runs the host
  stack) and writes it to CDC from core 0. Read it from `/dev/ttyACM0`.
- November 2026: Pico-PIO-USB 0.7.2 lost the handshake in the status stage of control transfers,
  and enumeration stopped with `process_set_config: ASSERT FAILED`. 0.7.3 fixed it (#140).
