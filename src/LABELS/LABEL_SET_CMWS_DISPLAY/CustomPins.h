// CustomPins.h - Pin configuration for CMWS_DISPLAY
//
// Managed by the Label Creator Custom Pins editor.
// Unknown defines are preserved at the bottom of this file.

#pragma once

// --- Feature Enables --------------------------------------
#define ENABLE_TFT_GAUGES                        1              // Enable TFT gauge rendering (only if TFT displays are present)
#define ENABLE_PCA9555                           0              // Enable PCA9555 I/O expander logic (only if PCA chips are present)

// --- RS485 Communication ----------------------------------
#define RS485_TX_PIN                             19             // GPIO for TX (connect to RS485 board DI)
#define RS485_RX_PIN                             21             // GPIO for RX (connect to RS485 board RO)
#define RS485_DE_PIN                             -1             // Direction enable pin (-1 if auto-direction)

// --- RS485 Test Pins --------------------------------------
#define RS485_TEST_BUTTON_GPIO                   0              // Test button GPIO (-1 to disable)
#define RS485_TEST_SWITCH_GPIO                   -1             // Test switch GPIO (-1 to disable)
#define RS485_TEST_LED_GPIO                      -1             // Test LED GPIO (-1 to disable)
