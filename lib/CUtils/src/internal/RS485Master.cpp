/**
 * @file RS485Master.cpp
 * @brief RS-485 Master Driver for CockpitOS (OpenHornet ABSIS Compatible)
 * 
 * PROTOCOL DERIVED FROM ACTUAL ARDUINO DCS-BIOS LIBRARY SOURCE CODE:
 *   - DcsBiosNgRS485Slave.cpp.inc
 *   - DcsBiosNgRS485Master.cpp.inc
 * 
 * ==========================================================================
 * PROTOCOL SUMMARY:
 * ==========================================================================
 * 
 * PACKET FORMAT:
 *   [Address] [MsgType] [DataLength] [Data...] [Checksum]
 * 
 * TWO-PHASE POLLING:
 *   1. BROADCAST: Address=0 - All slaves receive export data, none respond
 *   2. POLL: Address=N - Only slave N responds with input commands
 * 
 * SLAVE RESPONSE:
 *   - No data:  [0x00] (single byte, NO checksum)
 *   - Has data: [Length] [MsgType=0] [Data...] [0x72]
 * 
 * ==========================================================================
 * BROADCAST MODES (configured in RS485Config.h):
 * ==========================================================================
 * 
 * RS485_FILTER_ADDRESSES=1 (SMART MODE):
 *   - Parses DCS-BIOS stream, extracts address/value pairs
 *   - Queues changes, reconstructs complete frames for broadcast
 *   - Filtered by DcsOutputTable (only addresses slaves care about)
 *   - Minimal bandwidth, maximum reliability
 * 
 * RS485_FILTER_ADDRESSES=0 (RELAY MODE):
 *   - Raw byte relay, exactly like Arduino Mega master
 *   - NO parsing, NO queuing - bytes in, bytes out
 *   - Small buffer, continuous drain, accepts overflow
 *   - Maximum compatibility, works with any sim
 * 
 * RS485_CHANGE_DETECT (only applies when FILTER=1):
 *   - 1 = Only broadcast changed values (delta compression)
 *   - 0 = Broadcast all values every frame
 * 
 * ==========================================================================
 */

#if RS485_MASTER_ENABLED

#include <driver/uart.h>
#include "../RS485Config.h"

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================

static const uint8_t RS485_ADDR_BROADCAST = 0;
static const uint8_t RS485_MSGTYPE_POLL = 0;
static const uint8_t RS485_CHECKSUM_PLACEHOLDER = 0x72;

// Timeout tracking
static uint8_t rs485_skipTimeoutsAfterBroadcast = 0;

// ============================================================================
// STATE MACHINE
// ============================================================================

enum class RS485State : uint8_t {
    IDLE,
    BROADCAST_MSGTYPE, BROADCAST_LENGTH, BROADCAST_DATA, 
    BROADCAST_CHECKSUM, BROADCAST_WAIT_COMPLETE,
    POLL_MSGTYPE, POLL_LENGTH, POLL_WAIT_COMPLETE,
    RX_WAIT_LENGTH, RX_WAIT_MSGTYPE, RX_WAIT_DATA, RX_WAIT_CHECKSUM
};

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

struct SlaveStatus {
    bool     online;
    uint32_t responseCount;
    uint32_t timeoutCount;
    uint32_t lastResponseUs;
};

struct MasterStats {
    uint32_t broadcastCount;
    uint32_t pollCount;
    uint32_t responseCount;
    uint32_t timeoutCount;
    uint32_t inputCmdCount;
    uint32_t checksumErrors;
    uint32_t exportBytesSent;
    uint32_t pollCycles;
    uint32_t changesDetected;
    uint32_t addressesFiltered;
    uint32_t expectedTimeouts;
    uint32_t bufferOverflows;
};

// ============================================================================
// MODE-SPECIFIC DATA STRUCTURES
// ============================================================================

#if RS485_FILTER_ADDRESSES
// ============================================================================
// SMART MODE: Parse stream, queue address/value changes
// ============================================================================

enum RS485ParseState : uint8_t {
    RS485_PARSE_WAIT_FOR_SYNC,
    RS485_PARSE_ADDRESS_LOW, RS485_PARSE_ADDRESS_HIGH,
    RS485_PARSE_COUNT_LOW, RS485_PARSE_COUNT_HIGH,
    RS485_PARSE_DATA_LOW, RS485_PARSE_DATA_HIGH
};

static RS485ParseState rs485_parseState = RS485_PARSE_WAIT_FOR_SYNC;
static uint8_t  rs485_syncByteCount = 0;
static uint16_t rs485_parseAddress = 0;
static uint16_t rs485_parseCount = 0;
static uint16_t rs485_parseData = 0;

// Change tracking (for delta compression)
#if RS485_CHANGE_DETECT
static uint16_t rs485_prevExport[0x4000];  // 32KB for address range 0x0000-0x7FFF
static bool rs485_prevInitialized = false;
#endif

// Change queue
#ifndef RS485_CHANGE_QUEUE_SIZE
#define RS485_CHANGE_QUEUE_SIZE 128
#endif

struct RS485Change { uint16_t address; uint16_t value; };
static RS485Change rs485_changeQueue[RS485_CHANGE_QUEUE_SIZE];
static volatile uint8_t rs485_changeQueueHead = 0;
static volatile uint8_t rs485_changeQueueTail = 0;
static volatile uint8_t rs485_changeCount = 0;
static bool rs485_frameHasChanges = false;

#else
// ============================================================================
// RELAY MODE: Raw byte FIFO, no parsing (like Arduino master)
// ============================================================================

// Small ring buffer - matches Arduino's approach
// Arduino uses 128 bytes, we use 256 for safety margin
#define RS485_RAW_RING_SIZE 256
static uint8_t rs485_rawRing[RS485_RAW_RING_SIZE];
static volatile uint8_t rs485_rawHead = 0;  // Write position
static volatile uint8_t rs485_rawTail = 0;  // Read position
// Note: Count derived from head/tail, no separate counter needed

static inline uint8_t rs485_rawCount() {
    return (uint8_t)(rs485_rawHead - rs485_rawTail);  // Works with wrap-around
}

static inline bool rs485_rawEmpty() {
    return rs485_rawHead == rs485_rawTail;
}

static inline bool rs485_rawFull() {
    return rs485_rawCount() >= (RS485_RAW_RING_SIZE - 1);
}

#endif // RS485_FILTER_ADDRESSES

// ============================================================================
// COMMON STATE VARIABLES
// ============================================================================

static RS485State rs485_state = RS485State::IDLE;
static SlaveStatus rs485_slaves[RS485_MAX_SLAVES];
static MasterStats rs485_stats = {0};

// Auto-discovery
static bool rs485_slavePresent[RS485_MAX_SLAVES + 1];
static uint8_t rs485_pollAddrCounter = 1;
static uint8_t rs485_scanAddrCounter = 1;
static bool rs485_isScanning = false;

// Polling control
static uint8_t rs485_currentPollAddr = 0;
static bool rs485_broadcastPending = false;

// TX state
static uint8_t rs485_txExportData[512];
static size_t rs485_txExportLen = 0;
static size_t rs485_txExportIdx = 0;
static uint8_t rs485_txChecksum = 0;

// RX state
static uint8_t rs485_rxBuffer[RS485_INPUT_BUFFER_SIZE];
static size_t rs485_rxLen = 0;
static size_t rs485_rxExpected = 0;
static uint8_t rs485_rxMsgType = 0;

// Timing
static uint32_t rs485_opStartUs = 0;
static uint32_t rs485_lastPollCompleteUs = 0;
static uint32_t rs485_lastBroadcastMs = 0;

// Slave tracking
static uint8_t rs485_consecutiveTimeouts[RS485_MAX_SLAVES] = {0};
static uint32_t rs485_lastResponseTime[RS485_MAX_SLAVES] = {0};
static bool rs485_offlineReported[RS485_MAX_SLAVES] = {false};
static const uint32_t RS485_OFFLINE_TIME_MS = 1000;

// Expected timeout tracking
static bool rs485_expectTimeoutAfterData = false;

// Flags
static bool rs485_initialized = false;
static bool rs485_enabled = true;

// UART
static const uart_port_t RS485_UART_NUM = UART_NUM_1;

// ============================================================================
// SMART MODE FUNCTIONS (FILTER_ADDRESSES=1)
// ============================================================================

#if RS485_FILTER_ADDRESSES

#if RS485_CHANGE_DETECT
static void rs485_initPrevValues() {
    for (size_t i = 0; i < 0x4000; ++i) {
        rs485_prevExport[i] = 0xFFFFu;
    }
    rs485_prevInitialized = true;
    debugPrint("[RS485] 🔄 Change tracking initialized\n");
}
#endif

static void rs485_queueChange(uint16_t address, uint16_t value) {
    if (rs485_changeCount >= RS485_CHANGE_QUEUE_SIZE) {
        // Queue full - drop oldest to make room (like Arduino ring buffer)
        rs485_changeQueueTail = (rs485_changeQueueTail + 1) % RS485_CHANGE_QUEUE_SIZE;
        rs485_changeCount--;
        rs485_stats.bufferOverflows++;
    }
    
    uint8_t head = rs485_changeQueueHead;
    rs485_changeQueue[head].address = address;
    rs485_changeQueue[head].value = value;
    rs485_changeQueueHead = (head + 1) % RS485_CHANGE_QUEUE_SIZE;
    rs485_changeCount++;
    rs485_frameHasChanges = true;
    rs485_stats.changesDetected++;
}

static void rs485_processAddressValue(uint16_t address, uint16_t value) {
    if (address >= 0x8000) return;
    
    // Filter by DcsOutputTable
    if (findDcsOutputEntries(address) == nullptr) {
        rs485_stats.addressesFiltered++;
        return;
    }
    
    #if RS485_CHANGE_DETECT
    uint16_t index = address >> 1;
    if (rs485_prevExport[index] == value) return;
    rs485_prevExport[index] = value;
    #endif
    
    rs485_queueChange(address, value);
}

static void rs485_parseChar(uint8_t c) {
    switch (rs485_parseState) {
        case RS485_PARSE_WAIT_FOR_SYNC:
            break;
        case RS485_PARSE_ADDRESS_LOW:
            rs485_parseAddress = c;
            rs485_parseState = RS485_PARSE_ADDRESS_HIGH;
            break;
        case RS485_PARSE_ADDRESS_HIGH:
            rs485_parseAddress |= ((uint16_t)c << 8);
            rs485_parseState = (rs485_parseAddress != 0x5555) ? RS485_PARSE_COUNT_LOW : RS485_PARSE_WAIT_FOR_SYNC;
            break;
        case RS485_PARSE_COUNT_LOW:
            rs485_parseCount = c;
            rs485_parseState = RS485_PARSE_COUNT_HIGH;
            break;
        case RS485_PARSE_COUNT_HIGH:
            rs485_parseCount |= ((uint16_t)c << 8);
            rs485_parseState = RS485_PARSE_DATA_LOW;
            break;
        case RS485_PARSE_DATA_LOW:
            rs485_parseData = c;
            rs485_parseCount--;
            rs485_parseState = RS485_PARSE_DATA_HIGH;
            break;
        case RS485_PARSE_DATA_HIGH:
            rs485_parseData |= ((uint16_t)c << 8);
            rs485_parseCount--;
            rs485_processAddressValue(rs485_parseAddress, rs485_parseData);
            rs485_parseAddress += 2;
            rs485_parseState = (rs485_parseCount == 0) ? RS485_PARSE_ADDRESS_LOW : RS485_PARSE_DATA_LOW;
            break;
    }
    
    if (c == 0x55) {
        rs485_syncByteCount++;
    } else {
        rs485_syncByteCount = 0;
    }
    
    if (rs485_syncByteCount >= 4) {
        rs485_parseState = RS485_PARSE_ADDRESS_LOW;
        rs485_syncByteCount = 0;
        
        if (rs485_frameHasChanges) {
            rs485_broadcastPending = true;
            rs485_frameHasChanges = false;
        }
    }
}

// SMART MODE: Build complete DCS-BIOS frames from change queue
static void rs485_prepareExportData() {
    rs485_txExportLen = 0;
    
    while (rs485_changeCount > 0 && rs485_txExportLen < 240) {
        RS485Change& change = rs485_changeQueue[rs485_changeQueueTail];
        
        // Build complete DCS-BIOS frame
        rs485_txExportData[rs485_txExportLen++] = 0x55;
        rs485_txExportData[rs485_txExportLen++] = 0x55;
        rs485_txExportData[rs485_txExportLen++] = 0x55;
        rs485_txExportData[rs485_txExportLen++] = 0x55;
        rs485_txExportData[rs485_txExportLen++] = change.address & 0xFF;
        rs485_txExportData[rs485_txExportLen++] = (change.address >> 8) & 0xFF;
        rs485_txExportData[rs485_txExportLen++] = 0x02;
        rs485_txExportData[rs485_txExportLen++] = 0x00;
        rs485_txExportData[rs485_txExportLen++] = change.value & 0xFF;
        rs485_txExportData[rs485_txExportLen++] = (change.value >> 8) & 0xFF;
        
        rs485_changeQueueTail = (rs485_changeQueueTail + 1) % RS485_CHANGE_QUEUE_SIZE;
        rs485_changeCount--;
    }
    
    rs485_stats.exportBytesSent += rs485_txExportLen;
}

#else
// ============================================================================
// RELAY MODE FUNCTIONS (FILTER_ADDRESSES=0)
// ============================================================================

// Buffer a single raw byte - Arduino-style ring buffer
// When full, discard oldest (overwrite) - this matches Arduino behavior
static void rs485_bufferRawByte(uint8_t byte) {
    // Check if buffer is full
    if (rs485_rawFull()) {
        // Discard oldest byte by advancing tail
        rs485_rawTail++;
        rs485_stats.bufferOverflows++;
    }
    
    // Store byte and advance head
    rs485_rawRing[rs485_rawHead & (RS485_RAW_RING_SIZE - 1)] = byte;
    rs485_rawHead++;
}

// RELAY MODE: Drain raw bytes from FIFO
// Send whatever we have, up to ~128 bytes (Arduino uses 128-byte ring)
static void rs485_prepareExportData() {
    rs485_txExportLen = 0;
    
    uint8_t available = rs485_rawCount();
    if (available == 0) return;
    
    // Limit to ~128 bytes per broadcast (like Arduino)
    uint8_t toSend = (available > 128) ? 128 : available;
    
    for (uint8_t i = 0; i < toSend; i++) {
        rs485_txExportData[rs485_txExportLen++] = rs485_rawRing[rs485_rawTail & (RS485_RAW_RING_SIZE - 1)];
        rs485_rawTail++;
    }
    
    rs485_stats.exportBytesSent += rs485_txExportLen;
}

#endif // RS485_FILTER_ADDRESSES

// ============================================================================
// INPUT COMMAND PROCESSING (common to both modes)
// ============================================================================

static void rs485_processInputCommand(const uint8_t* data, size_t len) {
    if (len == 0) return;
    
    char cmdBuf[64];
    size_t cmdLen = (len >= sizeof(cmdBuf)) ? sizeof(cmdBuf) - 1 : len;
    memcpy(cmdBuf, data, cmdLen);
    cmdBuf[cmdLen] = '\0';
    
    while (cmdLen > 0 && (cmdBuf[cmdLen-1] == '\n' || cmdBuf[cmdLen-1] == '\r')) {
        cmdBuf[--cmdLen] = '\0';
    }
    
    char* space = strchr(cmdBuf, ' ');
    if (!space) {
        debugPrintf("[RS485] ⚠️ Malformed cmd: %s\n", cmdBuf);
        return;
    }
    
    *space = '\0';
    const char* label = cmdBuf;
    const char* value = space + 1;
    
    debugPrintf("[RS485] 🎚️ SWITCH: %s = %s (from slave %d)\n", label, value, rs485_currentPollAddr);
    rs485_stats.inputCmdCount++;
    
    sendCommand(label, value, false);
}

// ============================================================================
// RESPONSE HANDLERS (common to both modes)
// ============================================================================

static void rs485_handleResponse() {
    uint8_t idx = rs485_currentPollAddr - 1;
    if (idx < RS485_MAX_SLAVES) {
        if (rs485_offlineReported[idx]) {
            debugPrintf("[RS485] ✅ Slave %d is ONLINE\n", rs485_currentPollAddr);
            rs485_offlineReported[idx] = false;
        }
        rs485_slaves[idx].online = true;
        rs485_slaves[idx].responseCount++;
        rs485_slaves[idx].lastResponseUs = micros() - rs485_opStartUs;
        rs485_consecutiveTimeouts[idx] = 0;
        rs485_lastResponseTime[idx] = millis();
        
        rs485_slavePresent[rs485_currentPollAddr] = true;
    }

    rs485_stats.responseCount++;

    if (rs485_rxLen > 0 && rs485_rxMsgType == 0) {
        rs485_processInputCommand(rs485_rxBuffer, rs485_rxLen);
        rs485_expectTimeoutAfterData = true;
    }
}

static void rs485_handleTimeout() {
    uint8_t zero = 0x00;
    uart_write_bytes(RS485_UART_NUM, (const char*)&zero, 1);
    uart_wait_tx_done(RS485_UART_NUM, 1);

    if (rs485_expectTimeoutAfterData) {
        rs485_expectTimeoutAfterData = false;
        rs485_stats.expectedTimeouts++;
        rs485_stats.responseCount++;
        return;
    }
    else if (rs485_skipTimeoutsAfterBroadcast > 0 && !rs485_isScanning) {
        rs485_skipTimeoutsAfterBroadcast--;
        rs485_stats.expectedTimeouts++;
        rs485_stats.responseCount++;
        return;
    }
    
    uint8_t idx = rs485_currentPollAddr - 1;
    if (idx < RS485_MAX_SLAVES) {
        rs485_slaves[idx].timeoutCount++;
        rs485_consecutiveTimeouts[idx]++;
        
        if (rs485_isScanning) {
            rs485_slavePresent[rs485_currentPollAddr] = false;
        }
        
        uint32_t timeSinceResponse = millis() - rs485_lastResponseTime[idx];
        if (timeSinceResponse > RS485_OFFLINE_TIME_MS && 
            rs485_lastResponseTime[idx] != 0 &&
            !rs485_offlineReported[idx]) {
            rs485_slaves[idx].online = false;
            rs485_offlineReported[idx] = true;
            debugPrintf("[RS485] ⚠️ Slave %d OFFLINE (no response for %lu ms)\n", 
                       rs485_currentPollAddr, timeSinceResponse);
        }
    }
    
    rs485_stats.timeoutCount++;
}

// ============================================================================
// AUTO-DISCOVERY: ADVANCE POLL ADDRESS
// ============================================================================

static void rs485_advanceToNextSlave() {
    static uint8_t scanCounter = 0;
    rs485_isScanning = false;

    scanCounter++;
    if (scanCounter >= 50) {
        scanCounter = 0;

        for (uint8_t i = 1; i <= RS485_MAX_SLAVES; i++) {
            rs485_scanAddrCounter++;
            if (rs485_scanAddrCounter > RS485_MAX_SLAVES) rs485_scanAddrCounter = 1;

            if (!rs485_slavePresent[rs485_scanAddrCounter]) {
                rs485_currentPollAddr = rs485_scanAddrCounter;
                rs485_isScanning = true;
                rs485_lastPollCompleteUs = micros();
                rs485_state = RS485State::IDLE;
                return;
            }
        }
    }

    for (uint8_t i = 0; i < RS485_MAX_SLAVES; i++) {
        rs485_currentPollAddr++;
        if (rs485_currentPollAddr > RS485_MAX_SLAVES) {
            rs485_currentPollAddr = 1;
            rs485_stats.pollCycles++;

            #if RS485_FILTER_ADDRESSES
            if (rs485_changeCount > 0) rs485_broadcastPending = true;
            #else
            if (!rs485_rawEmpty()) rs485_broadcastPending = true;
            #endif
        }

        if (rs485_slavePresent[rs485_currentPollAddr]) {
            break;
        }
    }

    rs485_lastPollCompleteUs = micros();
    rs485_state = RS485State::IDLE;
}

// ============================================================================
// BROADCAST STATE MACHINE
// ============================================================================

static void rs485_startBroadcast() {
    delayMicroseconds(500);  // Let slaves resync

    rs485_prepareExportData();
    
    if (rs485_txExportLen == 0) {
        rs485_broadcastPending = false;
        rs485_state = RS485State::IDLE;
        return;
    }
    
    rs485_opStartUs = micros();
    rs485_broadcastPending = false;
    
    // Calculate checksum
    rs485_txChecksum = RS485_ADDR_BROADCAST ^ RS485_MSGTYPE_POLL ^ (uint8_t)rs485_txExportLen;
    for (size_t i = 0; i < rs485_txExportLen; i++) {
        rs485_txChecksum ^= rs485_txExportData[i];
    }
    
    uint8_t addr = RS485_ADDR_BROADCAST;
    uart_write_bytes(RS485_UART_NUM, (const char*)&addr, 1);
    rs485_state = RS485State::BROADCAST_MSGTYPE;
    rs485_stats.broadcastCount++;
    rs485_lastBroadcastMs = millis();

    #if RS485_FILTER_ADDRESSES
    rs485_skipTimeoutsAfterBroadcast = 10;
    #else
    rs485_skipTimeoutsAfterBroadcast = 3;  // Less for relay mode (smaller packets)
    #endif
}

static void rs485_processBroadcastTx() {
    switch (rs485_state) {
        case RS485State::BROADCAST_MSGTYPE: {
            uint8_t msgtype = RS485_MSGTYPE_POLL;
            uart_write_bytes(RS485_UART_NUM, (const char*)&msgtype, 1);
            rs485_state = RS485State::BROADCAST_LENGTH;
            break;
        }
        case RS485State::BROADCAST_LENGTH: {
            uint8_t len = (uint8_t)rs485_txExportLen;
            uart_write_bytes(RS485_UART_NUM, (const char*)&len, 1);
            rs485_txExportIdx = 0;
            rs485_state = RS485State::BROADCAST_DATA;
            break;
        }
        case RS485State::BROADCAST_DATA: {
            size_t txFree = 0;
            uart_get_tx_buffer_free_size(RS485_UART_NUM, &txFree);
            while (rs485_txExportIdx < rs485_txExportLen && txFree > 1) {
                uart_write_bytes(RS485_UART_NUM, (const char*)&rs485_txExportData[rs485_txExportIdx], 1);
                rs485_txExportIdx++;
                txFree--;
            }
            if (rs485_txExportIdx >= rs485_txExportLen) {
                rs485_state = RS485State::BROADCAST_CHECKSUM;
            }
            break;
        }
        case RS485State::BROADCAST_CHECKSUM: {
            uart_write_bytes(RS485_UART_NUM, (const char*)&rs485_txChecksum, 1);
            rs485_state = RS485State::BROADCAST_WAIT_COMPLETE;
            break;
        }
        case RS485State::BROADCAST_WAIT_COMPLETE: {
            uart_wait_tx_done(RS485_UART_NUM, 1);
            uart_flush_input(RS485_UART_NUM);

            #if RS485_POST_BROADCAST_DELAY_US > 0
            delayMicroseconds(RS485_POST_BROADCAST_DELAY_US);
            #endif
            rs485_state = RS485State::IDLE;
            break;
        }
        default:
            break;
    }
}

// ============================================================================
// POLL STATE MACHINE
// ============================================================================

static void rs485_startPoll(uint8_t addr) {
    rs485_currentPollAddr = addr;
    rs485_opStartUs = micros();
    
    uart_write_bytes(RS485_UART_NUM, (const char*)&addr, 1);
    rs485_state = RS485State::POLL_MSGTYPE;
    rs485_stats.pollCount++;
}

static void rs485_processPollTx() {
    switch (rs485_state) {
        case RS485State::POLL_MSGTYPE: {
            uint8_t msgtype = RS485_MSGTYPE_POLL;
            uart_write_bytes(RS485_UART_NUM, (const char*)&msgtype, 1);
            rs485_state = RS485State::POLL_LENGTH;
            break;
        }
        case RS485State::POLL_LENGTH: {
            uint8_t len = 0;
            uart_write_bytes(RS485_UART_NUM, (const char*)&len, 1);
            rs485_state = RS485State::POLL_WAIT_COMPLETE;
            break;
        }
        case RS485State::POLL_WAIT_COMPLETE: {
            uart_wait_tx_done(RS485_UART_NUM, 1);

            rs485_rxLen = 0;
            rs485_rxExpected = 0;
            rs485_rxMsgType = 0;
            rs485_opStartUs = micros();
            rs485_state = RS485State::RX_WAIT_LENGTH;
            break;
        }
        default:
            break;
    }
}

// ============================================================================
// RESPONSE STATE MACHINE
// ============================================================================

static void rs485_processRx() {
    size_t available = 0;
    uart_get_buffered_data_len(RS485_UART_NUM, &available);
    if (available == 0) {
        uint32_t elapsed = micros() - rs485_opStartUs;
        if (elapsed > RS485_POLL_TIMEOUT_US) {
            rs485_handleTimeout();
            rs485_advanceToNextSlave();
        }
        return;
    }

    uint8_t byte;
    int n;

    switch (rs485_state) {
    case RS485State::RX_WAIT_LENGTH: {
        n = uart_read_bytes(RS485_UART_NUM, &byte, 1, 0);
        if (n != 1) break;

        if (byte == 0x00) {
            rs485_handleResponse();
            uart_flush_input(RS485_UART_NUM);
            rs485_advanceToNextSlave();
            return;
        }

        rs485_rxExpected = byte;
        rs485_rxLen = 0;
        rs485_state = RS485State::RX_WAIT_MSGTYPE;
        break;
    }

    case RS485State::RX_WAIT_MSGTYPE: {
        n = uart_read_bytes(RS485_UART_NUM, &byte, 1, 0);
        if (n != 1) break;

        rs485_rxMsgType = byte;
        if (rs485_rxExpected > 0) {
            rs485_state = RS485State::RX_WAIT_DATA;
        } else {
            rs485_state = RS485State::RX_WAIT_CHECKSUM;
        }
        break;
    }

    case RS485State::RX_WAIT_DATA: {
        while (available > 0 && rs485_rxLen < rs485_rxExpected) {
            n = uart_read_bytes(RS485_UART_NUM, &byte, 1, 0);
            if (n != 1) break;

            if (rs485_rxLen < RS485_INPUT_BUFFER_SIZE) {
                rs485_rxBuffer[rs485_rxLen] = byte;
            }
            rs485_rxLen++;
            available--;
        }

        if (rs485_rxLen >= rs485_rxExpected) {
            rs485_state = RS485State::RX_WAIT_CHECKSUM;
        }
        break;
    }

    case RS485State::RX_WAIT_CHECKSUM: {
        n = uart_read_bytes(RS485_UART_NUM, &byte, 1, 0);
        if (n != 1) break;

        if (byte == RS485_CHECKSUM_PLACEHOLDER) {
            rs485_handleResponse();
        } else {
            rs485_stats.checksumErrors++;
            debugPrintf("[RS485] ⚠️ Checksum error from slave %d (got 0x%02X)\n", 
                       rs485_currentPollAddr, byte);
        }
        
        uart_flush_input(RS485_UART_NUM);
        rs485_advanceToNextSlave();
        break;
    }

    default:
        break;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool RS485Master_init() {
    if (rs485_initialized) return true;
    
    memset(rs485_slaves, 0, sizeof(rs485_slaves));
    memset(rs485_slavePresent, 0, sizeof(rs485_slavePresent));
    memset(rs485_consecutiveTimeouts, 0, sizeof(rs485_consecutiveTimeouts));
    memset(rs485_lastResponseTime, 0, sizeof(rs485_lastResponseTime));
    memset(rs485_offlineReported, 0, sizeof(rs485_offlineReported));
    
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };
    
    esp_err_t err;
    
    err = uart_driver_install(RS485_UART_NUM, 256, 256, 0, NULL, 0);
    if (err != ESP_OK) {
        debugPrintf("[RS485] ❌ UART driver install failed: %d\n", err);
        return false;
    }
    
    err = uart_param_config(RS485_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        debugPrintf("[RS485] ❌ UART param config failed: %d\n", err);
        return false;
    }
    
    err = uart_set_pin(RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, 
                       (RS485_EN_PIN >= 0) ? RS485_EN_PIN : UART_PIN_NO_CHANGE, 
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        debugPrintf("[RS485] ❌ UART set pin failed: %d\n", err);
        return false;
    }
    
    if (RS485_EN_PIN >= 0) {
        err = uart_set_mode(RS485_UART_NUM, UART_MODE_RS485_HALF_DUPLEX);
        if (err != ESP_OK) {
            debugPrintf("[RS485] ❌ RS-485 mode failed: %d\n", err);
            return false;
        }
    }
    
    #if RS485_FILTER_ADDRESSES
        #if RS485_CHANGE_DETECT
        if (!rs485_prevInitialized) {
            rs485_initPrevValues();
        }
        #endif
        rs485_changeQueueHead = rs485_changeQueueTail = rs485_changeCount = 0;
    #else
        rs485_rawHead = rs485_rawTail = 0;
    #endif
    
    rs485_initialized = true;
    rs485_enabled = true;
    rs485_state = RS485State::IDLE;
    rs485_currentPollAddr = 1;
    rs485_broadcastPending = false;
    rs485_lastPollCompleteUs = micros();
    rs485_lastBroadcastMs = 0;
    rs485_expectTimeoutAfterData = false;
    
    debugPrintf("[RS485] ✅ Init OK: %d baud, TX=%d, RX=%d, EN=%d\n",
                RS485_BAUD, RS485_TX_PIN, RS485_RX_PIN, RS485_EN_PIN);
    
    #if RS485_EN_PIN >= 0
    debugPrintln("[RS485] Direction: Manual (ESP32 controls EN pin)");
    #else
    debugPrintln("[RS485] Direction: Auto (hardware handles TX/RX switching)");
    #endif
    
    debugPrintf("[RS485] Auto-discovery enabled, scanning addresses 1-%d\n", RS485_MAX_SLAVES);
    
    #if RS485_FILTER_ADDRESSES
        #if RS485_CHANGE_DETECT
        debugPrintln("[RS485] Mode: SMART (filtered + change detection)");
        #else
        debugPrintln("[RS485] Mode: SMART (filtered, no delta)");
        #endif
    #else
        debugPrintln("[RS485] Mode: RELAY (raw bytes, Arduino-compatible) 🔄");
    #endif
    
    return true;
}

void RS485Master_loop() {
    if (!rs485_initialized || !rs485_enabled) return;

    if (!simReady()) return;
    
    // Status every 10 seconds
    static uint32_t lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 10000) {
        lastStatusPrint = millis();
        
        uint32_t missed = rs485_stats.pollCount - rs485_stats.responseCount;
        float responseRate = rs485_stats.pollCount > 0 ?
            100.0f * rs485_stats.responseCount / rs485_stats.pollCount : 0;
        
        uint8_t discoveredCount = 0;
        for (int i = 1; i <= RS485_MAX_SLAVES; i++) {
            if (rs485_slavePresent[i]) discoveredCount++;
        }
        
        #if RS485_FILTER_ADDRESSES
        debugPrintf("[RS485] 📊 SMART: Polls=%lu | Resp=%lu (%.1f%%) | Bcasts=%lu | Changes=%lu | Bytes=%lu | Slaves=%d\n",
            rs485_stats.pollCount, rs485_stats.responseCount, responseRate,
            rs485_stats.broadcastCount, rs485_stats.changesDetected, 
            rs485_stats.exportBytesSent, discoveredCount);
        #else
        debugPrintf("[RS485] 📊 RELAY: Polls=%lu | Resp=%lu (%.1f%%) | Bcasts=%lu | Bytes=%lu | Ring=%d | Overflow=%lu | Slaves=%d\n",
            rs485_stats.pollCount, rs485_stats.responseCount, responseRate,
            rs485_stats.broadcastCount, rs485_stats.exportBytesSent,
            rs485_rawCount(), rs485_stats.bufferOverflows, discoveredCount);
        #endif
    }
    
    switch (rs485_state) {
        case RS485State::IDLE: {
            #if RS485_FILTER_ADDRESSES
            // SMART MODE: Broadcast when changes queued + interval elapsed
            bool canBroadcast = rs485_broadcastPending && rs485_changeCount > 0;
            
            #ifdef RS485_DISABLE_BROADCASTS
            canBroadcast = false;
            rs485_changeCount = 0;
            #endif
            
            #if RS485_MIN_BROADCAST_INTERVAL_MS > 0
            if (canBroadcast && (millis() - rs485_lastBroadcastMs < RS485_MIN_BROADCAST_INTERVAL_MS)) {
                canBroadcast = false;
            }
            #endif
            
            #else
            // RELAY MODE: Broadcast IMMEDIATELY if data available (like Arduino)
            // No minimum interval - pump as fast as possible!
            bool canBroadcast = !rs485_rawEmpty();
            
            #ifdef RS485_DISABLE_BROADCASTS
            canBroadcast = false;
            rs485_rawHead = rs485_rawTail = 0;  // Discard data
            #endif
            
            #endif
            
            if (canBroadcast) {
                rs485_startBroadcast();
            } else {
                rs485_broadcastPending = false;
                rs485_startPoll(rs485_currentPollAddr);
            }
            break;
        }
        case RS485State::BROADCAST_MSGTYPE:
        case RS485State::BROADCAST_LENGTH:
        case RS485State::BROADCAST_DATA:
        case RS485State::BROADCAST_CHECKSUM:
        case RS485State::BROADCAST_WAIT_COMPLETE:
            rs485_processBroadcastTx();
            break;
        case RS485State::POLL_MSGTYPE:
        case RS485State::POLL_LENGTH:
        case RS485State::POLL_WAIT_COMPLETE:
            rs485_processPollTx();
            break;
        case RS485State::RX_WAIT_LENGTH:
        case RS485State::RX_WAIT_MSGTYPE:
        case RS485State::RX_WAIT_DATA:
        case RS485State::RX_WAIT_CHECKSUM:
            rs485_processRx();
            break;
        default:
            rs485_state = RS485State::IDLE;
            break;
    }
}

void RS485Master_feedExportData(const uint8_t* data, size_t len) {
    if (!rs485_initialized) return;
    
    #ifdef RS485_DISABLE_BROADCASTS
    (void)data; (void)len;
    return;
    #endif
    
    #if RS485_FILTER_ADDRESSES
    // SMART MODE: Parse and queue changes
    #if RS485_CHANGE_DETECT
    if (!rs485_prevInitialized) rs485_initPrevValues();
    #endif
    
    for (size_t i = 0; i < len; i++) {
        rs485_parseChar(data[i]);
    }
    #else
    // RELAY MODE: Buffer raw bytes (no parsing!)
    for (size_t i = 0; i < len; i++) {
        rs485_bufferRawByte(data[i]);
    }
    #endif
}

void RS485Master_forceFullSync() {
    #if RS485_FILTER_ADDRESSES
        #if RS485_CHANGE_DETECT
        rs485_initPrevValues();
        #endif
        rs485_changeQueueHead = rs485_changeQueueTail = rs485_changeCount = 0;
    #else
        rs485_rawHead = rs485_rawTail = 0;
    #endif
    rs485_broadcastPending = true;
    debugPrint("[RS485] 🔄 Forced full sync\n");
}

void RS485Master_setPollingRange(uint8_t minAddr, uint8_t maxAddr) {
    debugPrintf("[RS485] ⚠️ setPollingRange(%d-%d) ignored - using auto-discovery\n", minAddr, maxAddr);
}

void RS485Master_setEnabled(bool en) {
    rs485_enabled = en;
    debugPrintf("[RS485] %s\n", en ? "Enabled" : "Disabled");
}

bool RS485Master_isSlaveOnline(uint8_t address) {
    if (address < 1 || address > RS485_MAX_SLAVES) return false;
    return rs485_slavePresent[address] && rs485_slaves[address - 1].online;
}

uint8_t RS485Master_getOnlineSlaveCount() {
    uint8_t count = 0;
    for (uint8_t i = 1; i <= RS485_MAX_SLAVES; i++) {
        if (rs485_slavePresent[i] && rs485_slaves[i - 1].online) count++;
    }
    return count;
}

void RS485Master_printStatus() {
    debugPrintln("\n[RS485] ========== STATUS ==========");
    
    #if RS485_FILTER_ADDRESSES
        #if RS485_CHANGE_DETECT
        debugPrintln("[RS485] Mode: SMART (filtered + change detection)");
        #else
        debugPrintln("[RS485] Mode: SMART (filtered, no delta)");
        #endif
    #else
        debugPrintln("[RS485] Mode: RELAY (raw bytes, Arduino-compatible)");
    #endif
    
    debugPrint("[RS485] Discovered slaves: ");
    bool first = true;
    for (int i = 1; i <= RS485_MAX_SLAVES; i++) {
        if (rs485_slavePresent[i]) {
            if (!first) debugPrint(", ");
            debugPrintf("%d", i);
            first = false;
        }
    }
    if (first) debugPrint("(none)");
    debugPrintln("");
    
    float responseRate = rs485_stats.pollCount > 0 ? 
        100.0f * rs485_stats.responseCount / rs485_stats.pollCount : 0;
    debugPrintf("[RS485] Polls: %lu, Responses: %lu (%.1f%%)\n",
                rs485_stats.pollCount, rs485_stats.responseCount, responseRate);
    debugPrintf("[RS485] Timeouts: %lu (+ %lu expected)\n", 
                rs485_stats.timeoutCount, rs485_stats.expectedTimeouts);
    debugPrintf("[RS485] Input cmds: %lu\n", rs485_stats.inputCmdCount);
    debugPrintf("[RS485] Broadcasts: %lu, Bytes sent: %lu\n", 
                rs485_stats.broadcastCount, rs485_stats.exportBytesSent);
    
    #if RS485_FILTER_ADDRESSES
    debugPrintf("[RS485] Changes detected: %lu, Queue: %d\n",
                rs485_stats.changesDetected, rs485_changeCount);
    #else
    debugPrintf("[RS485] Ring buffer: %d/%d, Overflows: %lu\n",
                rs485_rawCount(), RS485_RAW_RING_SIZE, rs485_stats.bufferOverflows);
    #endif
    
    debugPrintln("[RS485] ================================\n");
}

#endif // RS485_MASTER_ENABLED
