// Bootloader.h - Remote bootloader entry for firmware updates
#pragma once

// Enter ROM bootloader for firmware updates
// Chip-specific implementation - works on S2/S3/C3/C6/H2
// ESP32 Classic will print error (hardware limitation)
void enterBootloaderMode();
