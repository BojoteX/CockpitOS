// GN1640T Standalone Panel Sweep Test (No external headers)
#include <Arduino.h>

#define CLK_PIN 37
#define DIO_PIN 36

uint8_t gn1640_clkPin;
uint8_t gn1640_dioPin;

// === LED Definition ===
struct LEDEntry {
  const char* label;
  uint8_t column; // GRID
  uint8_t row;    // SEG
};

const LEDEntry panelLEDs[] = {
  {"CA_CK_SEAT",  0, 0},
  {"CA_APU_ACC",  1, 0},
  {"CA_BATT_SW",  2, 0},
  {"CA_FCS_HOT",  0, 1},
  {"CA_GEN_TIE",  1, 1},
  {"CA_DASH_1",   2, 1},
  {"CA_FUEL_LO",  0, 2},
  {"CA_FCES",     1, 2},
  {"CA_DASH_2",   2, 2},
  {"CA_L_GEN",    0, 3},
  {"CA_R_GEN",    1, 3},
  {"CA_DASH_3",   2, 3},
};

const uint16_t panelLEDsCount = sizeof(panelLEDs) / sizeof(panelLEDs[0]);

// === INTERNAL LOW-LEVEL FUNCTIONS ===
void GN1640_startCondition() {
  digitalWrite(gn1640_dioPin, HIGH);
  digitalWrite(gn1640_clkPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(gn1640_dioPin, LOW);
  digitalWrite(gn1640_clkPin, LOW);
}

void GN1640_stopCondition() {
  digitalWrite(gn1640_clkPin, HIGH);
  delayMicroseconds(1);
  digitalWrite(gn1640_dioPin, HIGH);
  delayMicroseconds(100);
}

void GN1640_sendByte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(gn1640_clkPin, LOW);
    digitalWrite(gn1640_dioPin, (data & 0x01) ? HIGH : LOW);
    delayMicroseconds(500);
    digitalWrite(gn1640_clkPin, HIGH);
    delayMicroseconds(500);
    data >>= 1;
  }
  digitalWrite(gn1640_clkPin, LOW);
}

void GN1640_command(uint8_t cmd) {
  GN1640_startCondition();
  GN1640_sendByte(cmd);
  GN1640_stopCondition();
}

void GN1640_write(uint8_t column, uint8_t value) {
  GN1640_command(0x44);
  GN1640_startCondition();
  GN1640_sendByte(0xC0 | column);
  GN1640_sendByte(value);
  GN1640_stopCondition();
}

// === USER-FACING FUNCTIONS ===
void GN1640_allOff() {
  Serial.println("⚫ Turning ALL LEDs OFF (simultaneously)");
  uint8_t buffer[8] = {0};
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    buffer[panelLEDs[i].column] |= 0x00;
  }
  for (uint8_t col = 0; col < 8; col++) {
    GN1640_write(col, buffer[col]);
  }
}
void GN1640_init(uint8_t clkPin, uint8_t dioPin) {
  gn1640_clkPin = clkPin;
  gn1640_dioPin = dioPin;
  pinMode(clkPin, OUTPUT);
  pinMode(dioPin, OUTPUT);
  delay(100);
  Serial.begin(115200);
  Serial.println("🔧 GN1640 standalone init...");
  GN1640_command(0x48);
  GN1640_command(0x44);
  GN1640_clearAll();
  GN1640_command(0x8F);
  Serial.println("✅ Init complete.");
}

void GN1640_clearAll() {
  GN1640_allOff();
}

void GN1640_allOn() {
  Serial.println("🔆 Turning ALL LEDs ON (simultaneously)");
  uint8_t buffer[8] = {0};
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    buffer[panelLEDs[i].column] |= (1 << panelLEDs[i].row);
  }
  for (uint8_t col = 0; col < 8; col++) {
    GN1640_write(col, buffer[col]);
  }
}


void GN1640_setSingleLED(const char* label, bool state) {
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    if (strcmp(label, panelLEDs[i].label) == 0) {
      Serial.printf("🔸 Setting LED %s to %s\n", label, state ? "ON" : "OFF");
      GN1640_write(panelLEDs[i].column, state ? (1 << panelLEDs[i].row) : 0x00);
      return;
    }
  }
  Serial.printf("⚠️ LED '%s' not found\n", label);
}

void GN1640_sweepPanel() {
  Serial.println("🔍 Starting GN1640 CA panel sweep...");
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    GN1640_clearAll();
    GN1640_write(panelLEDs[i].column, (1 << panelLEDs[i].row));
    Serial.printf("🟢 LED ON: %s → GRID %d, SEG %d → addr=0x%02X, bit=%d\n",
      panelLEDs[i].label,
      panelLEDs[i].column,
      panelLEDs[i].row,
      0xC0 | panelLEDs[i].column,
      panelLEDs[i].row);
    delay(500);
  }
  GN1640_clearAll();
  Serial.println("✅ Sweep complete.");
}

void GN1640_testPattern() {
  GN1640_clearAll();
  delay(500);
  GN1640_allOn();
  delay(2000);
  GN1640_clearAll();
  delay(500);
  GN1640_sweepPanel();
  delay(500);
  GN1640_clearAll();
  delay(500);
  GN1640_allOn();
  delay(2000);
  GN1640_clearAll();
  Serial.println("🔁 Test cycle complete. Waiting 5s...");
  delay(5000);
}

void setup() {
  GN1640_init(CLK_PIN, DIO_PIN);
}

void loop() {
  GN1640_testPattern();
}
