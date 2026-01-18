#pragma once

// Public API for LED Control
// void initializeLEDs(const char* activePanels[], unsigned int panelCount);
void initializeLEDs();
void setLED(const char* label, bool state, uint8_t intensity = 100, uint16_t rawValue = 0, uint16_t maxValue = 0);

// NEW: Output driver management
void scanOutputDevicePresence();   // Call during init to detect which drivers are needed
void tickOutputDrivers();          // Call each frame to flush all driver buffers
bool hasOutputDevice(uint8_t deviceType);  // Query if a device type exists in LEDMapping

// Forward lookup declaration (findLED is auto-generated in LEDControl_Lookup.h)
struct LEDMapping;
const LEDMapping* findLED(const char* label);