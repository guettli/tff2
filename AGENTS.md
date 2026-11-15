# Notes for agents

Build, test and contribution rules are in [CONTRIBUTING.md](CONTRIBUTING.md). This file holds what
is not obvious from the code.

## RP2040 firmware: test on real hardware

No emulator covers the PIO-USB host port, so firmware changes are tested on a real board: the
**Hardware Test** workflow (`.github/workflows/hardware.yml`) builds the firmware on a self-hosted
runner, flashes it, runs `test_tff_automated.py` (30 cases) and checks 3 cold starts. It runs on
pushes to `main` and on same-repo PRs that touch firmware paths, never on PRs from forks. It is
not part of CI: when the runner is offline the job only waits, CI and Auto Release go on.

The runner is in the maintainer's LAN: `ssh root@upboard.fritz.box`
([docs/hardware_testing.md](docs/hardware_testing.md) describes the rig).

- Feather RP2040 USB Host: its USB-C port is on hub port `1-7.1` of the UpBoard, its USB-A host
  port gets the UpBoard's OTG gadget (`/dev/hidg0`, UDC `dwc3.1.auto`) as keyboard.
- Set up with `scripts/hw-runner/setup.sh <registration-token>`: user `gh-runner` (runner service in
  `/opt/actions-runner`), label `tff-rp2040-hw`, the gadget as systemd unit
  `tff-fake-keyboard.service`, and the root helper `/usr/local/sbin/tff-hw` (the only thing
  `gh-runner` may run with sudo).
- `sudo tff-hw flash <uf2>` flashes (1200-baud touch → `RPI-RP2`) and waits until the RP2040 shows
  up as keyboard and the gadget is `configured`; `sudo tff-hw cold-start` power-cycles the board
  with `uhubctl`; `sudo tff-hw status` prints the rig state and the kernel log.
- The UpBoard has `arm-none-eabi-gcc` and CMake, so for debugging, building there is faster than a
  CI round trip (about 4.5 minutes): `PICO_SDK_PATH=<sdk> scripts/build_rp2040.sh <build-dir>`.
  Use your own work tree under `/root/`; leave `/home/tff2` and `/home/gh-runner` alone.

Gadget state (`cat /sys/class/udc/dwc3.1.auto/state`): `configured` is good. `addressed` means
the RP2040 did not finish enumerating the gadget; the test suite then hangs or fails with
`BrokenPipeError` on `/dev/hidg0`.

When enumeration fails:

- Cross-check with CircuitPython (UF2 files in `/root/circuitpython/`): if it enumerates the
  gadget, the rig is fine and the firmware is at fault.
- TFF's `printf` goes to the UART, which is not connected. To see TinyUSB host logs, build with
  `CMAKE_BUILD_TYPE=Debug`, `CFG_TUH_LOG_LEVEL=1`, `CFG_TUD_LOG_LEVEL=3` and
  `CFG_TUSB_DEBUG_PRINTF` pointing to a function that buffers the text (core 1 runs the host
  stack) and writes it to CDC from core 0. Read it from `/dev/ttyACM0`.
- November 2026: Pico-PIO-USB 0.7.2 lost the handshake in the status stage of control transfers,
  and enumeration stopped with `process_set_config: ASSERT FAILED`. 0.7.3 fixed it (#140).
