#include <Arduino.h>

// --------- CONFIGURATION ---------
#define ANALOG_PROBE_ONLY   0   // <-- SET TO 1 for pots, 0 for switches

const int pinsToScan[] = {0, 2, 4, 15, 18, 23, 32};
const int NUM_PINS = sizeof(pinsToScan) / sizeof(pinsToScan[0]);

// --- ANALOG scan state ---
int lastAnalogValue[sizeof(pinsToScan) / sizeof(pinsToScan[0])];
const int ANALOG_THRESHOLD = 10;

// --- DIGITAL scan state ---
uint8_t lastDigitalState[sizeof(pinsToScan) / sizeof(pinsToScan[0])];
unsigned long lastDigitalChange[sizeof(pinsToScan) / sizeof(pinsToScan[0])];

void setup() {
  Serial.begin(115200);
  delay(300);

#if ANALOG_PROBE_ONLY
  Serial.println("\nESP32-S2 UNIVERSAL PANEL SCANNER (ANALOG-ONLY MODE)");
  for (int i = 0; i < NUM_PINS; ++i) {
    // Leave as input; no pullup for analog.
    pinMode(pinsToScan[i], INPUT);
    lastAnalogValue[i] = analogRead(pinsToScan[i]);
  }
#else
  Serial.println("\nESP32-S2 UNIVERSAL PANEL SCANNER (DIGITAL-ONLY MODE)");
  for (int i = 0; i < NUM_PINS; ++i) {
    pinMode(pinsToScan[i], INPUT_PULLUP);
    lastDigitalState[i] = digitalRead(pinsToScan[i]);
    lastDigitalChange[i] = millis();
  }
#endif
  Serial.println("Ready. Move switches or pots and check Serial Monitor for activity.");
}

void loop() {
#if ANALOG_PROBE_ONLY
  for (int i = 0; i < NUM_PINS; ++i) {
    int value = analogRead(pinsToScan[i]);
    if (abs(value - lastAnalogValue[i]) > ANALOG_THRESHOLD) {
      Serial.printf("ANALOG: GPIO %2d changed to %4d\n", pinsToScan[i], value);
      lastAnalogValue[i] = value;
    }
  }
#else
  for (int i = 0; i < NUM_PINS; ++i) {
    uint8_t state = digitalRead(pinsToScan[i]);
    if (state != lastDigitalState[i]) {
      unsigned long now = millis();
      if (now - lastDigitalChange[i] > 20) { // debounce
        Serial.printf("DIGITAL: GPIO %2d --> %s\n",
                      pinsToScan[i], state ? "HIGH (off)" : "LOW (on)");
        lastDigitalChange[i] = now;
        lastDigitalState[i] = state;
      }
    }
  }
#endif
  delay(10); // Responsive, but not busy-loop
}

