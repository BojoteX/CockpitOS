// CustomPins.h - Configuration PINS for IFEI

#pragma once

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         0 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            0 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// --- PINs for Jettison Select Panel with IFEI Panel ---
#define JETT_DIO_PIN                          PIN(8)
#define JETT_CLK_PIN                          PIN(9)

// IFEI Left LCD
#define DATA0_PIN                             PIN(34)
#define WR0_PIN                               PIN(35)
#define CS0_PIN                               PIN(36)

// IFEI  Right LCD
#define DATA1_PIN                             PIN(18)
#define WR1_PIN                               PIN(21)
#define CS1_PIN                               PIN(33)

// IFEI  Backlight PINs
#define BL_GREEN_PIN                          PIN(1)
#define BL_WHITE_PIN                          PIN(2)
#define BL_NVG_PIN                            PIN(4)

// --- PINs for HC165 Shift Register with IFEI Panel ---
#define HC165_BITS                 16		 // Number of bits in HC165 shift register (0 = disabled)
#define HC165_CONTROLLER_PL        PIN(39)   // Latch (PL)
#define HC165_CONTROLLER_CP        PIN(40)   // Clock (CP)
#define HC165_CONTROLLER_QH        PIN(38)   // Data (QH)

