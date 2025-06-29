#include <Wire.h>

const int scanPins[] = {8, 9};

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("🎯 Escaneo completo en todas las combinaciones SDA/SCL");

  for (int i = 0; i < sizeof(scanPins) / sizeof(scanPins[0]); i++) {
    for (int j = 0; j < sizeof(scanPins) / sizeof(scanPins[0]); j++) {
      if (i == j) continue;

      int sda = scanPins[i];
      int scl = scanPins[j];

      Serial.print("\n🧪 Probando SDA = GPIO ");
      Serial.print(sda);
      Serial.print(", SCL = GPIO ");
      Serial.println(scl);

      Wire.begin(sda, scl);
      Wire.setClock(400000);  // 400kHz Fast Mode 
      delay(100);

      bool found = false;

      for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
          Serial.print("✅ Dispositivo I²C detectado en dirección 0x");
          Serial.println(addr, HEX);
          found = true;
        }
      }

      if (!found) {
        Serial.println("❌ Ninguna dirección respondió en esta combinación.");
      }

      Wire.end();
      delay(200);
    }
  }

  Serial.println("\n🏁 Escaneo finalizado.");
}

void loop() {
  // Nada, ejecución única
}