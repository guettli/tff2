# How the TFF Automated Testing Works

## Complete Solution for Testing Your my-combos.yaml

Now that we can see the RP2040 is actually connected to the UpBoard, here's exactly how the automated testing works:

## Hardware Setup

```
UpBoard → libcomposite fake keyboard → RP2040 USB-A (Host Port)
                                           ↓
                                RP2040 Processes TFF combinations
                                           ↓
                              RP2040 USB-C (Device Port) → Computer
```

## Step-by-Step Process

### 1. Fake Keyboard Creation
```bash
# Uses libcomposite to create virtual HID keyboard
modprobe libcomposite
modprobe dummy_hcd

# Creates /dev/hidg0 as fake keyboard device
```

### 2. TFF Configuration
```bash
# Converts your my-combos.yaml to JSON for RP2040
python3 tools/convert_tff_config.py /home/guettli/projects/tff/my-combos.yaml config/tff-combos.json
```

### 3. Automated Testing
```bash
# Sends fake keyboard events programmatically
python3 send_fake_events.py
```

## Testing Your Exact my-combos.yaml Mappings

### What Gets Tested:
1. **j f** → backspace (within 100ms threshold)
2. **f j** → delete (within 100ms threshold)  
3. **Sequential keys** → individual key output (outside threshold)

### How It Works:
1. UpBoard sends `'j'` key event to RP2040 USB-A port
2. UpBoard waits 50ms (within threshold)
3. UpBoard sends `'f'` key event to RP2040 USB-A port
4. RP2040 recognizes combination and outputs backspace to USB-C port
5. Whatever is connected to USB-C receives the backspace key

## Files You Have Now

- `setup_fake_keyboard.sh` - Creates libcomposite fake keyboard
- `send_fake_events.py` - Sends fake keyboard events for testing
- `test_tff_automated.sh` - Complete automated testing script
- `config/tff-combos.json` - Your converted TFF configuration

## Ready for Your Testing

Everything is prepared for you to run the actual automated test:

```bash
# Run complete automated test
./test_tff_automated.sh
```

This will:
1. Setup fake keyboard gadget
2. Convert your TFF configuration
3. Send fake keyboard events
4. Test all combinations from your my-combos.yaml

The system is ready for your actual hardware testing!