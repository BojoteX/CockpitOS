/*
 * RS-485 Master for ESP32 S3 - DCS-BIOS Compatible
 *
 * USB Serial: DCS-BIOS data passthrough via socat. NO debug output.
 *
 * Wiring:
 *   ESP32 Pin 17 (TX) -> MAX485 DI
 *   ESP32 Pin 18 (RX) <- MAX485 RO
 *   ESP32 Pin 21      -> MAX485 DE + RE
 */

#include <HardwareSerial.h>

// =============================================================================
// PIN CONFIGURATION
// =============================================================================

#ifndef RS485_TX_PIN
#define RS485_TX_PIN            17
#endif

#ifndef RS485_RX_PIN
#define RS485_RX_PIN            18
#endif

#ifndef RS485_EN_PIN
#define RS485_EN_PIN            21
#endif

// =============================================================================
// PROTOCOL CONSTANTS
// =============================================================================

#define RS485_BAUD_RATE         250000
#define POLL_TIMEOUT_US         1000
#define DATA_TIMEOUT_US         5000
#define TX_BUFFER_SIZE          128
#define RX_BUFFER_SIZE          64
#define EXPORT_QUEUE_SIZE       1024
#define MAX_SLAVES              128
#define MAX_SERIAL_READ_PER_LOOP 64  // Limit bytes read per loop to prevent WDT

// =============================================================================
// HARDWARE SERIAL FOR RS485
// =============================================================================

HardwareSerial RS485(1);

// =============================================================================
// GLOBAL STATE
// =============================================================================

static uint8_t txBuffer[TX_BUFFER_SIZE];
static uint8_t rxBuffer[RX_BUFFER_SIZE];
static uint8_t exportQueue[EXPORT_QUEUE_SIZE];
static size_t exportQueueHead = 0;
static size_t exportQueueTail = 0;

static bool slavePresent[MAX_SLAVES];
static uint8_t pollAddressCounter = 1;
static uint8_t scanAddressCounter = 1;
static uint8_t currentPollAddress = 1;

// =============================================================================
// EXPORT QUEUE
// =============================================================================

static inline size_t exportQueueAvailable() {
    if (exportQueueHead >= exportQueueTail) {
        return exportQueueHead - exportQueueTail;
    }
    return EXPORT_QUEUE_SIZE - exportQueueTail + exportQueueHead;
}

static inline size_t exportQueueFree() {
    return EXPORT_QUEUE_SIZE - exportQueueAvailable() - 1;
}

static inline void exportQueuePut(uint8_t c) {
    size_t nextHead = (exportQueueHead + 1) % EXPORT_QUEUE_SIZE;
    if (nextHead != exportQueueTail) {
        exportQueue[exportQueueHead] = c;
        exportQueueHead = nextHead;
    }
}

static inline uint8_t exportQueueGet() {
    uint8_t c = exportQueue[exportQueueTail];
    exportQueueTail = (exportQueueTail + 1) % EXPORT_QUEUE_SIZE;
    return c;
}

// =============================================================================
// DIRECTION CONTROL
// =============================================================================

static inline void setTxMode() {
    RS485.flush();
    digitalWrite(RS485_EN_PIN, HIGH);
}

static inline void setRxMode() {
    RS485.flush();
    digitalWrite(RS485_EN_PIN, LOW);
}

// =============================================================================
// SEND BUFFER
// =============================================================================

static void sendBuffer(const uint8_t* data, size_t len) {
    if (len == 0) return;
    setTxMode();
    RS485.write(data, len);
    RS485.flush();
    setRxMode();
}

// =============================================================================
// POLL SLAVE
// =============================================================================

static void advancePollAddress() {
    pollAddressCounter = (pollAddressCounter + 1) % MAX_SLAVES;

    uint8_t startAddr = pollAddressCounter;
    while (!slavePresent[pollAddressCounter]) {
        pollAddressCounter = (pollAddressCounter + 1) % MAX_SLAVES;
        if (pollAddressCounter == startAddr) break;  // Prevent infinite loop
    }

    if (pollAddressCounter == 0) {
        scanAddressCounter = (scanAddressCounter + 1) % MAX_SLAVES;
        if (scanAddressCounter == 0) scanAddressCounter = 1;

        uint8_t startScan = scanAddressCounter;
        while (slavePresent[scanAddressCounter]) {
            scanAddressCounter = (scanAddressCounter + 1) % MAX_SLAVES;
            if (scanAddressCounter == 0) scanAddressCounter = 1;
            if (scanAddressCounter == startScan) break;
        }
        currentPollAddress = scanAddressCounter;
    } else {
        currentPollAddress = pollAddressCounter;
    }
}

static void pollSlave(uint8_t address) {
    if (address == 0) return;  // Never poll broadcast address

    // Build poll: [ADDR][MSGTYPE=0][LENGTH=0]
    txBuffer[0] = address;
    txBuffer[1] = 0x00;
    txBuffer[2] = 0x00;

    sendBuffer(txBuffer, 3);

    // Wait for response with yield
    unsigned long startTime = micros();
    while (!RS485.available()) {
        if ((micros() - startTime) > POLL_TIMEOUT_US) {
            slavePresent[address] = false;
            txBuffer[0] = 0x00;
            sendBuffer(txBuffer, 1);
            return;
        }
        yield();  // Feed watchdog
    }

    slavePresent[address] = true;
    uint8_t dataLen = RS485.read();

    if (dataLen == 0) return;
    if (dataLen > RX_BUFFER_SIZE) dataLen = RX_BUFFER_SIZE;

    // Read: [MSGTYPE][DATA...][CHECKSUM]
    size_t bytesRead = 0;
    size_t expectedBytes = dataLen + 2;
    if (expectedBytes > RX_BUFFER_SIZE) expectedBytes = RX_BUFFER_SIZE;

    startTime = micros();
    while (bytesRead < expectedBytes) {
        if (RS485.available()) {
            rxBuffer[bytesRead++] = RS485.read();
        } else if ((micros() - startTime) > DATA_TIMEOUT_US) {
            return;
        }
        yield();  // Feed watchdog
    }

    // Forward slave data to USB Serial (skip msgtype and checksum)
    if (bytesRead >= 2) {
        Serial.write(rxBuffer + 1, bytesRead - 2);
    }
}

// =============================================================================
// BROADCAST EXPORT DATA
// =============================================================================

static void broadcastExportData() {
    size_t available = exportQueueAvailable();
    if (available == 0) return;

    // Limit chunk size
    size_t dataLen = available;
    if (dataLen > TX_BUFFER_SIZE - 4) dataLen = TX_BUFFER_SIZE - 4;
    if (dataLen > 250) dataLen = 250;

    // Build: [ADDR=0][MSGTYPE=0][LENGTH][DATA...][CHECKSUM]
    txBuffer[0] = 0x00;
    txBuffer[1] = 0x00;
    txBuffer[2] = (uint8_t)dataLen;

    uint8_t checksum = 0 ^ 0 ^ (uint8_t)dataLen;

    for (size_t i = 0; i < dataLen; i++) {
        uint8_t c = exportQueueGet();
        txBuffer[3 + i] = c;
        checksum ^= c;
    }
    txBuffer[3 + dataLen] = checksum;

    sendBuffer(txBuffer, 4 + dataLen);
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    // USB Serial for DCS-BIOS passthrough
    Serial.begin(250000);
    Serial.setRxBufferSize(1024);  // Larger RX buffer

    // Direction control
    pinMode(RS485_EN_PIN, OUTPUT);
    digitalWrite(RS485_EN_PIN, LOW);

    // RS485 UART
    RS485.begin(RS485_BAUD_RATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

    // Initialize slave presence
    slavePresent[0] = true;
    for (int i = 1; i < MAX_SLAVES; i++) {
        slavePresent[i] = false;
    }
}

// =============================================================================
// LOOP
// =============================================================================

void loop() {
    // Read DCS-BIOS data from USB - LIMIT per iteration to prevent WDT
    int bytesRead = 0;
    while (Serial.available() && bytesRead < MAX_SERIAL_READ_PER_LOOP) {
        exportQueuePut(Serial.read());
        bytesRead++;
    }

    // Broadcast if we have data
    if (exportQueueAvailable() > 0) {
        broadcastExportData();
        yield();
        return;
    }

    // Poll next slave
    advancePollAddress();
    pollSlave(currentPollAddress);

    yield();  // Always yield to prevent WDT
}
