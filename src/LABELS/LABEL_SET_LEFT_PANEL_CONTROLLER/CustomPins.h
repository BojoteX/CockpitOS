// CustomPins.h - Configuration PINS Left Panel Controller

#pragma once

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         0 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            0 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// --- PINs for HC165 Shift Register with the Left Panel Controller ---
#define HC165_BITS                 40   // Number of bits in HC165 shift register (0 = disabled)
#define HC165_CONTROLLER_PL        PIN(39)  // Latch (PL)
#define HC165_CONTROLLER_CP        PIN(38)  // Clock (CP)  
#define HC165_CONTROLLER_QH        PIN(36)  // Data (QH) 
