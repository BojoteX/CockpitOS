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

// --- RS485 Communication Pins -----------------------------------------------
// Only needed for RS485 Slave or Master label sets.
#define RS485_TX_PIN                           17       // GPIO for TX (connect to RS485 board DI)
#define RS485_RX_PIN                           18       // GPIO for RX (connect to RS485 board RO)
#define RS485_DE_PIN                           -1       // GPIO for DE pin (-1 if not used)

// --- RS485 Test Pins (development only, remove for production) --------------
// All three feed into int8_t fields → use -1 for disabled.
#define RS485_TEST_BUTTON_GPIO                 -1
#define RS485_TEST_SWITCH_GPIO                 -1
#define RS485_TEST_LED_GPIO                    -1
