// CustomPins.h - Configuration PINS for TEST LABEL SET

#pragma once

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         0 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            0 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// Auto-Direction Control 
#define RS485_TX_PIN                               17 // GPIO Used for TX (Here's the PIN on your ESP32 you connected to the RS485 Board TX or DI)
#define RS485_RX_PIN                               18 // GPIO Used for RX (Here's the PIN on your ESP32 you connected to the RS485 Board RX or RO)
#define RS485_DE_PIN                               -1 // \If your board has a DE pin, connect it to this GPIO, no DE pin? set -1

// REMOVE FOR PRODUCTION
#define RS485_TEST_BUTTON_GPIO                      0 // Only use when RS485_SLAVE_ENABLED is set to 1. Choose a device address between (1-126) each slave should have a unique address
#define RS485_TEST_SWITCH_GPIO                      1 // Only use when RS485_SLAVE_ENABLED is set to 1. Choose a device address between (1-126) each slave should have a unique address
#define RS485_TEST_LED_GPIO                        15 // Only use when RS485_SLAVE_ENABLED is set to 1. Choose a device address between (1-126) each slave should have a unique address
