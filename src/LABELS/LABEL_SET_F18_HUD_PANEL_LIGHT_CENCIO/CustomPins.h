// CustomPins.h - Pin configuration for F18_HUD_PANEL_LIGHT_CENCIO
//
// Managed by the Label Creator Custom Pins editor.
// Unknown defines are preserved at the bottom of this file.

#pragma once

// --- Feature Enables --------------------------------------
#define ENABLE_TFT_GAUGES                        0              // Enable TFT gauge rendering (only if TFT displays are present)
#define ENABLE_PCA9555                           0              // Enable PCA9555 I/O expander logic (only if PCA chips are present)

// --- RS485 Communication ----------------------------------
#define RS485_TX_PIN                             17             // GPIO for TX (connect to RS485 board DI)
#define RS485_RX_PIN                             18             // GPIO for RX (connect to RS485 board RO)
#define RS485_DE_PIN                             -1             // Direction enable pin (-1 if auto-direction)

// --- RS485 Test Pins --------------------------------------
#define RS485_TEST_BUTTON_GPIO                   0              // Test button GPIO (-1 to disable)
#define RS485_TEST_SWITCH_GPIO                   -1             // Test switch GPIO (-1 to disable)
#define RS485_TEST_LED_GPIO                      15             // Test LED GPIO (-1 to disable)

// --- Preserved (not managed by editor) --------------------
// Used by PCA9555 expanders, OLED displays, or any I2C peripherals.

// For reading banks of physical switches via daisy-chained 74HC165 chips.
// HC165_BITS = total number of inputs across all chained chips (8 per chip).
// Set to 0 or leave commented to disable.

// HID selector switch for mode control (e.g., rotary or toggle).

// Pairs of DIO + CLK pins for each 7-segment display module.

// Each TFT gauge needs: CS, DC, MOSI, SCLK (required) + RST, MISO (optional).
// Use -1 for unused RST/MISO pins.
// Example (Cabin Pressure gauge):
// #define CABIN_PRESSURE_CS_PIN                  PIN(36)
// #define CABIN_PRESSURE_DC_PIN                  PIN(18)
// #define CABIN_PRESSURE_MOSI_PIN                PIN(39)
// #define CABIN_PRESSURE_SCLK_PIN                PIN(40)
// #define CABIN_PRESSURE_RST_PIN                 -1       // Not connected
// #define CABIN_PRESSURE_MISO_PIN                -1       // Not connected
// Example (Brake Pressure gauge):
// #define BRAKE_PRESSURE_CS_PIN                  PIN(5)
// #define BRAKE_PRESSURE_DC_PIN                  PIN(7)
// #define BRAKE_PRESSURE_MOSI_PIN                PIN(10)
// #define BRAKE_PRESSURE_SCLK_PIN                PIN(11)
// #define BRAKE_PRESSURE_RST_PIN                 -1
// #define BRAKE_PRESSURE_MISO_PIN                -1
// Example (Hydraulic Pressure gauge):
// #define HYD_PRESSURE_CS_PIN                    PIN(17)
// #define HYD_PRESSURE_DC_PIN                    PIN(3)
// #define HYD_PRESSURE_MOSI_PIN                  PIN(4)
// #define HYD_PRESSURE_SCLK_PIN                  PIN(12)
// #define HYD_PRESSURE_RST_PIN                   -1
// #define HYD_PRESSURE_MISO_PIN                  -1
// Example (Radar Altimeter gauge):
// #define RADARALT_CS_PIN                        PIN(10)
// #define RADARALT_DC_PIN                        PIN(11)
// #define RADARALT_MOSI_PIN                      PIN(13)
// #define RADARALT_SCLK_PIN                      PIN(14)
// #define RADARALT_RST_PIN                       -1
// #define RADARALT_MISO_PIN                      -1
// Example (Battery gauge):
// #define BATTERY_CS_PIN                         PIN(36)
// #define BATTERY_DC_PIN                         PIN(13)
// #define BATTERY_MOSI_PIN                       PIN(8)
// #define BATTERY_SCLK_PIN                       PIN(9)
// #define BATTERY_RST_PIN                        PIN(12)

// Parallel-interface LCD pairs (left + right) for the IFEI display.
// #define DATA0_PIN                              PIN(34)  // Left LCD data
// #define WR0_PIN                                PIN(35)  // Left LCD write enable
// #define CS0_PIN                                PIN(36)  // Left LCD chip select
// #define DATA1_PIN                              PIN(18)  // Right LCD data
// #define WR1_PIN                                PIN(21)  // Right LCD write enable
// #define CS1_PIN                                PIN(33)  // Right LCD chip select

// #define BL_GREEN_PIN                           PIN(1)   // Day mode backlight
// #define BL_WHITE_PIN                           PIN(2)   // White backlight
// #define BL_NVG_PIN                             PIN(4)   // NVG mode backlight

// #define ALR67_STROBE_1                         PIN(16)
// #define ALR67_STROBE_2                         PIN(17)
// #define ALR67_STROBE_3                         PIN(21)
// #define ALR67_STROBE_4                         PIN(37)
// #define ALR67_DataPin                          PIN(38)
// #define ALR67_BACKLIGHT_PIN                    PIN(14)

// Only needed for RS485 Slave or Master label sets.

// All three feed into int8_t fields → use -1 for disabled.
