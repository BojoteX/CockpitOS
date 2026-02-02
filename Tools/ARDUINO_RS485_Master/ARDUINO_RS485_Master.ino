/*
   Arduino RS-485 Master - Uses official DCS-BIOS library
   TX1 is 18
   RX1 is 19
   Direction control PIN is 2
   
 */

#define DCSBIOS_RS485_MASTER
#define UART1_TXENABLE_PIN 2

#include "DcsBios.h"

void setup() {
    DcsBios::setup();
}

void loop() {
    DcsBios::loop();
}