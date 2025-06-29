#pragma once

// Public API for LED Control
// void initializeLEDs(const char* activePanels[], unsigned int panelCount);
void initializeLEDs();

void setLED(const char* label, bool state, uint8_t intensity = 100, uint16_t rawValue = 0, uint16_t maxValue = 0);

// Forward lookup declaration (findLED is auto-generated in LEDControl_Lookup.h)
struct LEDMapping;
const LEDMapping* findLED(const char* label);

