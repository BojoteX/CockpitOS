// Pins.h - Configuration PINS for CockpitOS (Mostly placeholders) LABEL_SETS are where you define the Panel specific PINs

#pragma once

// Usually GPIOs are set in the LABEL SET corresponding to the panel you are building. Pins.h is for place holders or custom Pins

// What would be my TEST pin for default install? (not really used except for TEST label)
#define TEST_GPIO 0

// Caution Advisory GPIOs
#ifndef CA_DIO_PIN
  #define CA_DIO_PIN  -1
#endif
#ifndef CA_CLK_PIN
  #define CA_CLK_PIN  -1
#endif

// CRITICAL do not change this location, this triggers the loading of LABEL SETS
#include "src/LabelSetSelect.h"