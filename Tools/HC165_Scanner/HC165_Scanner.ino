// 74HC165 Generic Bit Scanner - 48 Bit Version
#define HC165_QH   33  // Serial data output (QH)
#define HC165_CP   34  // Clock input
#define HC165_PL   35  // Latch input (active-low)

#include <Arduino.h>

#define HC165_NUM_BITS 48   // Scan up to 8 chips

static bool hc165_initialized = false;
static uint8_t plPin, cpPin, qhPin;

void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH) {
  plPin = pinPL;
  cpPin = pinCP;
  qhPin = pinQH;
  pinMode(plPin, OUTPUT);
  pinMode(cpPin, OUTPUT);
  pinMode(qhPin, INPUT);
  digitalWrite(plPin, HIGH);
  digitalWrite(cpPin, LOW);
  hc165_initialized = true;
}

uint64_t HC165_readBits(uint8_t numBits) {
  if (!hc165_initialized || numBits == 0 || numBits > 64) return 0xFFFFFFFFFFFFFFFFULL;
  uint64_t result = 0;
  digitalWrite(plPin, LOW);      // Latch inputs
  delayMicroseconds(5);
  digitalWrite(plPin, HIGH);
  delayMicroseconds(1);

  for (uint8_t i = 0; i < numBits; i++) {
    result <<= 1;
    result |= digitalRead(qhPin);
    digitalWrite(cpPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(cpPin, LOW);
    delayMicroseconds(1);
  }
  return result;
}

// --- State Tracking ---
static uint64_t prevBits = 0xFFFFFFFFFFFFFFFFULL;

void printBits(const char* prefix, uint64_t value, uint8_t numBits) {
  Serial.print(prefix);
  Serial.print(": ");
  for (int i = numBits - 1; i >= 0; --i) {
    Serial.print((value >> i) & 1 ? '1' : '0');
  }
  Serial.println();
}

void printBitChanges(uint64_t prev, uint64_t curr, uint8_t numBits) {
  uint64_t changed = prev ^ curr;
  for (uint8_t i = 0; i < numBits; ++i) {
    if (changed & (1ULL << i)) {
      Serial.print("  BIT ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print((prev & (1ULL << i)) ? "1" : "0");
      Serial.print(" -> ");
      Serial.println((curr & (1ULL << i)) ? "1" : "0");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  HC165_init(HC165_PL, HC165_CP, HC165_QH);
  Serial.print("74HC165 Generic Bit Scanner Ready, scanning ");
  Serial.print(HC165_NUM_BITS);
  Serial.println(" bits");
}

void loop() {
  uint64_t bits = HC165_readBits(HC165_NUM_BITS);

  if (bits != prevBits) {
    printBits("[HC165]", bits, HC165_NUM_BITS);
    printBitChanges(prevBits, bits, HC165_NUM_BITS);
    prevBits = bits;
    Serial.println();
  }
  delay(3);
}
