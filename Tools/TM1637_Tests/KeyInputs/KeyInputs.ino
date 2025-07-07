// TM1637 Key Logger - Minimal Standalone
// Pins per your CockpitOS config
#define GLOBAL_CLK_PIN    37 // Shared Clock
#define LA_DIO_PIN        39 // Left Annunciator DIO
#define RA_DIO_PIN        40 // Right Annunciator DIO

#include <Arduino.h>

struct TM1637Device {
    uint8_t clkPin;
    uint8_t dioPin;
};

// --- Your ButtonLogger struct ---
struct ButtonLogger {
    uint8_t prevKeys = 0;
    uint8_t finalKeys = 0;
    uint16_t bitCounters[8] = {0};
    uint8_t sampleCount = 0;
    uint8_t deviceId = 0; // 0=LA, 1=RA
};

// Two devices, LA and RA
TM1637Device LA_Device = {GLOBAL_CLK_PIN, LA_DIO_PIN};
TM1637Device RA_Device = {GLOBAL_CLK_PIN, RA_DIO_PIN};

// --- Low-level TM1637 primitives ---
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
        digitalWrite(dev.dioPin, b & 0x01);
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

// --- Key read primitive ---
uint8_t tm1637_readKeys(TM1637Device &dev) {
    uint8_t keys = 0;
    tm1637_start(dev);
    tm1637_writeByte(dev, 0x42); // Read key scan data
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

// --- Debounce and logging ---
#define SAMPLE_WINDOW 6  // Like your default
#define MAJORITY     50

ButtonLogger laLogger = {0, 0, {0}, 0, 0};
ButtonLogger raLogger = {0, 0, {0}, 0, 1};

// Utility: print key states as 8 bits
void printKeyState(const char* prefix, uint8_t keys) {
    Serial.print(prefix);
    Serial.print(": ");
    for (int i = 7; i >= 0; --i) {
        Serial.print((keys >> i) & 1 ? '1' : '0');
    }
    Serial.println();
}

// Sample and update logger state
bool processButtonLogger(TM1637Device &dev, ButtonLogger& logger) {
    uint8_t raw = tm1637_readKeys(dev);
    for (uint8_t i = 0; i < 8; ++i)
        if (raw & (1 << i)) logger.bitCounters[i]++;
    logger.sampleCount++;
    if (logger.sampleCount >= SAMPLE_WINDOW) {
        uint8_t keys = 0;
        for (uint8_t i = 0; i < 8; ++i)
            if (logger.bitCounters[i] * 100 > SAMPLE_WINDOW * MAJORITY)
                keys |= (1 << i);
        logger.finalKeys = keys;
        memset(logger.bitCounters, 0, sizeof(logger.bitCounters));
        logger.sampleCount = 0;
        return true; // ready to compare/print
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(3000);
    pinMode(GLOBAL_CLK_PIN, OUTPUT);
    pinMode(LA_DIO_PIN, OUTPUT);
    pinMode(RA_DIO_PIN, OUTPUT);
    digitalWrite(GLOBAL_CLK_PIN, HIGH);
    digitalWrite(LA_DIO_PIN, HIGH);
    digitalWrite(RA_DIO_PIN, HIGH);
    Serial.println("TM1637 Key Logger Ready");
}

void loop() {
    // LEFT ANNUNCIATOR
    if (processButtonLogger(LA_Device, laLogger)) {
        if (laLogger.finalKeys != laLogger.prevKeys) {
            printKeyState("[LA] Buttons", laLogger.finalKeys);
            // Print changed bits only
            uint8_t changes = laLogger.finalKeys ^ laLogger.prevKeys;
            for (uint8_t i = 0; i < 8; ++i) {
                if (changes & (1 << i)) {
                    Serial.print("  LA Key ");
                    Serial.print(i);
                    Serial.print(": ");
                    Serial.println((laLogger.finalKeys & (1 << i)) ? "PRESSED" : "RELEASED");
                }
            }
            laLogger.prevKeys = laLogger.finalKeys;
        }
    }

    // RIGHT ANNUNCIATOR
    if (processButtonLogger(RA_Device, raLogger)) {
        if (raLogger.finalKeys != raLogger.prevKeys) {
            printKeyState("[RA] Buttons", raLogger.finalKeys);
            uint8_t changes = raLogger.finalKeys ^ raLogger.prevKeys;
            for (uint8_t i = 0; i < 8; ++i) {
                if (changes & (1 << i)) {
                    Serial.print("  RA Key ");
                    Serial.print(i);
                    Serial.print(": ");
                    Serial.println((raLogger.finalKeys & (1 << i)) ? "PRESSED" : "RELEASED");
                }
            }
            raLogger.prevKeys = raLogger.finalKeys;
        }
    }
    delay(3); // Fast polling
}
