// 74HC165 Key Logger - Minimal Standalone
// Pin assignments (per your code)
#define HC165_PL   35
#define HC165_CP   34
#define HC165_QH   33

#include <Arduino.h>

static bool hc165_initialized = false;
static uint8_t plPin, cpPin, qhPin;

// ----------- HC165 RAW FUNCTIONS ------------
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

uint8_t HC165_read() {
  if (!hc165_initialized) return 0xFF;

  uint8_t result = 0;

  digitalWrite(plPin, LOW);
  delayMicroseconds(5);
  digitalWrite(plPin, HIGH);
  delayMicroseconds(1);

  for (int i = 0; i < 8; i++) {
    result <<= 1;
    result |= digitalRead(qhPin);

    digitalWrite(cpPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(cpPin, LOW);
    delayMicroseconds(1);
  }
  return result;
}

// ----------- BUTTON LOGGER ------------

static uint8_t prevBits = 0xFF;

void printBits(const char* prefix, uint8_t value) {
  Serial.print(prefix);
  Serial.print(": ");
  for (int i = 7; i >= 0; --i) {
    Serial.print((value >> i) & 1 ? '1' : '0');
  }
  Serial.println();
}

void logChangedBits(uint8_t prev, uint8_t curr) {
  uint8_t changed = prev ^ curr;
  for (uint8_t i = 0; i < 8; ++i) {
    if (changed & (1 << i)) {
      Serial.print("  BIT ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println((curr & (1 << i)) ? "RELEASED" : "PRESSED"); // Active LOW!
    }
  }
}

void setup() {
  Serial.begin(115200);
  HC165_init(HC165_PL, HC165_CP, HC165_QH);
  Serial.println("74HC165 Key Logger Ready");
}

void loop() {
  uint8_t bits = HC165_read();

  if (bits != prevBits) {
    printBits("[HC165]", bits);
    logChangedBits(prevBits, bits);
    prevBits = bits;
  }
  delay(3); // Fast polling
}
