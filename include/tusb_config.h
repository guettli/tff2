#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------+
// Common Configuration
// --------------------------------------------------------------------+

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU OPT_MCU_RP2040
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_PICO
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

// Root Hub port configuration
// Port 0: Native USB hardware controller running as USB Device (full-speed)
#ifndef CFG_TUSB_RHPORT0_MODE
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#endif

// Port 1: Pico-PIO-USB software controller running as USB Host (full-speed)
#ifndef CFG_TUSB_RHPORT1_MODE
#define CFG_TUSB_RHPORT1_MODE (OPT_MODE_HOST | OPT_MODE_FULL_SPEED)
#endif

// Enable Device stack on Native USB Port 0
#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED 1
#endif

// Enable Host stack on PIO-USB Port 1
#ifndef CFG_TUH_ENABLED
#define CFG_TUH_ENABLED 1
#endif

// Enable Pico-PIO-USB driver for TinyUSB Host
#ifndef CFG_TUH_RPI_PIO_USB
#define CFG_TUH_RPI_PIO_USB 1
#endif

// Default D+ GPIO pin for Adafruit Feather RP2040 with USB Host (GPIO 16 D+, GPIO 17 D-)
#ifndef PIO_USB_DP_PIN_DEFAULT
#define PIO_USB_DP_PIN_DEFAULT 16
#endif

// --------------------------------------------------------------------+
// Device Configuration
// --------------------------------------------------------------------+

#define CFG_TUD_ENDPOINT0_SIZE 64

// Enabled Device Classes
#define CFG_TUD_HID 1
#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

// HID buffer size
#define CFG_TUD_HID_EP_BUFSIZE 64

// --------------------------------------------------------------------+
// Host Configuration
// --------------------------------------------------------------------+

#define CFG_TUH_ENUMERATION_BUFSIZE 256

#define CFG_TUH_HUB 1
#define CFG_TUH_HID 4
#define CFG_TUH_MSC 0
#define CFG_TUH_CDC 0

#define CFG_TUH_DEVICE_MAX (CFG_TUH_HUB ? 4 : 1)

#ifdef __cplusplus
}
#endif

#endif  // _TUSB_CONFIG_H_
