#include "rp2040_platform.h"
#include <cstdio>

int main() {
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