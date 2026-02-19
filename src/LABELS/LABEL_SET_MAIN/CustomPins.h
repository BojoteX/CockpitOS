// CustomPins.h - Pin configuration for MAIN
//
// Managed by the Label Creator Custom Pins editor.
// Unknown defines are preserved at the bottom of this file.

#pragma once

// --- Feature Enables --------------------------------------
#define ENABLE_TFT_GAUGES                        0              // Enable TFT gauge rendering (only if TFT displays are present)
#define ENABLE_PCA9555                           1              // Enable PCA9555 I/O expander logic (only if PCA chips are present)

// --- I2C Bus ----------------------------------------------
#define SDA_PIN                                  PIN(8)         // I2C data line for PCA9555 expanders
#define SCL_PIN                                  PIN(9)         // I2C clock line for PCA9555 expanders

// --- WS2812 Addressable LEDs ------------------------------
#define WS2812B_PIN                              PIN(35)        // Data line for WS2812B LED strip

// --- TM1637 7-Segment Displays ----------------------------
#define LA_DIO_PIN                               PIN(39)        // Left Angle display data
#define LA_CLK_PIN                               PIN(37)        // Left Angle display clock
#define RA_DIO_PIN                               PIN(40)        // Right Angle display data
#define RA_CLK_PIN                               PIN(37)        // Right Angle display clock

// --- Mode Switch ------------------------------------------
#define MODE_SWITCH_PIN                          PIN(33)        // HID mode selector switch (rotary or toggle)
