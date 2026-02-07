// CustomPins.h - Configuration PINS for TEST LABEL SET

#pragma once

// --- Feature Enables ---
#define ENABLE_TFT_GAUGES                         0 // Enable TFT Gauges for this specific LABEL SET? 1 = Yes (Enable only if TFT Gauges are present in your hardware/PCB)
#define ENABLE_PCA9555                            0 // Should we enable PCA9555 logic to this LABEL SET? 1 = Yes (Enable only if PCA expanders are present in your hardware/PCB)

// REMOVE FOR PRODUCTION
#define RS485_TEST_LED_GPIO                       2 // Only use when RS485_SLAVE_ENABLED is set to 1. Choose a device address between (1-126) each slave should have a unique address
