// HT1622 RAM Walker - Interactive Mapping Tool (LEFT LCD)
// ESP32-S2, Arduino Core 3.2.x

const uint8_t PIN_DATA = 18;
const uint8_t PIN_WR   = 21;
const uint8_t PIN_CS   = 33;
const uint8_t PIN_BL   = 2;

void ht_writeBit(uint8_t wr, uint8_t data, bool bit) {
  digitalWrite(data, bit);
  delayMicroseconds(2);
  digitalWrite(wr, LOW);
  delayMicroseconds(2);
  digitalWrite(wr, HIGH);
  delayMicroseconds(2);
}
void ht_sendCmd(uint8_t cs, uint8_t wr, uint8_t data, uint8_t cmd) {
  digitalWrite(cs, LOW); delayMicroseconds(2);
  // Send Command ID: 100 (MSB first)
  ht_writeBit(wr, data, 1);
  ht_writeBit(wr, data, 0);
  ht_writeBit(wr, data, 0);
  // Send 8-bit Command Code (MSB first)
  for (int i = 7; i >= 0; i--) ht_writeBit(wr, data, (cmd >> i) & 1);
  // Send 9th bit (Don't Care, use 0)
  ht_writeBit(wr, data, 0);
  digitalWrite(cs, HIGH); delayMicroseconds(4);
}
void ht_writeNibble(uint8_t cs, uint8_t wr, uint8_t data, uint8_t addr, uint8_t nibble) {
  digitalWrite(cs, LOW);
  // Write ID: 101 (MSB first)
  ht_writeBit(wr, data, 1);
  ht_writeBit(wr, data, 0);
  ht_writeBit(wr, data, 1);
  // 6-bit address (MSB first)
  for (int i = 5; i >= 0; i--) ht_writeBit(wr, data, (addr >> i) & 1);
  // 4-bit data, LSB first
  for (int b = 0; b < 4; b++) ht_writeBit(wr, data, (nibble >> b) & 1);
  digitalWrite(cs, HIGH);
}
void ht_clearDisplay(uint8_t cs, uint8_t wr, uint8_t data) {
  // Write 0x0 to all 64 nibbles (clear all segments)
  digitalWrite(cs, LOW);
  ht_writeBit(wr, data, 1);
  ht_writeBit(wr, data, 0);
  ht_writeBit(wr, data, 1);
  for (int i = 0; i < 6; i++) ht_writeBit(wr, data, 0); // addr=0
  for (int i = 0; i < 64; i++) for (int b = 0; b < 4; b++) ht_writeBit(wr, data, 0);
  digitalWrite(cs, HIGH);
}
void ht_init(uint8_t cs, uint8_t wr, uint8_t data) {
  ht_sendCmd(cs, wr, data, 0x00); // SYS DIS
  ht_sendCmd(cs, wr, data, 0x18); // RC 32K
  ht_sendCmd(cs, wr, data, 0x29); // BIAS, 1/4 Bias, 1/8 Duty
  ht_sendCmd(cs, wr, data, 0x01); // SYS EN
  ht_sendCmd(cs, wr, data, 0x03); // LCD ON
  ht_clearDisplay(cs, wr, data);
}

void wait_for_enter() {
  Serial.println("Press ENTER to continue...");
  while (!Serial.available()) delay(5);
  while (Serial.available()) Serial.read();
}

void setup() {
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_WR, OUTPUT);
  pinMode(PIN_DATA, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_WR, HIGH);
  digitalWrite(PIN_DATA, HIGH);

  Serial.begin(115200);
  delay(3000);
  Serial.println("\n--- HT1622 RAM Walker: LEFT LCD (DATA=34, WR=35, CS=36) ---");
  Serial.println("Each segment is addressed as RAM[addr] bit b, label: SEG_addr_b");
  Serial.println("Look at your LCD, write down what turns on for each step!");

  ht_init(PIN_CS, PIN_WR, PIN_DATA);
  delay(120);

  for (uint8_t addr = 0; addr < 64; addr++) {
    for (uint8_t bit = 0; bit < 4; bit++) {
      ht_clearDisplay(PIN_CS, PIN_WR, PIN_DATA);
      uint8_t value = (1 << bit); // Only this bit ON
      ht_writeNibble(PIN_CS, PIN_WR, PIN_DATA, addr, value);

      // --- Label this segment for you to write down ---
      Serial.print("SEG_");
      Serial.print(addr);
      Serial.print("_");
      Serial.print(bit);
      Serial.print(" : RAM addr ");
      Serial.print(addr);
      Serial.print(", bit ");
      Serial.print(bit);
      Serial.println();
      wait_for_enter();
    }
  }

  ht_clearDisplay(PIN_CS, PIN_WR, PIN_DATA);
  Serial.println("DONE. All segments OFF.");
}

void loop() {}
