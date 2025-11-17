// **************************************************************
// TM1637 - LED + Key Scan Driver
// **************************************************************

// Commands
#define TM1637_CMD_SET_ADDR  0xC0
#define TM1637_CMD_DISP_CTRL 0x88

// -------------------------------------------------------------------
// LOW-LEVEL BUS
// -------------------------------------------------------------------

void tm1637_start(TM1637Device& dev) {
    // START: ensure idle high, then DIO low while CLK high
    pinMode(dev.dioPin, OUTPUT);
    digitalWrite(dev.dioPin, HIGH);
    digitalWrite(dev.clkPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(dev.dioPin, LOW);
    delayMicroseconds(2);
}

void tm1637_stop(TM1637Device& dev) {
    // STOP: CLK high, then DIO high
    pinMode(dev.dioPin, OUTPUT);
    digitalWrite(dev.clkPin, LOW);
    delayMicroseconds(2);
    digitalWrite(dev.dioPin, LOW);
    delayMicroseconds(2);
    digitalWrite(dev.clkPin, HIGH);
    delayMicroseconds(2);
    digitalWrite(dev.dioPin, HIGH);
    delayMicroseconds(2);
}

bool tm1637_writeByte(TM1637Device& dev, uint8_t b) {
    pinMode(dev.dioPin, OUTPUT);
    for (uint8_t i = 0; i < 8; ++i) {
        digitalWrite(dev.clkPin, LOW);
        digitalWrite(dev.dioPin, (b & 0x01) ? HIGH : LOW);
        delayMicroseconds(3);
        digitalWrite(dev.clkPin, HIGH);
        delayMicroseconds(3);
        b >>= 1;
    }

    // ACK bit
    digitalWrite(dev.clkPin, LOW);
    pinMode(dev.dioPin, INPUT_PULLUP);
    delayMicroseconds(3);
    digitalWrite(dev.clkPin, HIGH);
    delayMicroseconds(3);
    bool ack = (digitalRead(dev.dioPin) == LOW);
    digitalWrite(dev.clkPin, LOW);
    pinMode(dev.dioPin, OUTPUT);
    return ack;
}

// -------------------------------------------------------------------
// KEY SCAN — EXACTLY LIKE YOUR STANDALONE
// -------------------------------------------------------------------
//
// Returns an 8-bit TM1637 key scan code (0xFF = no key).
// InputControl.cpp will decode this with TM1637_KEY_CODES[] and
// do debouncing via the standalone-style state machine.

uint8_t tm1637_readKeys(TM1637Device& dev) {
    uint8_t keys = 0xFF;

    tm1637_start(dev);
    tm1637_writeByte(dev, 0x42);  // read key scan data command
    pinMode(dev.dioPin, INPUT_PULLUP);

    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(dev.clkPin, LOW);
        delayMicroseconds(3);

        int bit = digitalRead(dev.dioPin);
        if (bit)
            keys |= (uint8_t)(1U << i);
        else
            keys &= (uint8_t)~(1U << i);

        digitalWrite(dev.clkPin, HIGH);
        delayMicroseconds(3);
    }

    tm1637_stop(dev);
    return keys;
}

// -------------------------------------------------------------------
// DISPLAY SIDE (unchanged logic, just using needsUpdate flag)
// -------------------------------------------------------------------

void tm1637_updateDisplay(TM1637Device& dev) {

    tm1637_start(dev);
    tm1637_writeByte(dev, 0x40); // Auto increment mode
    tm1637_stop(dev);

    tm1637_start(dev);
    tm1637_writeByte(dev, TM1637_CMD_SET_ADDR);
    for (int i = 0; i < 6; i++) {
        tm1637_writeByte(dev, dev.ledData[i]);
    }
    tm1637_stop(dev);

    tm1637_start(dev);
    tm1637_writeByte(dev, TM1637_CMD_DISP_CTRL | 7); // max brightness, display ON
    tm1637_stop(dev);
}

void tm1637_init(TM1637Device& dev, uint8_t clkPin, uint8_t dioPin) {
    dev.clkPin = clkPin;
    dev.dioPin = dioPin;
    pinMode(clkPin, OUTPUT);
    pinMode(dioPin, OUTPUT);
    memset(dev.ledData, 0, sizeof(dev.ledData));
    dev.needsUpdate = false;
    tm1637_updateDisplay(dev);
}

const char* getTM1637Label(const TM1637Device* dev) {
    if (dev == &RA_Device)     return "RA";
    if (dev == &LA_Device)     return "LA";
    if (dev == &JETSEL_Device) return "JETTSEL";
    return "UNKNOWN";
}

void tm1637_displaySingleLED(TM1637Device& dev, uint8_t grid, uint8_t segment, bool state) {
    if (&dev == &LA_Device && !PanelRegistry_has(PanelKind::LA))      return;
    if (&dev == &RA_Device && !PanelRegistry_has(PanelKind::RA))      return;
    if (&dev == &JETSEL_Device && !PanelRegistry_has(PanelKind::JETTSEL)) return;

    if (grid < 6 && segment < 8) {
        uint8_t before = dev.ledData[grid];
        if (state)
            dev.ledData[grid] |= (uint8_t)(1u << segment);
        else
            dev.ledData[grid] &= (uint8_t)~(1u << segment);

        if (dev.ledData[grid] != before) {
            dev.needsUpdate = true;
        }
    }
}

void tm1637_clearDisplay(TM1637Device& dev) {
    memset(dev.ledData, 0, sizeof(dev.ledData));
    tm1637_updateDisplay(dev);   // immediate update (needed for init flashes)
    dev.needsUpdate = false;     // keep flag consistent
}

void tm1637_allOn(TM1637Device& dev) {
    for (int i = 0; i < 6; i++) dev.ledData[i] = 0xFF;
    tm1637_updateDisplay(dev);   // immediate update
    dev.needsUpdate = false;
}

void tm1637_allOff(TM1637Device& dev) {
    memset(dev.ledData, 0, sizeof(dev.ledData));
    tm1637_updateDisplay(dev);   // immediate update
    dev.needsUpdate = false;
}

void tm1637_allOn() {
    debugPrintln("🔆 Turning ALL TM1637 LEDs ON");
    tm1637_allOn(RA_Device);
    tm1637_allOn(LA_Device);
    tm1637_allOn(JETSEL_Device);
}

void tm1637_allOff() {
    debugPrintln("⚫ Turning ALL TM1637 LEDs OFF");
    tm1637_allOff(RA_Device);
    tm1637_allOff(LA_Device);
    tm1637_allOff(JETSEL_Device);
}

void tm1637_tick() {
    if (LA_Device.needsUpdate) {
        tm1637_updateDisplay(LA_Device);
        LA_Device.needsUpdate = false;
    }
    if (RA_Device.needsUpdate) {
        tm1637_updateDisplay(RA_Device);
        RA_Device.needsUpdate = false;
    }
    if (JETSEL_Device.needsUpdate) {
        tm1637_updateDisplay(JETSEL_Device);
        JETSEL_Device.needsUpdate = false;
    }
}