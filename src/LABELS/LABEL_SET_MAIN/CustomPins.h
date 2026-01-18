// CustomPins.h - Configuration PINS for Main Panel (TEK)

#pragma once

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         0 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            1 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// I2C Pins
#define SDA_PIN                               PIN(8)
#define SCL_PIN                               PIN(9)

// HID Switch / Selector
#define MODE_SWITCH_PIN                       PIN(33)

// Main Panel Pins
#define LA_DIO_PIN                            PIN(39)
#define LA_CLK_PIN                            PIN(37)
#define RA_DIO_PIN                            PIN(40)
#define RA_CLK_PIN                            PIN(37)
#define WS2812B_PIN							  PIN(35)