#include <Arduino.h>
static bool hc165_initialized = false;
static uint8_t plPin, cpPin, qhPin;
static uint8_t hc165_numBits = 8;  // default to 8 bits if not specified

void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH, uint8_t numBits) {
  plPin = pinPL;
  cpPin = pinCP;
  qhPin = pinQH;
  hc165_numBits = numBits;
  if (hc165_numBits == 0 || hc165_numBits > 64) hc165_numBits = 8;  // safe fallback

  pinMode(plPin, OUTPUT);
  pinMode(cpPin, OUTPUT);
  pinMode(qhPin, INPUT);

  digitalWrite(plPin, HIGH);
  digitalWrite(cpPin, LOW);
  hc165_initialized = true;
}

uint64_t HC165_read() {
  if (!hc165_initialized) return 0xFFFFFFFFFFFFFFFFULL;

  uint64_t result = 0;

  digitalWrite(plPin, LOW);
  delayMicroseconds(5);
  digitalWrite(plPin, HIGH);
  delayMicroseconds(1);

  for (uint8_t i = 0; i < hc165_numBits; i++) {
    result <<= 1;
    result |= digitalRead(qhPin);

    digitalWrite(cpPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(cpPin, LOW);
    delayMicroseconds(1);
  }

  return result;
}
