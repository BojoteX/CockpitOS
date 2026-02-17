// CustomPins.h - Label-set-specific pin definitions for CockpitOS
//
// This file is automatically included by LabelSetConfig.h if present.
// Uncomment and set ONLY the pins your hardware actually uses.
// The PIN(X) macro converts S2 pins to S3 equivalents automatically.
//
// ============================================================================

#pragma once

// --- Feature Enables --------------------------------------------------------
#define ENABLE_TFT_GAUGES                         0  // 1 = Enable TFT gauge rendering (only if TFT displays are present)
#define ENABLE_PCA9555                            0  // 1 = Enable PCA9555 I/O expander logic (only if PCA chips are present)

// --- I2C Pins ---------------------------------------------------------------
// Used by PCA9555 expanders, OLED displays, or any I2C peripherals.
// #define SDA_PIN                                PIN(8)
// #define SCL_PIN                                PIN(9)

// --- HC165 Shift Register ---------------------------------------------------
// For reading banks of physical switches via daisy-chained 74HC165 chips.
// HC165_BITS = total number of inputs across all chained chips (8 per chip).
// Set to 0 or leave commented to disable.
// #define HC165_BITS                             16
// #define HC165_CONTROLLER_PL                    PIN(39)  // Latch (PL)
// #define HC165_CONTROLLER_CP                    PIN(38)  // Clock (CP)
// #define HC165_CONTROLLER_QH                    PIN(40)  // Data  (QH)

// --- WS2812B Addressable LED Strip ------------------------------------------
// #define WS2812B_PIN                            PIN(35)

// --- Mode Switch ------------------------------------------------------------
// HID selector switch for mode control (e.g., rotary or toggle).
// #define MODE_SWITCH_PIN                        PIN(33)

// --- TM1637 / HT16K33 7-Segment Displays -----------------------------------
// Pairs of DIO + CLK pins for each 7-segment display module.
// #define LA_DIO_PIN                             PIN(39)  // Left Angle display DIO
// #define LA_CLK_PIN                             PIN(37)  // Left Angle display CLK
// #define RA_DIO_PIN                             PIN(40)  // Right Angle display DIO
// #define RA_CLK_PIN                             PIN(37)  // Right Angle display CLK
// #define CA_DIO_PIN                             PIN(36)  // Caution Advisory display DIO
// #define CA_CLK_PIN                             PIN(37)  // Caution Advisory display CLK
// #define JETT_DIO_PIN                           PIN(8)   // Jettison Select display DIO
// #define JETT_CLK_PIN                           PIN(9)   // Jettison Select display CLK

// --- TFT Gauge Display Pins ------------------------------------------------
// Each TFT gauge needs: CS, DC, MOSI, SCLK (required) + RST, MISO (optional).
// Use -1 for unused RST/MISO pins.
//
// Example (Cabin Pressure gauge):
// #define CABIN_PRESSURE_CS_PIN                  PIN(36)
// #define CABIN_PRESSURE_DC_PIN                  PIN(18)
// #define CABIN_PRESSURE_MOSI_PIN                PIN(39)
// #define CABIN_PRESSURE_SCLK_PIN                PIN(40)
// #define CABIN_PRESSURE_RST_PIN                 -1       // Not connected
// #define CABIN_PRESSURE_MISO_PIN                -1       // Not connected
//
// Example (Brake Pressure gauge):
// #define BRAKE_PRESSURE_CS_PIN                  PIN(5)
// #define BRAKE_PRESSURE_DC_PIN                  PIN(7)
// #define BRAKE_PRESSURE_MOSI_PIN                PIN(10)
// #define BRAKE_PRESSURE_SCLK_PIN                PIN(11)
// #define BRAKE_PRESSURE_RST_PIN                 -1
// #define BRAKE_PRESSURE_MISO_PIN                -1
//
// Example (Hydraulic Pressure gauge):
// #define HYD_PRESSURE_CS_PIN                    PIN(17)
// #define HYD_PRESSURE_DC_PIN                    PIN(3)
// #define HYD_PRESSURE_MOSI_PIN                  PIN(4)
// #define HYD_PRESSURE_SCLK_PIN                  PIN(12)
// #define HYD_PRESSURE_RST_PIN                   -1
// #define HYD_PRESSURE_MISO_PIN                  -1
//
// Example (Radar Altimeter gauge):
// #define RADARALT_CS_PIN                        PIN(10)
// #define RADARALT_DC_PIN                        PIN(11)
// #define RADARALT_MOSI_PIN                      PIN(13)
// #define RADARALT_SCLK_PIN                      PIN(14)
// #define RADARALT_RST_PIN                       -1
// #define RADARALT_MISO_PIN                      -1
//
// Example (Battery gauge):
// #define BATTERY_CS_PIN                         PIN(36)
// #define BATTERY_DC_PIN                         PIN(13)
// #define BATTERY_MOSI_PIN                       PIN(8)
// #define BATTERY_SCLK_PIN                       PIN(9)
// #define BATTERY_RST_PIN                        PIN(12)

// --- IFEI LCD Panel Pins ----------------------------------------------------
// Parallel-interface LCD pairs (left + right) for the IFEI display.
// #define DATA0_PIN                              PIN(34)  // Left LCD data
// #define WR0_PIN                                PIN(35)  // Left LCD write enable
// #define CS0_PIN                                PIN(36)  // Left LCD chip select
// #define DATA1_PIN                              PIN(18)  // Right LCD data
// #define WR1_PIN                                PIN(21)  // Right LCD write enable
// #define CS1_PIN                                PIN(33)  // Right LCD chip select

// --- IFEI Backlight Pins ----------------------------------------------------
// #define BL_GREEN_PIN                           PIN(1)   // Day mode backlight
// #define BL_WHITE_PIN                           PIN(2)   // White backlight
// #define BL_NVG_PIN                             PIN(4)   // NVG mode backlight

// --- ALR-67 Countermeasures Panel -------------------------------------------
// #define ALR67_STROBE_1                         PIN(16)
// #define ALR67_STROBE_2                         PIN(17)
// #define ALR67_STROBE_3                         PIN(21)
// #define ALR67_STROBE_4                         PIN(37)
// #define ALR67_DataPin                          PIN(38)
// #define ALR67_BACKLIGHT_PIN                    PIN(14)

// --- RS485 Communication Pins -----------------------------------------------
// Only needed for RS485 Slave or Master label sets.
// #define RS485_TX_PIN                           17       // GPIO for TX (connect to RS485 board DI)
// #define RS485_RX_PIN                           18       // GPIO for RX (connect to RS485 board RO)
// #define RS485_DE_PIN                           -1       // GPIO for DE pin (-1 if not used)

// --- RS485 Test Pins (development only, remove for production) --------------
// All three feed into int8_t fields → use -1 for disabled.
// #define RS485_TEST_BUTTON_GPIO                 -1
// #define RS485_TEST_SWITCH_GPIO                 -1
// #define RS485_TEST_LED_GPIO                    -1
