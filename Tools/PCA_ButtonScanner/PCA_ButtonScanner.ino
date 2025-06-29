#include <Wire.h>

#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_FREQ 400000

// Configurable parameters
#define PCA9555_MIN_ADDR 0x20
#define PCA9555_MAX_ADDR 0x27

// Cache up to 8 devices
static uint8_t detectedPcas[8];
static uint8_t numDetected = 0;
static uint8_t prevPort0[8];
static uint8_t prevPort1[8];

// Scan I2C bus and find all PCA9555 devices
void scanPCA9555Devices() {
  numDetected = 0;
  for (uint8_t addr = PCA9555_MIN_ADDR; addr <= PCA9555_MAX_ADDR; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      detectedPcas[numDetected++] = addr;
      Serial.printf("✅ Found PCA9555 at 0x%02X\n", addr);

      // Initialize previous states
      prevPort0[numDetected-1] = 0xFF;
      prevPort1[numDetected-1] = 0xFF;

      // Configure both ports as inputs
      Wire.beginTransmission(addr);
      Wire.write(0x06); // Config register
      Wire.write(0xFF); // Port0 as input
      Wire.write(0xFF); // Port1 as input
      Wire.endTransmission();
    }
  }
}

// Read both input ports from a PCA9555
bool readPCA9555(uint8_t addr, uint8_t &port0, uint8_t &port1) {
  Wire.beginTransmission(addr);
  Wire.write(0x00); // Input register start
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, (uint8_t)2) != 2) return false;
  port0 = Wire.read();
  port1 = Wire.read();
  return true;
}

// Pretty print ports state
void printPortsState(uint8_t p0, uint8_t p1) {
  Serial.print("[p0:");
  for (int8_t i = 7; i >= 0; --i) Serial.print((p0 >> i) & 1);
  Serial.print(" | p1:");
  for (int8_t i = 7; i >= 0; --i) Serial.print((p1 >> i) & 1);
  Serial.print("]");
}

// Setup
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("🔍 Scanning I²C Bus for PCA9555 devices...");

  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
  scanPCA9555Devices();
  if (numDetected == 0) Serial.println("⚠️ No PCA9555 devices found.");
}

// Loop
void loop() {
  for (uint8_t i = 0; i < numDetected; i++) {
    uint8_t addr = detectedPcas[i];
    uint8_t port0, port1;
    if (readPCA9555(addr, port0, port1)) {
      if (port0 != prevPort0[i] || port1 != prevPort1[i]) {
        Serial.printf("⚡ PCA 0x%02X ", addr);
        printPortsState(port0, port1);
        Serial.println();

        // Detect changes
        for (uint8_t b = 0; b < 8; b++) {
          bool prev = (prevPort0[i] >> b) & 0x01;
          bool curr = (port0 >> b) & 0x01;
          if (prev != curr) {
            Serial.printf("  → Port0 Bit%d %s\n", b, curr ? "RELEASED" : "PRESSED");
          }
        }
        for (uint8_t b = 0; b < 8; b++) {
          bool prev = (prevPort1[i] >> b) & 0x01;
          bool curr = (port1 >> b) & 0x01;
          if (prev != curr) {
            Serial.printf("  → Port1 Bit%d %s\n", b, curr ? "RELEASED" : "PRESSED");
          }
        }

        prevPort0[i] = port0;
        prevPort1[i] = port1;
      }
    }
  }

  delay(50); // Light debounce
}
