#!/bin/bash
# Complete automated TFF hardware testing script with USB-OTG
# Tests all key combinations from config/tff-combos.yaml end-to-end

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "============================================="
echo "   TFF Automated Hardware Testing (USB-OTG)  "
echo "============================================="
echo ""

# 1. Ensure fake keyboard gadget is setup
if [ ! -c /dev/hidg0 ] || [ ! -w /dev/hidg0 ]; then
    echo "Configuring USB-OTG fake keyboard gadget..."
    ./setup_fake_keyboard.sh
fi

# 2. Run automated test suite
echo "Running automated hardware test suite..."
if id -Gn | grep -qw input; then
    python3 test_tff_automated.py "$@"
else
    sg input -c "python3 test_tff_automated.py $*"
fi