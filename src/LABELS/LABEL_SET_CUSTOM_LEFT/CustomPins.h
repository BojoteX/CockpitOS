// CustomPins.h - Configuration PINS Custom Left

#pragma once

// I2C Pins
#define SDA_PIN                               PIN(8)
#define SCL_PIN                               PIN(9)

// --- Pins ---
#define BRAKE_PRESSURE_CS_PIN       PIN(5)  // Chip Select (Blue)
#define BRAKE_PRESSURE_MOSI_PIN     PIN(10) // SDA (Yellow)
#define BRAKE_PRESSURE_SCLK_PIN     PIN(11) // SCL (Orange)
#define BRAKE_PRESSURE_DC_PIN       PIN(7)  // Data/Command (Green)
#define BRAKE_PRESSURE_RST_PIN      -1
#define BRAKE_PRESSURE_MISO_PIN     -1
