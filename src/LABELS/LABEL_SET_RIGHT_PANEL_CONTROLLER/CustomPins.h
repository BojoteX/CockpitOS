// CustomPins.h - Configuration PINS Right Panel Controller

#pragma once

// --- PINs for HC165 Shift Register with the Right Panel Controller ---
#define HC165_BITS                 48   // Number of bits in HC165 shift register (0 = disabled)
#define HC165_CONTROLLER_PL        PIN(35)   // Latch (PL)
#define HC165_CONTROLLER_CP        PIN(34)   // Clock (CP)
#define HC165_CONTROLLER_QH        PIN(33)   // Data (QH)

// --- TFT Pins for the Battery gauge used in the Right Panel Controller ---
#define BATTERY_MOSI_PIN   PIN(8)   // SDA (Yellow)
#define BATTERY_SCLK_PIN   PIN(9)   // SCL (Orange)
#define BATTERY_DC_PIN     PIN(13)  // Data/Command (Green)
#define BATTERY_CS_PIN     PIN(36)  // Chip Select (Blue)
#define BATTERY_RST_PIN    PIN(12)  // Used by the Right Panel Controller (UNKNOWN COLOR) 