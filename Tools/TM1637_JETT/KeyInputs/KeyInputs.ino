// TM1637 Key Logger - ESP32-S2
// Detects unique keys (0..15) from TM1637 2×8 matrix

#include <Arduino.h>

#define JETT_CLK_PIN    9
#define JETT_DIO_PIN    8

// ============================================================================
// STRUCTS FIRST  (this fixes your compilation error)
// ============================================================================
struct TM1637Device {
    uint8_t clkPin;
    uint8_t dioPin;
};

struct ButtonLogger {
    int8_t prevKey      = -1;
    int8_t currentKey   = -1;
    int8_t lastSample   = -1;
    uint8_t stableCount = 0;
};

// ============================================================================
// DEVICE INSTANCE
// ============================================================================
TM1637Device JETSEL_Device = { JETT_CLK_PIN, JETT_DIO_PIN };
ButtonLogger jetLogger;

// ============================================================================
// LOW-LEVEL TM1637
// ============================================================================
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
    bool ack = (digitalRead(dev.dioPin) == LOW);
    digitalWrite(dev.clkPin, LOW);
    pinMode(dev.dioPin, OUTPUT);
    return ack;
}

uint8_t tm1637_readKeys(TM1637Device &dev) {
    uint8_t keys = 0xFF;

    tm1637_start(dev);
    tm1637_writeByte(dev, 0x42);  // read scan code
    pinMode(dev.dioPin, INPUT_PULLUP);

    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(dev.clkPin, LOW);
        delayMicroseconds(3);

        if (digitalRead(dev.dioPin))
            keys |= (1U << i);
        else
            keys &= ~(1U << i);

        digitalWrite(dev.clkPin, HIGH);
        delayMicroseconds(3);
    }

    tm1637_stop(dev);
    return keys;
}

// ============================================================================
// KEY DECODE (0..15)
// ============================================================================
static const uint8_t TM1637_KEY_CODES[16] = {
    0xF7,0xF6,0xF5,0xF4,0xF3,0xF2,0xF1,0xF0, // K1 SG1..SG8
    0xEF,0xEE,0xED,0xEC,0xEB,0xEA,0xE9,0xE8  // K2 SG1..SG8
};

int8_t tm1637_decodeKey(uint8_t raw) {
    if (raw == 0xFF) return -1;

    for (uint8_t i = 0; i < 16; i++)
        if (raw == TM1637_KEY_CODES[i])
            return i;

    return -2; // unknown noise
}

// ============================================================================
// DEBOUNCE + STATE MACHINE
// ============================================================================
#define SAMPLE_WINDOW 4
#define POLL_DELAY_MS 3

bool processButtonLogger(TM1637Device &dev, ButtonLogger &logger) {
    uint8_t raw = tm1637_readKeys(dev);
    int8_t k = tm1637_decodeKey(raw);

    if (k == logger.lastSample)
        logger.stableCount++;
    else {
        logger.lastSample = k;
        logger.stableCount = 1;
    }

    if (logger.stableCount >= SAMPLE_WINDOW) {
        if (k != logger.currentKey) {
            logger.currentKey = k;
            return true;
        }
    }
    return false;
}

void printKey(int8_t key) {
    if (key < 0) {
        Serial.print("NONE");
        return;
    }
    Serial.print("key#");
    Serial.print(key);
    Serial.print(" (K");
    Serial.print((key / 8) + 1);
    Serial.print(", SG");
    Serial.print((key % 8) + 1);
    Serial.print(")");
}

// ============================================================================
// SETUP / LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(JETT_CLK_PIN, OUTPUT);
    pinMode(JETT_DIO_PIN, OUTPUT);
    digitalWrite(JETT_CLK_PIN, HIGH);
    digitalWrite(JETT_DIO_PIN, HIGH);

    Serial.println("TM1637 Key Tester Ready");
}

void loop() {
    if (processButtonLogger(JETSEL_Device, jetLogger)) {
        int8_t now = jetLogger.currentKey;
        int8_t prev = jetLogger.prevKey;

        if (now >= 0) {
            Serial.print("PRESSED ");
            printKey(now);
        } else {
            Serial.print("RELEASED ");
            printKey(prev);
        }
        Serial.println();

        jetLogger.prevKey = now;
    }

    delay(POLL_DELAY_MS);
}
