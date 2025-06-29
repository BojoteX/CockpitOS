// **************************************************************
// TM1637
//
//
//
// **************************************************************
#define TM1637_CMD_SET_ADDR  0xC0
#define TM1637_CMD_DISP_CTRL 0x88

// Static button sampling state for RIGHT annunciator (RA)
static uint16_t raBitCounters[8] = {0}; // One counter per bit
static uint16_t raTotalSamples = 0;
static const uint16_t raSampleWindowSize = 50; // Number of samples before evaluating

// Static button sampling state for LEFT annunciator (LA)
static uint16_t laBitCounters[8] = {0};
static uint16_t laTotalSamples = 0;
static const uint16_t laSampleWindowSize = 50;

// Sampling and decision parameters (tweakable)
#define TM1637_SAMPLE_WINDOW_SIZE    6     // Number of samples per evaluation
#define TM1637_MAJORITY_PERCENT     50     // Required majority (in percent) to accept a bit

bool tm1637_handleSamplingWindow(TM1637Device& dev, uint16_t& sampleCounter, uint8_t& finalKeys) {
    uint8_t rawKeys = tm1637_readKeys(dev);
    tm1637_addSample(dev, rawKeys);

    sampleCounter++;
    if (sampleCounter >= TM1637_SAMPLE_WINDOW_SIZE) {
        sampleCounter = 0;
        finalKeys = tm1637_evaluateResult(dev);
        tm1637_resetSampling(dev);
        return true; // Final decision available
    }
    return false; // Still collecting samples
}

void tm1637_addSample(TM1637Device& dev, uint8_t rawKeys) {
    if (dev.dioPin == RA_DIO_PIN) {
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (rawKeys & (1 << bit)) {
                raBitCounters[bit]++;
            }
        }
        raTotalSamples++;
    }
    else if (dev.dioPin == LA_DIO_PIN) {
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (rawKeys & (1 << bit)) {
                laBitCounters[bit]++;
            }
        }
        laTotalSamples++;
    }
}

uint8_t tm1637_evaluateResult(TM1637Device& dev) {
    uint8_t result = 0;

    if (dev.dioPin == RA_DIO_PIN && raTotalSamples > 0) {
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if ((raBitCounters[bit] * 100) > (raTotalSamples * TM1637_MAJORITY_PERCENT)) {
                result |= (1 << bit);
            }
        }
    }
    else if (dev.dioPin == LA_DIO_PIN && laTotalSamples > 0) {
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if ((laBitCounters[bit] * 100) > (laTotalSamples * TM1637_MAJORITY_PERCENT)) {
                result |= (1 << bit);
            }
        }
    }

    return result;
}

void tm1637_resetSampling(TM1637Device& dev) {
    if (dev.dioPin == RA_DIO_PIN) {
        memset(raBitCounters, 0, sizeof(raBitCounters));
        raTotalSamples = 0;
    }
    else if (dev.dioPin == LA_DIO_PIN) {
        memset(laBitCounters, 0, sizeof(laBitCounters));
        laTotalSamples = 0;
    }
}

uint8_t tm1637_readKeys(TM1637Device &dev) {
    uint8_t keys = 0;
    tm1637_start(dev);
    tm1637_writeByte(dev, 0x42);
    pinMode(dev.dioPin, INPUT_PULLUP);

    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(dev.clkPin, LOW);
        delayMicroseconds(3);
        keys |= (digitalRead(dev.dioPin) << i);
        digitalWrite(dev.clkPin, HIGH);
        delayMicroseconds(3);
    }

    tm1637_stop(dev);
    return keys;
}

void tm1637_startBothDevices() {
    tm1637_start(LA_Device);
    tm1637_start(RA_Device);
}

void tm1637_resetBothDevices() {
    // Reset LA Device
    tm1637_start(LA_Device);
    tm1637_writeByte(LA_Device, 0x40); // Data command setting
    tm1637_stop(LA_Device);

    // Reset RA Device
    tm1637_start(RA_Device);
    tm1637_writeByte(RA_Device, 0x40); // Data command setting
    tm1637_stop(RA_Device);
}

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
    tm1637_writeByte(dev, 0x40); // Auto increment
    tm1637_stop(dev);

    tm1637_start(dev);
    tm1637_writeByte(dev, TM1637_CMD_SET_ADDR);
    for (int i = 0; i < 6; i++) {
        tm1637_writeByte(dev, dev.ledData[i]);
    }
    tm1637_stop(dev);

    tm1637_start(dev);
    tm1637_writeByte(dev, TM1637_CMD_DISP_CTRL | 7); // Brightness max
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

const char* getTM1637Label(const TM1637Device* dev) {
    if (dev == &RA_Device) return "RA";
    if (dev == &LA_Device) return "LA";
    return "UNKNOWN";
}

void tm1637_displaySingleLED(TM1637Device &dev, uint8_t grid, uint8_t segment, bool state) {
    if (&dev == &LA_Device && !hasLA) return;
    if (&dev == &RA_Device && !hasRA) return;

    if (grid < 6 && segment < 8) {
        uint8_t before = dev.ledData[grid];
        if (state)
            dev.ledData[grid] |= (1 << segment);
        else
            dev.ledData[grid] &= ~(1 << segment);

        if (dev.ledData[grid] != before) {
            dev.needsUpdate = true; // Only set dirty if actual change
        }
    }
}

/*
void tm1637_displaySingleLED(TM1637Device &dev, uint8_t grid, uint8_t segment, bool state) {

    // Only act if device is present
    if (&dev == &LA_Device && !hasLA) return;
    if (&dev == &RA_Device && !hasRA) return;

    // debugPrintf("[TM1637] ❌ Write / LED skipped. Device: %s FORCED NOT PRESENT FOR DEBUG\n", getTM1637Label(&dev));
    // return;

    if (grid < 6 && segment < 8) {
        if (state)
            dev.ledData[grid] |= (1 << segment);
        else
            dev.ledData[grid] &= ~(1 << segment);

	// Update display BUT only if dev
        tm1637_updateDisplay(dev);
    }
}
*/

void tm1637_clearDisplay(TM1637Device &dev) {
    memset(dev.ledData, 0, sizeof(dev.ledData));
    tm1637_updateDisplay(dev);
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
  tm1637_allOn(RA_Device);
  tm1637_allOn(LA_Device);
}

void tm1637_allOff() {
  debugPrint("⚫ Turning ALL TM1637 LEDs OFF");
  tm1637_allOff(RA_Device);
  tm1637_allOff(LA_Device);
}

void tm1637_sweep(TM1637Device& dev, const char* deviceName) {
  debugPrintf("🔁 Sweep for %s:\n", deviceName);
  for (int grid = 0; grid < 6; grid++) {
    for (int seg = 0; seg < 8; seg++) {
      tm1637_allOff(dev);
      tm1637_displaySingleLED(dev, grid, seg, true);
      debugPrintf("🟢 %s LED at GRID %d, SEG %d\n", deviceName, grid, seg);
      delay(200);
    }
  }
  tm1637_allOff(dev);
  debugPrintf("✅ %s sweep complete.\n", deviceName);
}

void tm1637_sweepPanel() {
  debugPrintln("🔍 Starting TM1637 panel sweep...");
  for (uint16_t i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType != DEVICE_TM1637) continue;

    uint8_t clk     = panelLEDs[i].info.tm1637Info.clkPin;
    uint8_t dio     = panelLEDs[i].info.tm1637Info.dioPin;
    uint8_t grid    = panelLEDs[i].info.tm1637Info.segment;
    uint8_t segment = panelLEDs[i].info.tm1637Info.bit;

    TM1637Device* dev = (dio == RA_DIO_PIN) ? &RA_Device : (dio == LA_DIO_PIN) ? &LA_Device : nullptr;
    if (dev) {
      tm1637_allOff(*dev);
      tm1637_displaySingleLED(*dev, grid, segment, true);
      debugPrintf("🟢 LED ON: %s → GRID %d, SEG %d\n", panelLEDs[i].label, grid, segment);
      delay(500);
    }
  }
  tm1637_allOff();
  debugPrintln("✅ TM1637 labeled sweep complete.");
}

void tm1637_testPattern() {
  debugPrintln("🧪 Running TM1637 Global Test Pattern...");
  tm1637_allOff();
  tm1637_allOn();
  debugPrintln("✅ TM1637 Global Test Complete.");
}

void tm1637_testPattern(TM1637Device& dev, const char* deviceName) {
  debugPrintf("🧪 Running TM1637 Test Pattern for %s...\n", deviceName);
  tm1637_allOff(dev);
  tm1637_allOn(dev);
  debugPrintf("✅ %s Test Complete.\n", deviceName);
}

// TM1637 Helper Implementation
void TM1637_setPanelAllLEDs(TM1637Device &dev, bool state) {
    if (state) tm1637_allOn(dev);
    else tm1637_allOff(dev);
}

// Turns all TM1637 LEDs on/off (RA and LA)
void TM1637_setAllLEDs(bool state) {
  if (state)
    tm1637_allOn();
  else
    tm1637_allOff();
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
}
