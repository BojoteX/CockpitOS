#include <Wire.h>

// Valid Lolin S2 Pins (two groups, uncomment to test)
// const int scanPins[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const int scanPins[] = {15, 16, 17, 18, 21, 33, 34, 35, 36, 37, 38, 39, 40};

// Configurable parameters (extend if probing non-standard PCAs)
#define PCA9555_MIN_ADDR 0x20
#define PCA9555_MAX_ADDR 0x27

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("🎯 Full scan for SDA/SCL combinations");

  for (int i = 0; i < sizeof(scanPins) / sizeof(scanPins[0]); i++) {
    for (int j = 0; j < sizeof(scanPins) / sizeof(scanPins[0]); j++) {
      if (i == j) continue;

      int sda = scanPins[i];
      int scl = scanPins[j];

      Wire.begin(sda, scl);
      Wire.setClock(400000);  // 400kHz Fast Mode 
      delay(100);

      bool found = false;

      for (uint8_t addr = PCA9555_MIN_ADDR; addr <= PCA9555_MAX_ADDR; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
          Serial.print("✅ I²C device detected at 0x");
          Serial.print(addr, HEX);
          Serial.print(" with 🧪 SDA = GPIO ");
          Serial.print(sda);
          Serial.print(", SCL = GPIO ");
          Serial.println(scl);
          found = true;
        }
      }

      Wire.end();
      delay(200);
    }
  }

  Serial.println("\n🏁 Finalized scan.");
}

void loop() {
  
}