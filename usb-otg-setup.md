# USB-OTG Setup for Automated Testing

For automated testing without physical keyboard:

1. **Connect USB-OTG adapter** to RP2040 USB-C port (device port)
2. **Use libcomposite** to create fake HID keyboard:
   
3. **Configure gadget** to appear as keyboard to RP2040
4. **Send programmatic events** via /dev/hidg0 to test TFF combinations
