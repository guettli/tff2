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

// Enable Device stack
#ifndef CFG_TUD_ENABLED
#define CFG_TUD_ENABLED 1
#endif

// Enable Host stack
#ifndef CFG_TUH_ENABLED
#define CFG_TUH_ENABLED 1
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
