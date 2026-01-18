// CustomPins.h - Configuration PINS Left Panel Controller

#pragma once

// --- PINs for HC165 Shift Register with the Left Panel Controller ---
#define HC165_BITS                 40   // Number of bits in HC165 shift register (0 = disabled)
#define HC165_CONTROLLER_PL        PIN(39)  // Latch (PL)
#define HC165_CONTROLLER_CP        PIN(38)  // Clock (CP)  
#define HC165_CONTROLLER_QH        PIN(36)  // Data (QH) 
