// CustomPins.h - Configuration PINS Custom Right

#pragma once

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         1 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            1 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// I2C Pins
#define SDA_PIN                               PIN(8)
#define SCL_PIN                               PIN(9)

// HID Switch / Selector
#define MODE_SWITCH_PIN                       PIN(33)

// Caution Advisory GPIOs (override Pins.h defaults)
#undef  CA_DIO_PIN
#define CA_DIO_PIN                            PIN(36)
#undef  CA_CLK_PIN
#define CA_CLK_PIN                            PIN(37)

// --- TFT Pins for the Hyd Pressure Custom Right ---
#define HYD_PRESSURE_CS_PIN     PIN(17)  // Chip Select (Blue) *** CANT USE GPIO 8 *** Conflict with i2c BUS for PCA9555
#define HYD_PRESSURE_DC_PIN     PIN(3)   // Data/Command (Green)
#define HYD_PRESSURE_MOSI_PIN   PIN(4)   // SDA (Yellow)
#define HYD_PRESSURE_SCLK_PIN	PIN(12)  // SCL (Orange)
#define HYD_PRESSURE_RST_PIN    -1
#define HYD_PRESSURE_MISO_PIN   -1

// --- TFT Pins for the Radar Altimeter Custom Right ---
#define RADARALT_CS_PIN   PIN(10)    // Chip Select (Blue)
#define RADARALT_DC_PIN   PIN(11)    // Data/Command (Green)
#define RADARALT_MOSI_PIN PIN(13)    // SDA (Yellow)
#define RADARALT_SCLK_PIN PIN(14)    // SCL (Orange)
#define RADARALT_RST_PIN  -1
#define RADARALT_MISO_PIN -1