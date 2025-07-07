// TM1637 Standalone Panel Sweep Test (No external headers)
#include <Arduino.h>

#define CLK_PIN 37
#define RA_DIO_PIN 40
#define LA_DIO_PIN 39

struct TM1637Device {
  uint8_t clkPin;
  uint8_t dioPin;
  uint8_t ledData[6];
};

TM1637Device RA_Device;
TM1637Device LA_Device;

struct TM1637LEDEntry {
  const char* label;
  TM1637Device* device;
  uint8_t grid;
  uint8_t segment;
};

const TM1637LEDEntry tm1637LEDs[] = {
  {"RA_APU_FIRE",  &RA_Device, 5, 0},
  {"RA_ENG_FIRE",  &RA_Device, 0, 2},
  {"RA_RCDR_ON",   &RA_Device, 0, 1},
  {"RA_AI",        &RA_Device, 4, 1},
  {"RA_CW",        &RA_Device, 5, 1},
  {"RA_DISP",      &RA_Device, 0, 0},
  {"RA_SAM",       &RA_Device, 3, 0},
  {"RA_AAA",       &RA_Device, 4, 0},
  {"RA_DASH_L1",   &RA_Device, 1, 1},
  {"RA_DASH_L2",   &RA_Device, 2, 1},
  {"RA_DASH_L3",   &RA_Device, 3, 1},
  {"RA_DASH_R1",   &RA_Device, 1, 0},
  {"RA_DASH_R2",   &RA_Device, 2, 0},

  {"LA_GO",             &LA_Device, 0, 0},
  {"LA_NO_GO",          &LA_Device, 0, 1},
  {"LA_ENG_FIRE",       &LA_Device, 0, 2},
  {"LA_L_BLEED",        &LA_Device, 1, 1},
  {"LA_R_BLEED",        &LA_Device, 2, 1},
  {"LA_SPD_BRK",        &LA_Device, 3, 1},
  {"LA_STBY",           &LA_Device, 1, 0},
  {"LA_L_BAR_RED",      &LA_Device, 2, 0},
  {"LA_REC",            &LA_Device, 3, 0},
  {"LA_L_BAR_GREEN",    &LA_Device, 4, 0},
  {"LA_XMT",            &LA_Device, 4, 1},
  {"LA_MASTER_CAUTION", &LA_Device, 5, 0},
  {"LA_ASPJ_ON",        &LA_Device, 5, 1},
};

const uint8_t tm1637LEDCount = sizeof(tm1637LEDs) / sizeof(tm1637LEDs[0]);

// === Internal TM1637 Functions ===
#define TM1637_CMD_SET_ADDR  0xC0
#define TM1637_CMD_DISP_CTRL 0x88

void tm1637_start(TM1637Device &dev) {
  pinMode(dev.dioPin, OUTPUT);
  digitalWrite(dev.clkPin, HIGH);
  digitalWrite(dev.dioPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(dev.dioPin, LOW);
}

void tm1637_stop(TM1637Device &dev) {
  pinMode(dev.dioPin, OUTPUT);
  digitalWrite(dev.clkPin, LOW);
  delayMicroseconds(2);
  digitalWrite(dev.dioPin, LOW);
  delayMicroseconds(2);
  digitalWrite(dev.clkPin, HIGH);
  delayMicroseconds(2);
  digitalWrite(dev.dioPin, HIGH);
}

bool tm1637_writeByte(TM1637Device &dev, uint8_t b) {
  pinMode(dev.dioPin, OUTPUT);
  for (uint8_t i = 0; i < 8; ++i) {
    digitalWrite(dev.clkPin, LOW);
    digitalWrite(dev.dioPin, (b & 0x01));
    delayMicroseconds(3);
    digitalWrite(dev.clkPin, HIGH);
    delayMicroseconds(3);
    b >>= 1;
  }
  digitalWrite(dev.clkPin, LOW);
  pinMode(dev.dioPin, INPUT_PULLUP);
  delayMicroseconds(3);
  digitalWrite(dev.clkPin, HIGH);
  delayMicroseconds(3);
  bool ack = digitalRead(dev.dioPin) == 0;
  digitalWrite(dev.clkPin, LOW);
  pinMode(dev.dioPin, OUTPUT);
  return ack;
}

void tm1637_updateDisplay(TM1637Device &dev) {
  tm1637_start(dev);
  tm1637_writeByte(dev, 0x40);
  tm1637_stop(dev);
  tm1637_start(dev);
  tm1637_writeByte(dev, TM1637_CMD_SET_ADDR);
  for (int i = 0; i < 6; i++) {
    tm1637_writeByte(dev, dev.ledData[i]);
  }
  tm1637_stop(dev);
  tm1637_start(dev);
  tm1637_writeByte(dev, TM1637_CMD_DISP_CTRL | 7);
  tm1637_stop(dev);
}

void tm1637_init(TM1637Device &dev, uint8_t clkPin, uint8_t dioPin) {
  dev.clkPin = clkPin;
  dev.dioPin = dioPin;
  pinMode(clkPin, OUTPUT);
  pinMode(dioPin, OUTPUT);
  memset(dev.ledData, 0, sizeof(dev.ledData));
  tm1637_updateDisplay(dev);
}

void tm1637_setSingleLED(TM1637Device &dev, uint8_t grid, uint8_t segment, bool state) {
  if (grid < 6 && segment < 8) {
    if (state)
      dev.ledData[grid] |= (1 << segment);
    else
      dev.ledData[grid] &= ~(1 << segment);
    tm1637_updateDisplay(dev);
  }
}

void tm1637_allOff(TM1637Device &dev) {
  memset(dev.ledData, 0, sizeof(dev.ledData));
  tm1637_updateDisplay(dev);
}

void tm1637_allOn(TM1637Device &dev) {
  for (int i = 0; i < 6; i++) {
    dev.ledData[i] = 0xFF;
  }
  tm1637_updateDisplay(dev);
}

void tm1637_allOn() {
  Serial.println("🔆 Turning ALL TM1637 LEDs ON");
  tm1637_allOn(RA_Device);
  tm1637_allOn(LA_Device);
}

void tm1637_allOff() {
  Serial.println("⚫ Turning ALL TM1637 LEDs OFF");
  tm1637_allOff(RA_Device);
  tm1637_allOff(LA_Device);
}

void tm1637_sweep() {
  Serial.println("🔍 Starting TM1637 sweep...");
  for (uint8_t i = 0; i < tm1637LEDCount; i++) {
    tm1637_allOff();
    tm1637_setSingleLED(*tm1637LEDs[i].device, tm1637LEDs[i].grid, tm1637LEDs[i].segment, true);
    Serial.printf("🟢 LED ON: %s → GRID %d, SEG %d\n",
      tm1637LEDs[i].label,
      tm1637LEDs[i].grid,
      tm1637LEDs[i].segment);
      delay(50);
  }
  tm1637_allOff();
  Serial.println("✅ TM1637 sweep complete.");
}

void tm1637_testPattern() {
  tm1637_allOff();
  delay(500);
  tm1637_sweep();
  delay(500);
  tm1637_allOn();
  delay(1000);
  tm1637_allOff();
  delay(500);
}

void setup() {
  tm1637_init(RA_Device, CLK_PIN, RA_DIO_PIN);
  tm1637_init(LA_Device, CLK_PIN, LA_DIO_PIN);
  Serial.begin(115200);
  delay(200);
}

void loop() {
  tm1637_testPattern();
}