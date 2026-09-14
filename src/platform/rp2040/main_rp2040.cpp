#include "rp2040_platform.h"
#include <iostream>

int main() {
    std::cout << "TFF-like Keyboard Remapping - RP2040 Implementation\n";
    std::cout << "===================================================\n\n";

    // Initialize the RP2040 platform
    RP2040Platform platform;
    if (!platform.initialize()) {
        std::cerr << "Failed to initialize RP2040 platform\n";
        return 1;
    }

    std::cout << "Starting main event loop...\n";
    std::cout << "Press Ctrl+C to exit\n\n";

    // Run the main event loop
    platform.run();

    // Cleanup (only reached on graceful shutdown)
    platform.cleanup();
    return 0;
}