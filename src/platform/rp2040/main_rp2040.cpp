#include "rp2040_platform.h"
#include <cstdio>

#ifdef PICO_BUILD
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#endif

int main() {
#ifdef PICO_BUILD
    // Pico-PIO-USB requires RP2040 system clock to be 120 MHz (or 240 MHz)
    // for exact USB Full-Speed 12 Mbps / Low-Speed 1.5 Mbps bit-banging timing.
    set_sys_clock_khz(120000, true);
    stdio_init_all();
#endif

    printf("TFF-like Keyboard Remapping - RP2040 Implementation\n");
    printf("===================================================\n\n");

    // Initialize the RP2040 platform
    RP2040Platform platform;
    if (!platform.initialize()) {
        printf("Failed to initialize RP2040 platform\n");
        return 1;
    }

    printf("Starting main event loop...\n");

    // Run the main event loop
    platform.run();

    // Cleanup (only reached on graceful shutdown)
    platform.cleanup();
    return 0;
}