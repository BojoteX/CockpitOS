// Pins.h - Configuration PINS for CockpitOS

#pragma once

// ==============================================================================================================
// The PIN(X) macro is optional. Using PIN(8) or just 8 has the same effect! The PIN macro exists to support
// any custom logic needed for a pin. In our case, PIN(X) automatically converts an S2 pin to its equivalent
// S3 position. This ensures backward compatibility with backplane PCBs, allowing you to use S2 and S3 devices
// interchangeably. (I do this with TEKCreation Brain Controllers)
// ==============================================================================================================

// ===== IN THIS TOP SECTION ONLY ADD PINS, NO CONDITIONALS (OR PRE-PROCESSORS INVOLING LABEL SETS) SEE BELOW FOR CONDITIONALS =========

// General Pins (canonical S2 → mapped by PIN())
#define SDA_PIN                               PIN(8)
#define SCL_PIN                               PIN(9)
#define GLOBAL_CLK_PIN                        PIN(37)
#define CA_DIO_PIN                            PIN(36)
#define LA_DIO_PIN                            PIN(39)
#define LA_CLK_PIN                            GLOBAL_CLK_PIN
#define RA_DIO_PIN                            PIN(40)
#define RA_CLK_PIN                            GLOBAL_CLK_PIN
#define LOCKSHOOT_DIO_PIN                     PIN(35)
#define WS2812B_PIN                           LOCKSHOOT_DIO_PIN

// Misc Pins
#define MODE_SWITCH_PIN                       PIN(33)

// IFEI Left LCD
#define DATA0_PIN                             PIN(34)
#define WR0_PIN                               PIN(35)
#define CS0_PIN                               PIN(36)

// IFEI  Right LCD
#define DATA1_PIN                             PIN(18)
#define WR1_PIN                               PIN(21)
#define CS1_PIN                               PIN(33)

// IFEI  Backlight PINs
#define BL_GREEN_PIN                          PIN(1)
#define BL_WHITE_PIN                          PIN(2)
#define BL_NVG_PIN                            PIN(4)

// ALR-67 Pins
#define ALR67_STROBE_1                        PIN(16)
#define ALR67_STROBE_2                        PIN(17)
#define ALR67_STROBE_3                        PIN(21)
#define ALR67_STROBE_4                        PIN(37)
#define ALR67_DataPin                         PIN(38)
#define ALR67_DataPin                         PIN(38)
#define ALR67_BACKLIGHT_PIN                   PIN(14)

// Radar Altimeter GPIOs
#define RA_TEST_GPIO                          PIN(2)
#define RA_DEC_HEIGHT_GPIO                    PIN(3)
#define RA_INC_HEIGHT_GPIO                    PIN(4)

// ALR-67 extra pins (Analog Gauge)
// #define PRESSURE_ALT_GAUGE_PIN                PIN(18)    

// CRITICAL do not change this location, this triggers the loading of LABEL SETS
#include "src/LabelSetSelect.h"

// ===== IN THIS BOTTOM SECTION YOU CAN DO CONDITIONALS (PRE-PROCESSORS INVOLING LABEL SETS) SEE BELOW =========

// HC165 PIN per panel
#if defined(HAS_LEFT_PANEL_CONTROLLER)
  #define HC165_BITS                 40   // Number of bits in HC165 shift register (0 = disabled)
  #define HC165_CONTROLLER_PL        PIN(39)  // Latch (PL)
  #define HC165_CONTROLLER_CP        PIN(38)  // Clock (CP)  
  #define HC165_CONTROLLER_QH        PIN(36)  // Data (QH)  
#elif defined(HAS_RIGHT_PANEL_CONTROLLER)
  #define HC165_BITS                 48   // Number of bits in HC165 shift register (0 = disabled)
  #define HC165_CONTROLLER_PL        PIN(35)   // Latch (PL)
  #define HC165_CONTROLLER_CP        PIN(34)   // Clock (CP)
  #define HC165_CONTROLLER_QH        PIN(33)   // Data (QH)
#elif defined(HAS_IFEI)
  #define HC165_BITS                 16   // Number of bits in HC165 shift register (0 = disabled)
  #define HC165_CONTROLLER_PL        PIN(39)   // Latch (PL)
  #define HC165_CONTROLLER_CP        PIN(40)   // Clock (CP)
  #define HC165_CONTROLLER_QH        PIN(38)   // Data (QH)
#elif defined(HAS_ALR67)
  #define HC165_BITS                 8   // Number of bits in HC165 shift register (0 = disabled) 
  #define HC165_CONTROLLER_PL        PIN(35)   // Latch (PL)
  #define HC165_CONTROLLER_CP        PIN(34)   // Clock (CP)
  #define HC165_CONTROLLER_QH        PIN(33)   // Data (QH)
#elif defined(HAS_KY58)
  #define HC165_BITS                 16   // Number of bits in HC165 shift register (0 = disabled) 
  #define HC165_CONTROLLER_PL        PIN(39)   // Latch (PL)
  #define HC165_CONTROLLER_CP        PIN(38)   // Clock (CP)
  #define HC165_CONTROLLER_QH        PIN(40)   // Data (QH)
#elif defined(HAS_TEST_ONLY)
  #define HC165_BITS                 8   // Number of bits in HC165 shift register (0 = disabled) 
  #define HC165_CONTROLLER_PL        12   // Latch (PL)
  #define HC165_CONTROLLER_CP        11   // Clock (CP)
  #define HC165_CONTROLLER_QH        10   // Data (QH)
#else
  #define HC165_BITS                 0   // Number of bits in HC165 shift register (0 = disabled) 
  #define HC165_CONTROLLER_PL       -1
  #define HC165_CONTROLLER_CP       -1
  #define HC165_CONTROLLER_QH       -1  
#endif

/*
       S2 to S3 GPIO conversion table
       the PIN() macro does this automatically
       Simply use PIN(12) on an S2 it will 
       automatically convert it to 10 on an S3
-----------------------------------------------
| S2 Mini GPIO | S3 Mini GPIO (same position) |
| ------------ | ---------------------------- |
| 3            | 2                            |
| 5            | 4                            |
| 7            | 12                           |
| 9            | 13                           |
| 12           | 10                           |
| 2            | 3                            |
| 4            | 5                            |
| 8            | 7                            |
| 10           | 8                            |
| 13           | 9                            |
| 40           | 33                           |
| 38           | 37                           |
| 36           | 38                           |
| 39           | 43                           |
| 37           | 44                           |
| 35           | 36                           |
| 33           | 35                           |
-----------------_-----------------------------

*/