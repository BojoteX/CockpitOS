// **************************************************************
// GN1640 Utils
//
//
//
// **************************************************************

#define GN1640_CLK_HALF_PERIOD_US 100 // Was 500 before 

// ---- GN1640 Buffered State ----
static uint8_t rowShadow[8] = {0};  // desired state
static uint8_t rowActive[8] = {0};  // current hardware state
static bool refreshPending = false;
static unsigned long lastUpdateTime = 0;

static uint8_t gn1640_clkPin, gn1640_dioPin;
// Precisely matched timings
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
  delayMicroseconds(100); // Matches proven STOP_DELAY
}

void GN1640_sendByte(uint8_t data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(gn1640_clkPin, LOW);
    digitalWrite(gn1640_dioPin, (data & 0x01) ? HIGH : LOW);
    delayMicroseconds(GN1640_CLK_HALF_PERIOD_US); // Matches CLK_HALF_PERIOD exactly
    digitalWrite(gn1640_clkPin, HIGH);
    delayMicroseconds(GN1640_CLK_HALF_PERIOD_US); // Matches CLK_HALF_PERIOD exactly
    data >>= 1;
  }
  digitalWrite(gn1640_clkPin, LOW); // Wait for ACK (ignored)
}

void GN1640_command(uint8_t cmd) {
  GN1640_startCondition();
  GN1640_sendByte(cmd);
  GN1640_stopCondition();
}

void GN1640_init(uint8_t clkPin, uint8_t dioPin) {
  gn1640_clkPin = clkPin;
  gn1640_dioPin = dioPin;

  pinMode(gn1640_clkPin, OUTPUT);
  pinMode(gn1640_dioPin, OUTPUT);
  delay(100);

  GN1640_command(0x48);  // Correct addressing setup (CRITICAL)
  GN1640_command(0x44);  // FIXED ADDRESS MODE (CRITICAL)

  // Explicitly clear RAM at addresses 0-3
  for (uint8_t addr = 0; addr < 4; addr++) {
    GN1640_startCondition();
    GN1640_sendByte(0xC0 | addr);
    GN1640_sendByte(0x00);
    GN1640_stopCondition();
  }
  GN1640_command(0x8F); // <-- Maximum brightness here!
}

void GN1640_setLED(uint8_t row, uint8_t col, bool state) {

  // Return if we dont have the caution advisory
  if(!hasCA) return;

  if (row > 7 || col > 7) return;

  uint8_t bit = (1 << col);
  bool changed = false;

  if (state && !(rowShadow[row] & bit)) {
    rowShadow[row] |= bit;
    changed = true;
  } else if (!state && (rowShadow[row] & bit)) {
    rowShadow[row] &= ~bit;
    changed = true;
  }

  if (changed) {
    refreshPending = true;
    lastUpdateTime = millis();
  }
}


void GN1640_write(uint8_t column, uint8_t value) {
  GN1640_command(0x44); // Fixed address mode
  GN1640_startCondition();
  GN1640_sendByte(0xC0 | column);
  GN1640_sendByte(value);
  GN1640_stopCondition();
}

void GN1640_clearAll() {
  GN1640_allOff();
}

void GN1640_allOff() {
  debugPrintln("⚫ Turning ALL LEDs OFF (simultaneously)");
  uint8_t used[8] = {0};
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType != DEVICE_GN1640T) continue;
    used[panelLEDs[i].info.gn1640Info.column] = 1;
  }
  for (uint8_t col = 0; col < 8; col++) {
    if (used[col]) {
      GN1640_write(col, 0x00);
    }
  }
}

void GN1640_allOn() {
  debugPrintln("🔆 Turning ALL LEDs ON (simultaneously)");
  uint8_t buffer[8] = {0};
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType != DEVICE_GN1640T) continue;
    uint8_t col = panelLEDs[i].info.gn1640Info.column;
    uint8_t row = panelLEDs[i].info.gn1640Info.row;
    buffer[col] |= (1 << row);
  }
  for (uint8_t col = 0; col < 8; col++) {
    GN1640_write(col, buffer[col]);
  }
}

void GN1640_sweep() {
  GN1640_sweepPanel();
}

void GN1640_sweepPanel() {
  debugPrintln("🔍 Starting GN1640 panel sweep...");
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType != DEVICE_GN1640T) continue;

    GN1640_clearAll();
    uint8_t col = panelLEDs[i].info.gn1640Info.column;
    uint8_t row = panelLEDs[i].info.gn1640Info.row;

    GN1640_write(col, (1 << row));

    debugPrintf("🟢 LED ON: %s → GRID %d, SEG %d → addr=0x%02X, bit=%d\n",
      panelLEDs[i].label, col, row, 0xC0 | col, row);

    delay(100);
  }
  GN1640_clearAll();
  debugPrintln("✅ Sweep complete.");
}

void GN1640_testPattern() {
  GN1640_allOff();
  GN1640_allOn();
  debugPrintln("🔁 Test cycle complete. Waiting 5s...");
}

// GN1640T Helper Implementation
void GN1640_setAllLEDs(bool state) {
    if (state) GN1640_allOn();
    else GN1640_allOff();
}

void GN1640_clear() {
  for (uint8_t row = 0; row < 8; row++) {
    rowShadow[row] = 0;
    rowActive[row] = 255;  // Force flush
  }
  refreshPending = true;
  lastUpdateTime = millis();
}

void GN1640_tick() {
  if (!refreshPending) return;
  if (millis() - lastUpdateTime < 2) return;

  for (uint8_t row = 0; row < 8; row++) {
    if (rowShadow[row] != rowActive[row]) {
      GN1640_startCondition();
      GN1640_sendByte(0xC0 | row);
      GN1640_sendByte(rowShadow[row]);
      GN1640_stopCondition();
      rowActive[row] = rowShadow[row];
    }
  }

  refreshPending = false;
}

bool GN1640_sendByteWithAck(uint8_t data) {
  // clock out 8 data bits…
  for (int i = 0; i < 8; i++) {
    digitalWrite(gn1640_clkPin, LOW);
    digitalWrite(gn1640_dioPin, (data & 0x01) ? HIGH : LOW);
    delayMicroseconds(GN1640_CLK_HALF_PERIOD_US);
    digitalWrite(gn1640_clkPin, HIGH);
    delayMicroseconds(GN1640_CLK_HALF_PERIOD_US);
    data >>= 1;
  }

  // release for ACK
  digitalWrite(gn1640_clkPin, LOW);
  pinMode(gn1640_dioPin, INPUT_PULLUP);
  delayMicroseconds(1);

  // ninth clock → sample ACK
  digitalWrite(gn1640_clkPin, HIGH);
  delayMicroseconds(GN1640_CLK_HALF_PERIOD_US);
  bool ack = (digitalRead(gn1640_dioPin) == LOW);

  // finish
  digitalWrite(gn1640_clkPin, LOW);
  pinMode(gn1640_dioPin, OUTPUT);
  return ack;
}

bool GN1640_detect(uint8_t clkPin, uint8_t dioPin) {
  // stash pins
  gn1640_clkPin = clkPin;
  gn1640_dioPin = dioPin;
  pinMode(gn1640_clkPin, OUTPUT);
  pinMode(gn1640_dioPin, OUTPUT);
  digitalWrite(gn1640_clkPin, HIGH);
  digitalWrite(gn1640_dioPin, HIGH);

  // probe
  GN1640_startCondition();
  bool present = GN1640_sendByteWithAck(0x44);   // fixed-address mode cmd
  GN1640_stopCondition();

  return present;
}
