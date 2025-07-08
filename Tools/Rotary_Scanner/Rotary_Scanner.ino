// --------------- Matrix Encoder Standalone Key Logger -----------------
// This sketch logs the detected pattern for a matrix-based rotary or key switch
// using a strobe/return wire matrix with active-LOW logic.
//
// Hardware: Connect each strobe output to the corresponding position on the rotary or key matrix.
// Connect the shared data return line to a digital input with INPUT_PULLUP.
//
// Pin assignments (adjust for your hardware):
const int STROBES[] = {16, 17, 21, 37};  // Example: 4 strobes for a 5-position rotary
const int NUM_STROBES = sizeof(STROBES) / sizeof(STROBES[0]);
const int DATA_PIN = 38;                 // Shared return line (active LOW)

#include <Arduino.h>

// --------- MATRIX READ FUNCTION (copied from your MatrixRotary.cpp) ---------
uint8_t MatrixRotary_readPattern(const int* strobes, int count, int dataPin) {
  uint8_t pattern = 0;
  for (int i = 0; i < count; i++) {
    digitalWrite(strobes[i], LOW);
    delayMicroseconds(1); // Allow line to settle
    if (digitalRead(dataPin) == LOW) {
      pattern |= (1 << i); // Bit set if key detected (active-low)
    }
    digitalWrite(strobes[i], HIGH);
  }
  return pattern;
}

// --------- LOGGING HELPERS ---------
void printPatternBits(uint8_t pattern, int count) {
  Serial.print("Pattern: ");
  for (int i = count - 1; i >= 0; --i) {
    Serial.print((pattern >> i) & 1 ? '1' : '0');
  }
  Serial.print(" (0x");
  Serial.print(pattern, HEX);
  Serial.println(")");
}

// --------- ARDUINO SETUP/LOOP ---------
void setup() {
  Serial.begin(115200);
  // Strobe pins as OUTPUT, default HIGH (inactive)
  for (int i = 0; i < NUM_STROBES; ++i) {
    pinMode(STROBES[i], OUTPUT);
    digitalWrite(STROBES[i], HIGH);
  }
  // Data pin as INPUT_PULLUP
  pinMode(DATA_PIN, INPUT_PULLUP);

  Serial.println("Matrix Encoder Key Logger Ready.");
}

void loop() {
  static uint8_t prevPattern = 0xFF; // Impossible value to force first print
  uint8_t currPattern = MatrixRotary_readPattern(STROBES, NUM_STROBES, DATA_PIN);

  if (currPattern != prevPattern) {
    Serial.print("[MATRIX] ");
    printPatternBits(currPattern, NUM_STROBES);

    // Optionally, decode known patterns for rotary/keys here
    // Example:
    // if (currPattern == 0x08) Serial.println("Position F");
    // else if (currPattern == 0x04) Serial.println("Position U");
    // ...etc...

    prevPattern = currPattern;
  }
  delay(3); // ~300 Hz polling (fast enough for human input)
}
