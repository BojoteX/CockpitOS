#include <Arduino.h>

// ONLY unassigned pins included!
const int ledProbePins[] = {
  3, 12, 35, 36, 37, 38, 39, 40
};
const int NUM_LEDS = sizeof(ledProbePins) / sizeof(ledProbePins[0]);
const int delayMs = 800;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nESP32-S2 LED MAPPER (ONLY UNASSIGNED PINS)");
  Serial.println("Stepping through each candidate pin HIGH and LOW.");
}

void loop() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    // Set all pins to INPUT (safe)
    for (int j = 0; j < NUM_LEDS; ++j) {
      pinMode(ledProbePins[j], INPUT);
      digitalWrite(ledProbePins[j], LOW); // Tri-state
    }
    // Set current pin HIGH
    pinMode(ledProbePins[i], OUTPUT);
    digitalWrite(ledProbePins[i], HIGH);
    Serial.printf("PIN %2d --> HIGH\n", ledProbePins[i]);
    delay(delayMs);

    // Set current pin LOW
    digitalWrite(ledProbePins[i], LOW);
    Serial.printf("PIN %2d --> LOW\n", ledProbePins[i]);
    delay(delayMs);
  }
  Serial.println("--- CYCLE COMPLETE --- (Repeats)");
  delay(1200);
}

