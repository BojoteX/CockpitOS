// **************************************************************
// TM1637 - LED + Key Scan Driver
// **************************************************************

// Commands
#define TM1637_CMD_SET_ADDR  0xC0
#define TM1637_CMD_DISP_CTRL 0x88

static constexpr uint8_t MAX_TM1637_HW = 8;
static TM1637Device g_tmDevices[MAX_TM1637_HW];
static uint8_t      g_tmCount = 0;

TM1637Device* TM1637_getOrCreate(uint8_t clkPin, uint8_t dioPin) {
    // 1) Look for an existing device with the same pins
    for (uint8_t i = 0; i < g_tmCount; ++i) {
        TM1637Device& dev = g_tmDevices[i];
        if (dev.clkPin == clkPin && dev.dioPin == dioPin) {
            return &dev;
        }
    }

    // 2) Create a new one if there is space
    if (g_tmCount >= MAX_TM1637_HW) {
        debugPrintf("TM1637: registry full, cannot register CLK=%u DIO=%u\n", clkPin, dioPin);
        return nullptr;
    }

    TM1637Device* dev = &g_tmDevices[g_tmCount++];
    tm1637_init(*dev, clkPin, dioPin);
    return dev;
}

TM1637Device* TM1637_findByPins(uint8_t clkPin, uint8_t dioPin) {
    for (uint8_t i = 0; i < g_tmCount; ++i) {
        TM1637Device& dev = g_tmDevices[i];
        if (dev.clkPin == clkPin && dev.dioPin == dioPin) {
            return &dev;
        }
    }
    return nullptr;
}

TM1637Device* TM1637_findByDIO(uint8_t dioPin) {
    for (uint8_t i = 0; i < g_tmCount; ++i) {
        TM1637Device& dev = g_tmDevices[i];
        if (dev.dioPin == dioPin) {
            return &dev;
        }
    }
    return nullptr;
}

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

void tm1637_displaySingleLED(TM1637Device& dev, uint8_t grid, uint8_t segment, bool state) {
    if (grid >= 6 || segment >= 8) return;

    uint8_t before = dev.ledData[grid];
    if (state)
        dev.ledData[grid] |= (uint8_t)(1u << segment);
    else
        dev.ledData[grid] &= (uint8_t)~(1u << segment);

    if (dev.ledData[grid] != before) {
        dev.needsUpdate = true;
    }
}

void tm1637_clearDisplay(TM1637Device& dev) {
    memset(dev.ledData, 0, sizeof(dev.ledData));
    tm1637_updateDisplay(dev);   // immediate update (needed for init flashes)
    dev.needsUpdate = false;     // keep flag consistent
}

void tm1637_allOn(TM1637Device& dev) {
    for (int i = 0; i < 6; i++) dev.ledData[i] = 0xFF;
    tm1637_updateDisplay(dev);
}

void tm1637_allOff(TM1637Device& dev) {
    memset(dev.ledData, 0, sizeof(dev.ledData));
    tm1637_updateDisplay(dev);
}

void tm1637_allOn() {
    debugPrintln("🔆 Turning ALL TM1637 LEDs ON");
    for (uint8_t i = 0; i < g_tmCount; ++i) {
        tm1637_allOn(g_tmDevices[i]);
    }
}

void tm1637_allOff() {
    debugPrintln("⚫ Turning ALL TM1637 LEDs OFF");
    for (uint8_t i = 0; i < g_tmCount; ++i) {
        tm1637_allOff(g_tmDevices[i]);
    }
}

void tm1637_tick() {
    for (uint8_t i = 0; i < g_tmCount; ++i) {
        TM1637Device& dev = g_tmDevices[i];
        if (dev.needsUpdate) {
            tm1637_updateDisplay(dev);
            dev.needsUpdate = false;
        }
    }
}

uint8_t TM1637_getDeviceCount() {
    return g_tmCount;
}

TM1637Device* TM1637_getDeviceAt(uint8_t index) {
    if (index >= g_tmCount) return nullptr;
    return &g_tmDevices[index];
}
