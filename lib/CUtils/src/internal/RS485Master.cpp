/**
 * @file RS485Master.cpp
 * @brief RS-485 Master Driver for CockpitOS - FIX H
 * 
 * ==========================================================================
 * FIX H: MATCH STANDALONE EXACTLY + FIX INPUT HANDLING
 * ==========================================================================
 * 
 * Key insight: Standalone has outputs working, inputs NOT working.
 * CockpitOS has inputs working, outputs NOT working.
 * 
 * This suggests the POLL vs BROADCAST interaction is the problem.
 * 
 * This fix:
 * 1. Matches standalone's exact broadcast behavior
 * 2. Fixes the input command parsing bug (was double-processing)
 * 3. Adds proper input deduplication
 * 4. 8KB queue to hold full DCS-BIOS frame
 * 
 * ==========================================================================
 */

#if RS485_MASTER_ENABLED

#include <HardwareSerial.h>
#include "../RS485Config.h"

HardwareSerial RS485(1);

// ============================================================================
// PROTOCOL CONSTANTS - MATCH STANDALONE
// ============================================================================

#define RS485_TX_BUFFER_SIZE    128
#define RS485_MAX_DATA_LEN      (RS485_TX_BUFFER_SIZE - 4)  // 124 bytes
#define RS485_POLL_TIMEOUT_US   1000   // Match standalone
#define RS485_DATA_TIMEOUT_US   5000   // Match standalone

// ============================================================================
// STRUCTURES
// ============================================================================

struct MasterStats {
    uint32_t broadcastCount;
    uint32_t pollCount;
    uint32_t responseCount;
    uint32_t timeoutCount;
    uint32_t inputCmdCount;
    uint32_t exportBytesSent;
    uint32_t queueOverflows;
};

// ============================================================================
// EXPORT QUEUE - 8KB
// ============================================================================

#define RS485_EXPORT_QUEUE_SIZE 8192
static uint8_t rs485_exportQueue[RS485_EXPORT_QUEUE_SIZE];
static volatile size_t rs485_exportQueueHead = 0;
static volatile size_t rs485_exportQueueTail = 0;

static MasterStats rs485_stats = {0};

static inline size_t rs485_exportQueueAvailable() {
    if (rs485_exportQueueHead >= rs485_exportQueueTail) {
        return rs485_exportQueueHead - rs485_exportQueueTail;
    }
    return RS485_EXPORT_QUEUE_SIZE - rs485_exportQueueTail + rs485_exportQueueHead;
}

static inline bool rs485_exportQueuePut(uint8_t c) {
    size_t nextHead = (rs485_exportQueueHead + 1) % RS485_EXPORT_QUEUE_SIZE;
    if (nextHead != rs485_exportQueueTail) {
        rs485_exportQueue[rs485_exportQueueHead] = c;
        rs485_exportQueueHead = nextHead;
        return true;
    }
    rs485_stats.queueOverflows++;
    return false;
}

static inline uint8_t rs485_exportQueueGet() {
    uint8_t c = rs485_exportQueue[rs485_exportQueueTail];
    rs485_exportQueueTail = (rs485_exportQueueTail + 1) % RS485_EXPORT_QUEUE_SIZE;
    return c;
}

// ============================================================================
// INTERNAL STATE
// ============================================================================

static uint8_t rs485_txBuffer[RS485_TX_BUFFER_SIZE];
static uint8_t rs485_rxBuffer[64];

static bool rs485_slavePresent[RS485_MAX_SLAVES];
static uint8_t rs485_pollAddressCounter = 1;
static uint8_t rs485_scanAddressCounter = 1;
static uint8_t rs485_currentPollAddress = 1;

static bool rs485_initialized = false;
static bool rs485_enabled = true;

static uint32_t rs485_maxQueueSeen = 0;

// Input deduplication
static char rs485_lastInputCmd[64] = {0};
static uint32_t rs485_lastInputTimeMs = 0;
static const uint32_t INPUT_DEDUPE_MS = 100;  // Ignore same command within 100ms

// ============================================================================
// DIRECTION CONTROL - MATCH STANDALONE
// ============================================================================

static inline void rs485_setTxMode() {
    RS485.flush();
    digitalWrite(RS485_EN_PIN, HIGH);
}

static inline void rs485_setRxMode() {
    RS485.flush();
    digitalWrite(RS485_EN_PIN, LOW);
}

static void rs485_sendBuffer(const uint8_t* data, size_t len) {
    if (len == 0) return;
    rs485_setTxMode();
    RS485.write(data, len);
    RS485.flush();
    rs485_setRxMode();
}

// ============================================================================
// INPUT COMMAND PROCESSING - WITH DEDUPLICATION
// ============================================================================

static void rs485_processInputCommand(const uint8_t* data, size_t len) {
    if (len == 0 || len >= sizeof(rs485_lastInputCmd)) return;
    
    // Build command string
    char cmdBuf[64];
    memcpy(cmdBuf, data, len);
    cmdBuf[len] = '\0';
    
    // Strip trailing newlines/carriage returns
    while (len > 0 && (cmdBuf[len-1] == '\n' || cmdBuf[len-1] == '\r')) {
        cmdBuf[--len] = '\0';
    }
    
    if (len == 0) return;
    
    // Deduplication: ignore if same command within dedupe window
    uint32_t now = millis();
    if (strcmp(cmdBuf, rs485_lastInputCmd) == 0 && 
        (now - rs485_lastInputTimeMs) < INPUT_DEDUPE_MS) {
        return;  // Duplicate, ignore
    }
    
    // Save for deduplication
    strncpy(rs485_lastInputCmd, cmdBuf, sizeof(rs485_lastInputCmd) - 1);
    rs485_lastInputTimeMs = now;
    
    // Parse "LABEL VALUE" format
    char* space = strchr(cmdBuf, ' ');
    if (!space) {
        debugPrintf("[RS485] ⚠️ Malformed cmd: %s\n", cmdBuf);
        return;
    }
    
    *space = '\0';
    const char* label = cmdBuf;
    const char* value = space + 1;
    
    debugPrintf("[RS485] 🎚️ SWITCH: %s = %s\n", label, value);
    rs485_stats.inputCmdCount++;
    
    sendCommand(label, value, false);
}

// ============================================================================
// ADVANCE POLL ADDRESS - MATCH STANDALONE
// ============================================================================

static void rs485_advancePollAddress() {
    rs485_pollAddressCounter = (rs485_pollAddressCounter + 1) % RS485_MAX_SLAVES;

    uint8_t startAddr = rs485_pollAddressCounter;
    while (!rs485_slavePresent[rs485_pollAddressCounter]) {
        rs485_pollAddressCounter = (rs485_pollAddressCounter + 1) % RS485_MAX_SLAVES;
        if (rs485_pollAddressCounter == startAddr) break;
    }

    if (rs485_pollAddressCounter == 0) {
        rs485_scanAddressCounter = (rs485_scanAddressCounter + 1) % RS485_MAX_SLAVES;
        if (rs485_scanAddressCounter == 0) rs485_scanAddressCounter = 1;

        uint8_t startScan = rs485_scanAddressCounter;
        while (rs485_slavePresent[rs485_scanAddressCounter]) {
            rs485_scanAddressCounter = (rs485_scanAddressCounter + 1) % RS485_MAX_SLAVES;
            if (rs485_scanAddressCounter == 0) rs485_scanAddressCounter = 1;
            if (rs485_scanAddressCounter == startScan) break;
        }
        rs485_currentPollAddress = rs485_scanAddressCounter;
    } else {
        rs485_currentPollAddress = rs485_pollAddressCounter;
    }
}

// ============================================================================
// POLL SLAVE - MATCH STANDALONE EXACTLY
// ============================================================================

static void rs485_pollSlave(uint8_t address) {
    if (address == 0) return;
    
    rs485_stats.pollCount++;
    
    // Build poll: [ADDR][MSGTYPE=0][LENGTH=0]
    rs485_txBuffer[0] = address;
    rs485_txBuffer[1] = 0x00;
    rs485_txBuffer[2] = 0x00;
    
    rs485_sendBuffer(rs485_txBuffer, 3);
    
    // Wait for response with yield
    unsigned long startTime = micros();
    while (!RS485.available()) {
        if ((micros() - startTime) > RS485_POLL_TIMEOUT_US) {
            rs485_slavePresent[address] = false;
            // Standalone sends a single 0x00 on timeout - mimic that
            rs485_txBuffer[0] = 0x00;
            rs485_sendBuffer(rs485_txBuffer, 1);
            rs485_stats.timeoutCount++;
            return;
        }
        yield();
    }
    
    rs485_slavePresent[address] = true;
    rs485_stats.responseCount++;
    
    uint8_t dataLen = RS485.read();
    
    if (dataLen == 0) return;  // Empty response, slave has nothing
    if (dataLen > sizeof(rs485_rxBuffer)) dataLen = sizeof(rs485_rxBuffer);
    
    // Read: [MSGTYPE][DATA...][CHECKSUM]
    size_t bytesRead = 0;
    size_t expectedBytes = dataLen + 2;
    if (expectedBytes > sizeof(rs485_rxBuffer)) expectedBytes = sizeof(rs485_rxBuffer);
    
    startTime = micros();
    while (bytesRead < expectedBytes) {
        if (RS485.available()) {
            rs485_rxBuffer[bytesRead++] = RS485.read();
        } else if ((micros() - startTime) > RS485_DATA_TIMEOUT_US) {
            return;  // Incomplete response, abort
        }
        yield();
    }
    
    // Process input data (skip msgtype byte at [0] and checksum at end)
    if (bytesRead >= 2) {
        // Data starts at rs485_rxBuffer[1], length is bytesRead - 2 (skip msgtype + checksum)
        rs485_processInputCommand(rs485_rxBuffer + 1, bytesRead - 2);
    }
}

// ============================================================================
// BROADCAST EXPORT DATA - MATCH STANDALONE EXACTLY
// ============================================================================

static void rs485_broadcastExportData() {
    size_t available = rs485_exportQueueAvailable();
    if (available == 0) return;
    
    if (available > rs485_maxQueueSeen) {
        rs485_maxQueueSeen = available;
    }
    
    // Limit chunk size - match standalone
    size_t dataLen = available;
    if (dataLen > RS485_MAX_DATA_LEN) dataLen = RS485_MAX_DATA_LEN;
    
    // Build: [ADDR=0][MSGTYPE=0][LENGTH][DATA...][CHECKSUM]
    rs485_txBuffer[0] = 0x00;
    rs485_txBuffer[1] = 0x00;
    rs485_txBuffer[2] = (uint8_t)dataLen;
    
    uint8_t checksum = 0 ^ 0 ^ (uint8_t)dataLen;
    
    for (size_t i = 0; i < dataLen; i++) {
        uint8_t c = rs485_exportQueueGet();
        rs485_txBuffer[3 + i] = c;
        checksum ^= c;
    }
    rs485_txBuffer[3 + dataLen] = checksum;
    
    rs485_sendBuffer(rs485_txBuffer, 4 + dataLen);
    
    rs485_stats.broadcastCount++;
    rs485_stats.exportBytesSent += dataLen;
}

// ============================================================================
// PUBLIC INTERFACE
// ============================================================================

bool RS485Master_init() {
    if (rs485_initialized) return true;
    
    pinMode(RS485_EN_PIN, OUTPUT);
    digitalWrite(RS485_EN_PIN, LOW);
    
    RS485.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
    
    // Initialize slave presence - match standalone
    rs485_slavePresent[0] = true;  // Broadcast address always "present"
    for (int i = 1; i < RS485_MAX_SLAVES; i++) {
        rs485_slavePresent[i] = false;
    }
    
    rs485_exportQueueHead = rs485_exportQueueTail = 0;
    rs485_maxQueueSeen = 0;
    memset(&rs485_stats, 0, sizeof(rs485_stats));
    memset(rs485_lastInputCmd, 0, sizeof(rs485_lastInputCmd));
    rs485_lastInputTimeMs = 0;
    
    rs485_initialized = true;
    rs485_enabled = true;
    
    debugPrintf("[RS485] ✅ Init OK (FIX H): %d baud, TX=%d, RX=%d, EN=%d\n",
                RS485_BAUD, RS485_TX_PIN, RS485_RX_PIN, RS485_EN_PIN);
    debugPrintf("[RS485] Queue: %d bytes, Max broadcast: %d bytes\n", 
                RS485_EXPORT_QUEUE_SIZE, RS485_MAX_DATA_LEN);
    
    return true;
}

void RS485Master_loop() {
    if (!rs485_initialized || !rs485_enabled) return;
    if (!simReady()) return;
    
    // Status every 10 seconds
    static uint32_t lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 10000) {
        lastStatusPrint = millis();
        float responseRate = rs485_stats.pollCount > 0 ?
            100.0f * rs485_stats.responseCount / rs485_stats.pollCount : 0;
        debugPrintf("[RS485] 📊 Polls=%lu | Resp=%lu (%.1f%%) | Bcasts=%lu | Cmds=%lu\n",
            rs485_stats.pollCount, rs485_stats.responseCount, responseRate,
            rs485_stats.broadcastCount, rs485_stats.inputCmdCount);
        debugPrintf("[RS485] 📊 Queue: %d, peak=%d, overflow=%lu\n",
            rs485_exportQueueAvailable(), rs485_maxQueueSeen, rs485_stats.queueOverflows);
    }
    
    // ==========================================================================
    // MATCH STANDALONE LOOP EXACTLY:
    // 1. If we have export data -> broadcast and return (skip polling)
    // 2. Otherwise -> poll next slave
    // ==========================================================================
    
    if (rs485_exportQueueAvailable() > 0) {
        rs485_broadcastExportData();
        yield();
        return;  // Skip polling this iteration
    }
    
    // Poll next slave
    rs485_advancePollAddress();
    rs485_pollSlave(rs485_currentPollAddress);
    
    yield();
}

void RS485Master_feedExportData(const uint8_t* data, size_t len) {
    if (!rs485_initialized) return;
    
    for (size_t i = 0; i < len; i++) {
        rs485_exportQueuePut(data[i]);
    }
}

void RS485Master_forceFullSync() {
    rs485_exportQueueHead = rs485_exportQueueTail = 0;
    rs485_maxQueueSeen = 0;
    debugPrint("[RS485] 🔄 Forced full sync\n");
}

void RS485Master_setPollingRange(uint8_t minAddr, uint8_t maxAddr) {
    // Not used in standalone-matching mode
    debugPrintf("[RS485] Poll range request ignored (using standalone algorithm)\n");
}

void RS485Master_setEnabled(bool en) {
    rs485_enabled = en;
    debugPrintf("[RS485] %s\n", en ? "Enabled" : "Disabled");
}

bool RS485Master_isSlaveOnline(uint8_t address) {
    if (address < 1 || address >= RS485_MAX_SLAVES) return false;
    return rs485_slavePresent[address];
}

uint8_t RS485Master_getOnlineSlaveCount() {
    uint8_t count = 0;
    for (uint8_t i = 1; i < RS485_MAX_SLAVES; i++) {
        if (rs485_slavePresent[i]) count++;
    }
    return count;
}

void RS485Master_printStatus() {
    debugPrintln("\n[RS485] ========== STATUS ==========");
    debugPrintln("[RS485] Mode: FIX H (standalone-matching)");
    float responseRate = rs485_stats.pollCount > 0 ? 
        100.0f * rs485_stats.responseCount / rs485_stats.pollCount : 0;
    debugPrintf("[RS485] Polls: %lu, Responses: %lu (%.1f%%)\n",
                rs485_stats.pollCount, rs485_stats.responseCount, responseRate);
    debugPrintf("[RS485] Timeouts: %lu\n", rs485_stats.timeoutCount);
    debugPrintf("[RS485] Input cmds: %lu\n", rs485_stats.inputCmdCount);
    debugPrintf("[RS485] Broadcasts: %lu, Bytes: %lu\n", 
                rs485_stats.broadcastCount, rs485_stats.exportBytesSent);
    debugPrintf("[RS485] Queue peak: %lu, Overflows: %lu\n",
                rs485_maxQueueSeen, rs485_stats.queueOverflows);
    debugPrintln("[RS485] ================================\n");
}

#endif // RS485_MASTER_ENABLED
