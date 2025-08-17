#include <Arduino.h>

// List of GPIOs to test (S3, not S2, but logic is identical for valid pins)
const int testPins[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21,
  33, 34, 35, 36, 37, 38, 43, 44
};
const int NUM_PINS = sizeof(testPins) / sizeof(testPins[0]);

// Store last read value for each pin
uint8_t lastVals[NUM_PINS];

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\n[ESP32-S3 GPIO Pin Checker]");
  for (int i = 0; i < NUM_PINS; ++i) {
    pinMode(testPins[i], INPUT_PULLUP); // Use pull-up for robustness
    lastVals[i] = digitalRead(testPins[i]);
  }
  Serial.println("Touch pins to GND (LOW) to see changes. Monitoring...");
}

void loop() {
  for (int i = 0; i < NUM_PINS; ++i) {
    int val = digitalRead(testPins[i]);
    if (val != lastVals[i]) {
      Serial.printf("PIN %2d CHANGED: %d\n", testPins[i], val);
      lastVals[i] = val;
    }
  }
  delay(1); // Minimal delay for CPU sanity (nearly instant response)
}
