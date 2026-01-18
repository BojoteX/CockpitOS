// CustomPins.h - Configuration PINS KY58

#pragma once

#define HC165_BITS                 16   // Number of bits in HC165 shift register (0 = disabled) 
#define HC165_CONTROLLER_PL        PIN(39)   // Latch (PL)
#define HC165_CONTROLLER_CP        PIN(38)   // Clock (CP)
#define HC165_CONTROLLER_QH        PIN(40)   // Data (QH)