// CustomPins.h - Configuration PINS for ALR-67

#pragma once

// ALR-67 extra pins (Analog Gauge)
// #define PRESSURE_ALT_GAUGE_PIN                PIN(18)    

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         1 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            1 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// I2C Pins
#define SDA_PIN                               PIN(8)
#define SCL_PIN                               PIN(9)

// ALR-67 Pins
#define ALR67_STROBE_1                        PIN(16)
#define ALR67_STROBE_2                        PIN(17)
#define ALR67_STROBE_3                        PIN(21)
#define ALR67_STROBE_4                        PIN(37)
#define ALR67_DataPin                         PIN(38)
#define ALR67_DataPin                         PIN(38)
#define ALR67_BACKLIGHT_PIN                   PIN(14)

// --- PINs for HC165 Shift Register with the Right Panel Controller ---
#define HC165_BITS                 8		 // Number of bits in HC165 shift register (0 = disabled) 
#define HC165_CONTROLLER_PL        PIN(35)   // Latch (PL)
#define HC165_CONTROLLER_CP        PIN(34)   // Clock (CP)
#define HC165_CONTROLLER_QH        PIN(33)   // Data (QH)

// --- TFT Pins for the Battery gauge used in the Right Panel Controller ---
#define CABIN_PRESSURE_DC_PIN   PIN(18)  // Data/Command (Green)
#define CABIN_PRESSURE_CS_PIN   PIN(36)  // Chip Select (Blue)
#define CABIN_PRESSURE_MOSI_PIN PIN(39)  // SDA (Yellow)
#define CABIN_PRESSURE_SCLK_PIN PIN(40)  // SCL (Orange)
#define CABIN_PRESSURE_RST_PIN  -1		 // Reset (White)
#define CABIN_PRESSURE_MISO_PIN -1		 // Unused