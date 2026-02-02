/**
 * @file RS485Slave.cpp
 * @brief RS-485 Slave Driver for CockpitOS (CUtils internal module)
 *
 * Implements the DCS-BIOS RS-485 slave protocol, allowing an ESP32 running
 * CockpitOS to operate as a slave on an RS-485 bus alongside Arduino slaves.
 *
 * FULLY COMPATIBLE WITH:
 *   - Arduino DCS-BIOS RS485 Master (DcsBiosNgRS485Master)
 *   - CockpitOS RS485 Master (both SMART and RELAY modes)
 *   - ESP32 RS485 Master reference implementation
 *
 * Integration: Include this from CUtils.cpp via:
 *   #if RS485_SLAVE_ENABLED
 *   #include "internal/RS485Slave.cpp"
 *   #endif
 *
 * The slave:
 * - Listens for broadcast packets (address 0) with export data
 * - Listens for poll packets addressed to RS485_SLAVE_ADDRESS
 * - Feeds received export data to DcsBios::parser (same as UDP/USB)
 * - Queues input commands and sends them when polled
 *
 * ==========================================================================
 * PROTOCOL DETAILS (DCS-BIOS RS485 Standard)
 * ==========================================================================
 *
 * Baud Rate: 250,000 bps, 8N1
 *
 * MASTER → SLAVE PACKETS:
 *   Broadcast (export data to all):
 *     [Addr=0][MsgType=0][Length>0][ExportData...][Checksum]
 *
 *   Poll (request response from specific slave):
 *     [Addr=N][MsgType=0][Length=0]   ← NO CHECKSUM when Length=0!
 *
 * SLAVE → MASTER RESPONSES:
 *   No data to send:
 *     [0x00]   ← Single byte, NO checksum!
 *
 *   Has data to send:
 *     [Length][MsgType=0][InputData...][Checksum]
 *
 * FIELD DEFINITIONS:
 *   - Addr: 0 = broadcast, 1-126 = specific slave
 *   - MsgType: Always 0 for DCS-BIOS
 *   - Length: Number of DATA bytes (does NOT include MsgType!)
 *   - Checksum: XOR of all preceding bytes, OR fixed 0x72 (Arduino compat)
 *
 * CRITICAL PROTOCOL NOTES:
 *   1. When Length=0, there is NO checksum byte! Packet ends at Length byte.
 *   2. This applies to both master→slave polls AND slave→master responses.
 *   3. Length field counts DATA bytes only, not including MsgType.
 *   4. This matches Arduino's DcsBiosNgRS485Slave behavior exactly.
 *
 * ==========================================================================
 */

#if RS485_SLAVE_ENABLED

// Mutual exclusion check
#if RS485_MASTER_ENABLED
#error "RS485_MASTER_ENABLED and RS485_SLAVE_ENABLED are mutually exclusive! Choose one."
#endif

#include <driver/uart.h>
#include "../RS485SlaveConfig.h"

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================

static const uint8_t RS485_ADDR_BROADCAST = 0;
static const uint8_t RS485_MSGTYPE_EXPORT = 0;
static const uint8_t RS485_CHECKSUM_FIXED = 0x72;  // Arduino compatibility

// ============================================================================
// STATE MACHINE
// ============================================================================

enum class RS485SlaveState : uint8_t {
    // Sync states
    SYNC,                       // Waiting for bus idle gap

    // RX states - receiving from master
    RX_WAIT_ADDRESS,            // Waiting for address byte
    RX_WAIT_MSGTYPE,            // Waiting for message type byte
    RX_WAIT_LENGTH,             // Waiting for data length byte
    RX_WAIT_DATA,               // Receiving data bytes
    RX_WAIT_CHECKSUM,           // Waiting for checksum byte (ONLY when Length > 0!)

    // Skip states - another slave is responding
    RX_SKIP_RESPONSE_LENGTH,    // Skip other slave's response length
    RX_SKIP_RESPONSE_MSGTYPE,   // Skip other slave's response msgtype
    RX_SKIP_RESPONSE_DATA,      // Skip other slave's response data
    RX_SKIP_RESPONSE_CHECKSUM,  // Skip other slave's response checksum

    // TX states - sending our response
    TX_SEND_LENGTH,             // Sending response length
    TX_SEND_MSGTYPE,            // Sending message type
    TX_SEND_DATA,               // Sending command data
    TX_SEND_CHECKSUM            // Sending checksum
};

static RS485SlaveState rs485s_state = RS485SlaveState::SYNC;

// State name helper for debugging
static const char* rs485s_stateName(RS485SlaveState s) {
    switch(s) {
        case RS485SlaveState::SYNC: return "SYNC";
        case RS485SlaveState::RX_WAIT_ADDRESS: return "RX_WAIT_ADDRESS";
        case RS485SlaveState::RX_WAIT_MSGTYPE: return "RX_WAIT_MSGTYPE";
        case RS485SlaveState::RX_WAIT_LENGTH: return "RX_WAIT_LENGTH";
        case RS485SlaveState::RX_WAIT_DATA: return "RX_WAIT_DATA";
        case RS485SlaveState::RX_WAIT_CHECKSUM: return "RX_WAIT_CHECKSUM";
        case RS485SlaveState::RX_SKIP_RESPONSE_LENGTH: return "RX_SKIP_RESP_LEN";
        case RS485SlaveState::RX_SKIP_RESPONSE_MSGTYPE: return "RX_SKIP_RESP_MSG";
        case RS485SlaveState::RX_SKIP_RESPONSE_DATA: return "RX_SKIP_RESP_DATA";
        case RS485SlaveState::RX_SKIP_RESPONSE_CHECKSUM: return "RX_SKIP_RESP_CS";
        case RS485SlaveState::TX_SEND_LENGTH: return "TX_SEND_LENGTH";
        case RS485SlaveState::TX_SEND_MSGTYPE: return "TX_SEND_MSGTYPE";
        case RS485SlaveState::TX_SEND_DATA: return "TX_SEND_DATA";
        case RS485SlaveState::TX_SEND_CHECKSUM: return "TX_SEND_CHECKSUM";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// RX STATE
// ============================================================================

// Current packet info
static uint8_t rs485s_rxAddress = 0;
static uint8_t rs485s_rxMsgType = 0;
static uint8_t rs485s_rxLength = 0;

// RX buffer for export data
static uint8_t rs485s_rxBuffer[512];
static size_t rs485s_rxLen = 0;
static uint8_t rs485s_rxChecksum = 0;

// Skip state counter (number of DATA bytes remaining to skip)
static uint8_t rs485s_skipLength = 0;

// ============================================================================
// TX STATE (ring buffer for input commands)
// ============================================================================

static uint8_t rs485s_txBuffer[RS485_TX_BUFFER_SIZE];
static volatile size_t rs485s_txHead = 0;
static volatile size_t rs485s_txTail = 0;
static volatile size_t rs485s_txCount = 0;

// TX state during response
static size_t rs485s_txSendIdx = 0;
static size_t rs485s_txSendLen = 0;
static uint8_t rs485s_txChecksum = 0;

// ============================================================================
// STATISTICS
// ============================================================================

static uint32_t rs485s_pollsReceived = 0;
static uint32_t rs485s_broadcastsReceived = 0;
static uint32_t rs485s_pollsIgnored = 0;     // For other addresses
static uint32_t rs485s_exportBytesRx = 0;
static uint32_t rs485s_commandsSent = 0;
static uint32_t rs485s_checksumErrors = 0;
static uint32_t rs485s_syncResets = 0;

// DIAGNOSTIC COUNTERS
static uint32_t rs485s_bcastWithData = 0;    // addr=0, len>0 (EXPECTED)
static uint32_t rs485s_pollWithData = 0;     // addr>0, len>0 (unusual but valid)
static uint32_t rs485s_pollEmpty = 0;        // addr>0, len=0 (normal polls)
static uint32_t rs485s_rawBytesRx = 0;       // Total bytes seen on wire

// ============================================================================
// TIMING
// ============================================================================

static uint32_t rs485s_lastByteTime = 0;
static uint32_t rs485s_lastPollTime = 0;
static uint32_t rs485s_rxStartTime = 0;

// ============================================================================
// FLAGS
// ============================================================================

static bool rs485s_initialized = false;
static bool rs485s_enabled = true;

// UART
static const uart_port_t RS485S_UART_NUM = UART_NUM_1;

// ============================================================================
// FORWARD DECLARATIONS (for parser integration)
// ============================================================================

// From DCSBIOSBridge - we feed received export data here
extern void parseDcsBiosUdpPacket(const uint8_t* data, size_t len);

// ============================================================================
// SYNC HELPERS
// ============================================================================

/**
 * @brief Reset to sync state - used on errors or timeouts
 */
static void rs485s_resetToSync() {
    rs485s_state = RS485SlaveState::SYNC;
    rs485s_syncResets++;
    rs485s_lastByteTime = micros();
}

/**
 * @brief Check for RX timeout and reset if needed
 * @return true if timeout occurred
 */
static bool rs485s_checkRxTimeout() {
    if (rs485s_state != RS485SlaveState::SYNC &&
        rs485s_state != RS485SlaveState::RX_WAIT_ADDRESS) {
        uint32_t elapsed = micros() - rs485s_rxStartTime;
        if (elapsed > RS485_SLAVE_RX_TIMEOUT_US) {
            // For SKIP states: other slave probably doesn't exist, just wait for next packet
            if (rs485s_state >= RS485SlaveState::RX_SKIP_RESPONSE_LENGTH &&
                rs485s_state <= RS485SlaveState::RX_SKIP_RESPONSE_CHECKSUM) {
                rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
                return true;
            }
            // For other states: full resync
            rs485s_resetToSync();
            return true;
        }
    }
    return false;
}

// ============================================================================
// TX BUFFER FUNCTIONS
// ============================================================================

/**
 * @brief Queue a command to be sent when polled
 * Format: "LABEL VALUE\n"
 */
static bool rs485s_queueCommandInternal(const char* label, const char* value) {
    // Calculate needed space: label + space + value + newline
    size_t labelLen = strlen(label);
    size_t valueLen = strlen(value);
    size_t needed = labelLen + 1 + valueLen + 1;  // +1 space, +1 newline

    if (needed > RS485_TX_BUFFER_SIZE - rs485s_txCount) {
        debugPrintf("[RS485S] ⚠️ TX buffer full, dropping: %s %s\n", label, value);
        return false;
    }

    // Add to ring buffer
    noInterrupts();
    for (size_t i = 0; i < labelLen; i++) {
        rs485s_txBuffer[rs485s_txHead] = label[i];
        rs485s_txHead = (rs485s_txHead + 1) % RS485_TX_BUFFER_SIZE;
    }
    rs485s_txBuffer[rs485s_txHead] = ' ';
    rs485s_txHead = (rs485s_txHead + 1) % RS485_TX_BUFFER_SIZE;
    for (size_t i = 0; i < valueLen; i++) {
        rs485s_txBuffer[rs485s_txHead] = value[i];
        rs485s_txHead = (rs485s_txHead + 1) % RS485_TX_BUFFER_SIZE;
    }
    rs485s_txBuffer[rs485s_txHead] = '\n';
    rs485s_txHead = (rs485s_txHead + 1) % RS485_TX_BUFFER_SIZE;
    rs485s_txCount += needed;
    interrupts();

    debugPrintf("[RS485S] 📝 Queued command: %s %s (pending=%d)\n", label, value, rs485s_txCount);
    return true;
}

/**
 * @brief Process received export data
 */
static void rs485s_processExportData() {
    if (rs485s_rxLen == 0) return;

    rs485s_exportBytesRx += rs485s_rxLen;

    #if RS485_SLAVE_DEBUG_VERBOSE
    debugPrintf("[RS485S] 📥 Processing %d export bytes, first 4: [%02X %02X %02X %02X]\n",
               rs485s_rxLen,
               rs485s_rxLen > 0 ? rs485s_rxBuffer[0] : 0,
               rs485s_rxLen > 1 ? rs485s_rxBuffer[1] : 0,
               rs485s_rxLen > 2 ? rs485s_rxBuffer[2] : 0,
               rs485s_rxLen > 3 ? rs485s_rxBuffer[3] : 0);
    #endif

    // Feed to CockpitOS DCS-BIOS parser (same path as UDP/USB!)
    parseDcsBiosUdpPacket(rs485s_rxBuffer, rs485s_rxLen);
}

/**
 * @brief Prepare response data from TX buffer
 */
static void rs485s_prepareResponse() {
    // Snapshot current TX buffer for sending
    noInterrupts();
    rs485s_txSendLen = rs485s_txCount;
    rs485s_txSendIdx = 0;

    // Limit to 253 bytes (max that fits in Length byte while leaving room)
    if (rs485s_txSendLen > 253) {
        rs485s_txSendLen = 253;
    }
    interrupts();

    #if RS485_SLAVE_DEBUG_VERBOSE
    debugPrintf("[RS485S] 📤 Preparing response with %d bytes\n", rs485s_txSendLen);
    #endif
}

/**
 * @brief Called after successful response transmission
 */
static void rs485s_responseComplete() {
    // Remove sent bytes from TX buffer
    noInterrupts();
    rs485s_txTail = (rs485s_txTail + rs485s_txSendLen) % RS485_TX_BUFFER_SIZE;
    rs485s_txCount -= rs485s_txSendLen;
    interrupts();

    if (rs485s_txSendLen > 0) {
        rs485s_commandsSent++;
        debugPrintf("[RS485S] ✅ Sent %d bytes in response\n", rs485s_txSendLen);
    }
}

/**
 * @brief Handle a complete packet (called when packet is fully received)
 *
 * This is called either:
 * - After RX_WAIT_CHECKSUM when Length > 0
 * - Directly from RX_WAIT_LENGTH when Length = 0 (no checksum in protocol!)
 *
 * Mirrors Arduino's RX_HOST_MESSAGE_COMPLETE behavior.
 */
static void rs485s_handlePacketComplete() {
    // Determine what to do based on address
    if (rs485s_rxAddress == RS485_ADDR_BROADCAST) {
        // ================================================================
        // BROADCAST: Process export data, no response required
        // ================================================================
        rs485s_broadcastsReceived++;

        if (rs485s_rxMsgType == RS485_MSGTYPE_EXPORT && rs485s_rxLen > 0) {
            rs485s_bcastWithData++;
            
            #if RS485_SLAVE_DEBUG_VERBOSE
            debugPrintf("[RS485S] 📡 BROADCAST len=%d first=[%02X %02X %02X %02X]\n",
                       rs485s_rxLen,
                       rs485s_rxLen > 0 ? rs485s_rxBuffer[0] : 0,
                       rs485s_rxLen > 1 ? rs485s_rxBuffer[1] : 0,
                       rs485s_rxLen > 2 ? rs485s_rxBuffer[2] : 0,
                       rs485s_rxLen > 3 ? rs485s_rxBuffer[3] : 0);
            #endif
            
            // Clamp to buffer size
            if (rs485s_rxLen > sizeof(rs485s_rxBuffer)) {
                rs485s_rxLen = sizeof(rs485s_rxBuffer);
            }
            rs485s_processExportData();
        }

        // Ready for next packet (broadcasts don't get responses)
        rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;

    } else if (rs485s_rxAddress == RS485_SLAVE_ADDRESS) {
        // ================================================================
        // POLL FOR US: Process any data, then we MUST respond
        // ================================================================
        rs485s_pollsReceived++;
        rs485s_lastPollTime = millis();

        // DIAGNOSTIC: Track poll type
        if (rs485s_rxLen > 0) {
            rs485s_pollWithData++;  // Unusual but valid
            #if RS485_SLAVE_DEBUG_VERBOSE
            debugPrintf("[RS485S] 📨 POLL+DATA addr=%d len=%d\n", rs485s_rxAddress, rs485s_rxLen);
            #endif
        } else {
            rs485s_pollEmpty++;  // Normal - polls should be empty
            #if RS485_SLAVE_DEBUG_VERBOSE
            debugPrintf("[RS485S] 📨 POLL addr=%d (empty, will respond with %d bytes)\n", 
                       rs485s_rxAddress, rs485s_txCount);
            #endif
        }

        // Process export data if present (some masters send data with poll)
        if (rs485s_rxMsgType == RS485_MSGTYPE_EXPORT && rs485s_rxLen > 0) {
            if (rs485s_rxLen > sizeof(rs485s_rxBuffer)) {
                rs485s_rxLen = sizeof(rs485s_rxBuffer);
            }
            rs485s_processExportData();
        }

        // Prepare and send response
        rs485s_prepareResponse();
        rs485s_state = RS485SlaveState::TX_SEND_LENGTH;

        // Small delay before responding (let bus settle, ~50µs)
        delayMicroseconds(50);

    } else {
        // ================================================================
        // POLL FOR ANOTHER SLAVE: We must skip their response
        // ================================================================
        rs485s_pollsIgnored++;
        #if RS485_SLAVE_DEBUG_VERBOSE
        debugPrintf("[RS485S] ⏭️ Poll for addr=%d (not us=%d), skipping response\n", 
                   rs485s_rxAddress, RS485_SLAVE_ADDRESS);
        #endif
        rs485s_state = RS485SlaveState::RX_SKIP_RESPONSE_LENGTH;
    }
}

/**
 * @brief Process incoming bytes (RX state machine)
 *
 * Protocol: [Address][MsgType][Length][Data...][Checksum]
 *
 * CRITICAL: When Length=0, there is NO checksum byte!
 * The packet ends immediately after the Length byte.
 *
 * Address = 0: Broadcast (export data to all slaves, no response)
 * Address = N: Poll for slave N (slave N must respond)
 *
 * After a poll for another slave, we must skip their response before
 * we can receive the next packet.
 */
static void rs485s_processRx() {
    uint32_t now = micros();

    // Check for bus idle (sync gap)
    if (rs485s_state == RS485SlaveState::SYNC) {
        size_t available = 0;
        uart_get_buffered_data_len(RS485S_UART_NUM, &available);

        if (available > 0) {
            // Data present, not idle yet - discard and reset timer
            uint8_t discard[64];
            size_t toRead = (available > sizeof(discard)) ? sizeof(discard) : available;
            uart_read_bytes(RS485S_UART_NUM, discard, toRead, 0);
            rs485s_lastByteTime = now;
            rs485s_rawBytesRx += toRead;
        } else if ((now - rs485s_lastByteTime) >= RS485_SLAVE_SYNC_GAP_US) {
            // Bus has been idle long enough, ready to receive
            rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
            debugPrintln("[RS485S] ✓ Sync achieved, waiting for packets");
        }
        return;
    }

    // Check for RX timeout
    if (rs485s_checkRxTimeout()) {
        return;
    }

    // Process available bytes
    while (true) {
        size_t available = 0;
        uart_get_buffered_data_len(RS485S_UART_NUM, &available);
        if (available == 0) break;

        uint8_t byte;
        if (uart_read_bytes(RS485S_UART_NUM, &byte, 1, 0) != 1) break;

        rs485s_lastByteTime = now;
        rs485s_rawBytesRx++;

        switch (rs485s_state) {
            // ==============================================================
            // RX_WAIT_ADDRESS - First byte of packet
            // ==============================================================
            case RS485SlaveState::RX_WAIT_ADDRESS: {
                // Sanity check: valid addresses are 0 (broadcast) or 1-126 (slaves)
                // Address 127+ would be invalid, likely corruption from echo
                if (byte > 126) {
                    #if RS485_SLAVE_DEBUG_VERBOSE
                    debugPrintf("[RS485S] ⚠️ Invalid address 0x%02X, ignoring\n", byte);
                    #endif
                    // Stay in RX_WAIT_ADDRESS, this byte was garbage
                    break;
                }
                
                rs485s_rxAddress = byte;
                rs485s_rxChecksum = byte;
                rs485s_rxStartTime = now;
                rs485s_rxLen = 0;
                rs485s_state = RS485SlaveState::RX_WAIT_MSGTYPE;
                break;
            }

            // ==============================================================
            // RX_WAIT_MSGTYPE - Second byte of packet
            // ==============================================================
            case RS485SlaveState::RX_WAIT_MSGTYPE: {
                // In DCS-BIOS protocol, MsgType is ALWAYS 0
                // Anything else indicates corruption (likely echo)
                if (byte != 0) {
                    #if RS485_SLAVE_DEBUG_VERBOSE
                    debugPrintf("[RS485S] ⚠️ Invalid MsgType 0x%02X (expected 0), resync\n", byte);
                    #endif
                    rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
                    // Don't count this as a sync reset, just discard bad packet
                    break;
                }
                
                rs485s_rxMsgType = byte;
                rs485s_rxChecksum ^= byte;
                rs485s_state = RS485SlaveState::RX_WAIT_LENGTH;
                break;
            }

            // ==============================================================
            // RX_WAIT_LENGTH - Third byte of packet
            // ==============================================================
            case RS485SlaveState::RX_WAIT_LENGTH: {
                rs485s_rxLength = byte;
                rs485s_rxChecksum ^= byte;
                rs485s_rxLen = 0;

                // Sanity check: DCS-BIOS broadcasts typically have <= 250 bytes
                // If we see a very large length, it's likely corruption
                // (e.g., we read 0x55 from a sync sequence as length = 85 is valid,
                //  but 0xAA = 170 or 0xFF = 255 in unusual places could be garbage)
                // We'll accept up to 253 (max valid length byte)
                
                if (rs485s_rxLength > 0) {
                    // Has data, continue to receive data bytes
                    rs485s_state = RS485SlaveState::RX_WAIT_DATA;
                } else {
                    // ======================================================
                    // CRITICAL: Length=0 means NO DATA and NO CHECKSUM!
                    // Packet is complete right now. Do NOT wait for checksum.
                    // This matches Arduino's RX_HOST_MESSAGE_COMPLETE behavior.
                    // ======================================================
                    #if RS485_SLAVE_DEBUG_VERBOSE
                    debugPrintf("[RS485S] Length=0, packet complete (no checksum expected)\n");
                    #endif
                    rs485s_handlePacketComplete();
                }
                break;
            }

            // ==============================================================
            // RX_WAIT_DATA - Receive data bytes
            // ==============================================================
            case RS485SlaveState::RX_WAIT_DATA: {
                // Only buffer if this is a broadcast or poll for us
                if (rs485s_rxAddress == RS485_ADDR_BROADCAST ||
                    rs485s_rxAddress == RS485_SLAVE_ADDRESS) {
                    if (rs485s_rxLen < sizeof(rs485s_rxBuffer)) {
                        rs485s_rxBuffer[rs485s_rxLen] = byte;
                    }
                }
                rs485s_rxLen++;
                rs485s_rxChecksum ^= byte;

                if (rs485s_rxLen >= rs485s_rxLength) {
                    // All data received, now wait for checksum
                    rs485s_state = RS485SlaveState::RX_WAIT_CHECKSUM;
                }
                break;
            }

            // ==============================================================
            // RX_WAIT_CHECKSUM - Verify and process packet (only when Length > 0)
            // ==============================================================
            case RS485SlaveState::RX_WAIT_CHECKSUM: {
                // Validate checksum
                if (byte != rs485s_rxChecksum) {
                    #if RS485_SLAVE_DEBUG_VERBOSE
                    debugPrintf("[RS485S] ⚠️ Checksum error: got 0x%02X, expected 0x%02X\n",
                               byte, rs485s_rxChecksum);
                    #endif
                    rs485s_checksumErrors++;
                    // Still process the packet, but log the error
                }

                // Handle the complete packet
                rs485s_handlePacketComplete();
                break;
            }

            // ==============================================================
            // SKIP STATES - Skip another slave's response
            // ==============================================================
            
            // Response format: [Length][MsgType][Data × Length][Checksum]
            // where Length = DATA byte count (does NOT include MsgType!)
            
            case RS485SlaveState::RX_SKIP_RESPONSE_LENGTH: {
                rs485s_skipLength = byte;  // This is DATA byte count
                if (rs485s_skipLength == 0) {
                    // Other slave had no data: just [0x00], no checksum
                    // Ready for next packet immediately
                    rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
                } else {
                    // Has data: need to skip MsgType + Data + Checksum
                    rs485s_state = RS485SlaveState::RX_SKIP_RESPONSE_MSGTYPE;
                }
                break;
            }

            case RS485SlaveState::RX_SKIP_RESPONSE_MSGTYPE: {
                // Skip MsgType byte
                // IMPORTANT: Do NOT decrement skipLength here!
                // skipLength is the DATA count only, MsgType is separate.
                if (rs485s_skipLength == 0) {
                    // Edge case: Length>0 but only MsgType, no data (unusual)
                    rs485s_state = RS485SlaveState::RX_SKIP_RESPONSE_CHECKSUM;
                } else {
                    rs485s_state = RS485SlaveState::RX_SKIP_RESPONSE_DATA;
                }
                break;
            }

            case RS485SlaveState::RX_SKIP_RESPONSE_DATA: {
                rs485s_skipLength--;
                if (rs485s_skipLength == 0) {
                    rs485s_state = RS485SlaveState::RX_SKIP_RESPONSE_CHECKSUM;
                }
                break;
            }

            case RS485SlaveState::RX_SKIP_RESPONSE_CHECKSUM: {
                // Skip checksum byte, ready for next packet
                rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
                break;
            }

            default:
                // Unknown state, resync
                debugPrintf("[RS485S] ⚠️ Unknown state %d, resync\n", (int)rs485s_state);
                rs485s_resetToSync();
                break;
        }

        // If we've transitioned to a TX state, exit the RX loop
        // Let RS485Slave_loop() route to rs485s_processTx()
        if (rs485s_state >= RS485SlaveState::TX_SEND_LENGTH) {
            break;
        }

        // Update timestamp for next iteration
        now = micros();
    }
}

/**
 * @brief Process outgoing response (TX state machine)
 *
 * Response format:
 * - No data:   [0x00] (single byte, NO checksum!)
 * - With data: [Length][MsgType=0][Data...][Checksum]
 *
 * Where Length = number of DATA bytes (does NOT include MsgType!)
 *
 * For Arduino compatibility, we use 0x72 as checksum placeholder
 * when RS485_SLAVE_ARDUINO_COMPAT is enabled.
 */
static void rs485s_processTx() {
    switch (rs485s_state) {
        case RS485SlaveState::TX_SEND_LENGTH: {
            // Length byte = data byte count (matches Arduino behavior)
            uint8_t len = (uint8_t)rs485s_txSendLen;

            uart_write_bytes(RS485S_UART_NUM, (const char*)&len, 1);
            rs485s_txChecksum = len;

            if (len > 0) {
                rs485s_state = RS485SlaveState::TX_SEND_MSGTYPE;
            } else {
                // No data - single 0x00 byte response, NO CHECKSUM!
                uart_wait_tx_done(RS485S_UART_NUM, 10);
                
                // Echo suppression (even single byte can echo)
                delayMicroseconds(500);
                uart_flush_input(RS485S_UART_NUM);
                
                rs485s_responseComplete();
                rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
                #if RS485_SLAVE_DEBUG_VERBOSE
                debugPrintln("[RS485S] 📤 Sent empty response [0x00]");
                #endif
            }
            break;
        }

        case RS485SlaveState::TX_SEND_MSGTYPE: {
            uint8_t msgType = 0;  // 0 = input command data
            uart_write_bytes(RS485S_UART_NUM, (const char*)&msgType, 1);
            rs485s_txChecksum ^= msgType;
            rs485s_txSendIdx = 0;
            rs485s_state = RS485SlaveState::TX_SEND_DATA;
            break;
        }

        case RS485SlaveState::TX_SEND_DATA: {
            // Send data from ring buffer
            while (rs485s_txSendIdx < rs485s_txSendLen) {
                size_t bufIdx = (rs485s_txTail + rs485s_txSendIdx) % RS485_TX_BUFFER_SIZE;
                uint8_t byte = rs485s_txBuffer[bufIdx];
                uart_write_bytes(RS485S_UART_NUM, (const char*)&byte, 1);
                rs485s_txChecksum ^= byte;
                rs485s_txSendIdx++;
            }
            rs485s_state = RS485SlaveState::TX_SEND_CHECKSUM;
            break;
        }

        case RS485SlaveState::TX_SEND_CHECKSUM: {
            // Send checksum
            #if RS485_SLAVE_ARDUINO_COMPAT
            // Arduino DCS-BIOS uses fixed 0x72 placeholder
            uint8_t checksum = RS485_CHECKSUM_FIXED;
            #else
            // Use calculated XOR checksum
            uint8_t checksum = rs485s_txChecksum;
            #endif

            uart_write_bytes(RS485S_UART_NUM, (const char*)&checksum, 1);
            uart_wait_tx_done(RS485S_UART_NUM, 10);

            // ═══════════════════════════════════════════════════════════════
            // CRITICAL: Echo suppression fix
            // 
            // Despite UART_MODE_RS485_HALF_DUPLEX, there's a timing race where
            // our transmitted bytes can echo back and be read as incoming data.
            // This corrupts the state machine (we read our own response as a
            // "broadcast" with address 0x00).
            //
            // Solution: Wait for potential echo to arrive, then flush it.
            // At 250kbaud with 30 bytes, echo takes ~1.2ms to complete.
            // ═══════════════════════════════════════════════════════════════
            delayMicroseconds(1500);  // Wait for echo to arrive
            uart_flush_input(RS485S_UART_NUM);  // Clear any echo
            delayMicroseconds(200);  // Additional settling time
            uart_flush_input(RS485S_UART_NUM);  // Double-flush for safety

            rs485s_responseComplete();

            // Ready for next packet
            rs485s_state = RS485SlaveState::RX_WAIT_ADDRESS;
            break;
        }

        default:
            // Shouldn't be in TX states without proper setup
            debugPrintf("[RS485S] ⚠️ Unexpected TX state %d, resync\n", (int)rs485s_state);
            rs485s_resetToSync();
            break;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool RS485Slave_init() {
    if (rs485s_initialized) {
        debugPrintln("[RS485S] Already initialized");
        return true;
    }

    debugPrintln("[RS485S] Initializing RS-485 Slave...");

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

    err = uart_driver_install(RS485S_UART_NUM, 512, 512, 0, NULL, 0);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ UART driver install failed: %d\n", err);
        return false;
    }

    err = uart_param_config(RS485S_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ UART param config failed: %d\n", err);
        return false;
    }

    // Handle EN pin configuration
    int en_pin = RS485_EN_PIN;
    
    err = uart_set_pin(RS485S_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, 
                       (en_pin >= 0) ? en_pin : UART_PIN_NO_CHANGE, 
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ UART set pin failed: %d\n", err);
        return false;
    }

    // ALWAYS use RS-485 half-duplex mode - required for proper RS-485 communication
    // The EN pin setting only affects whether ESP32 drives direction control or
    // whether external auto-direction hardware handles it
    err = uart_set_mode(RS485S_UART_NUM, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ RS-485 half-duplex mode failed: %d\n", err);
        return false;
    }

    // Clear buffers
    rs485s_txHead = 0;
    rs485s_txTail = 0;
    rs485s_txCount = 0;
    rs485s_rxLen = 0;

    // Reset statistics
    rs485s_pollsReceived = 0;
    rs485s_broadcastsReceived = 0;
    rs485s_pollsIgnored = 0;
    rs485s_exportBytesRx = 0;
    rs485s_commandsSent = 0;
    rs485s_checksumErrors = 0;
    rs485s_syncResets = 0;
    rs485s_bcastWithData = 0;
    rs485s_pollWithData = 0;
    rs485s_pollEmpty = 0;
    rs485s_rawBytesRx = 0;

    // Initialize timing
    rs485s_lastByteTime = micros();
    rs485s_lastPollTime = 0;
    rs485s_rxStartTime = 0;

    rs485s_initialized = true;
    rs485s_enabled = true;
    rs485s_state = RS485SlaveState::SYNC;  // Start in SYNC to detect first packet

    debugPrintln("[RS485S] ═══════════════════════════════════════════════");
    debugPrintf("[RS485S] ✅ SLAVE INITIALIZED\n");
    debugPrintf("[RS485S]    Address: %d\n", RS485_SLAVE_ADDRESS);
    debugPrintf("[RS485S]    Baud: %d\n", RS485_BAUD);
    debugPrintf("[RS485S]    TX Pin: GPIO%d\n", RS485_TX_PIN);
    debugPrintf("[RS485S]    RX Pin: GPIO%d\n", RS485_RX_PIN);
    debugPrintf("[RS485S]    EN Pin: %s\n", (en_pin >= 0) ? String(en_pin).c_str() : "Auto-direction");
    #if RS485_SLAVE_ARDUINO_COMPAT
    debugPrintln("[RS485S]    Mode: Arduino-compatible (fixed 0x72 checksum)");
    #else
    debugPrintln("[RS485S]    Mode: Full protocol (calculated XOR checksum)");
    #endif
    debugPrintln("[RS485S] ═══════════════════════════════════════════════");

    return true;
}

void RS485Slave_loop() {
    if (!rs485s_initialized || !rs485s_enabled) return;

    switch (rs485s_state) {
        // RX states
        case RS485SlaveState::SYNC:
        case RS485SlaveState::RX_WAIT_ADDRESS:
        case RS485SlaveState::RX_WAIT_MSGTYPE:
        case RS485SlaveState::RX_WAIT_LENGTH:
        case RS485SlaveState::RX_WAIT_DATA:
        case RS485SlaveState::RX_WAIT_CHECKSUM:
        case RS485SlaveState::RX_SKIP_RESPONSE_LENGTH:
        case RS485SlaveState::RX_SKIP_RESPONSE_MSGTYPE:
        case RS485SlaveState::RX_SKIP_RESPONSE_DATA:
        case RS485SlaveState::RX_SKIP_RESPONSE_CHECKSUM:
            rs485s_processRx();
            break;

        // TX states
        case RS485SlaveState::TX_SEND_LENGTH:
        case RS485SlaveState::TX_SEND_MSGTYPE:
        case RS485SlaveState::TX_SEND_DATA:
        case RS485SlaveState::TX_SEND_CHECKSUM:
            rs485s_processTx();
            break;

        default:
            // Unknown state, reset
            rs485s_resetToSync();
            break;
    }

    // Periodic status (every 5 seconds)
    static uint32_t lastStatusTime = 0;
    if (millis() - lastStatusTime > 5000) {
        lastStatusTime = millis();
        debugPrintln("[RS485S] ─────────────────────────────────────────────");
        debugPrintf("[RS485S] State: %s\n", rs485s_stateName(rs485s_state));
        debugPrintf("[RS485S] Raw bytes RX: %lu\n", rs485s_rawBytesRx);
        debugPrintf("[RS485S] Polls: %lu (empty=%lu, data=%lu)\n", 
                   rs485s_pollsReceived, rs485s_pollEmpty, rs485s_pollWithData);
        debugPrintf("[RS485S] Broadcasts: %lu (with data=%lu)\n", 
                   rs485s_broadcastsReceived, rs485s_bcastWithData);
        debugPrintf("[RS485S] Export bytes: %lu\n", rs485s_exportBytesRx);
        debugPrintf("[RS485S] Commands sent: %lu\n", rs485s_commandsSent);
        debugPrintf("[RS485S] TX pending: %d bytes\n", rs485s_txCount);
        debugPrintf("[RS485S] Errors: checksum=%lu, sync=%lu\n", 
                   rs485s_checksumErrors, rs485s_syncResets);
        debugPrintf("[RS485S] Last poll: %lu ms ago\n", 
                   rs485s_lastPollTime ? (millis() - rs485s_lastPollTime) : 0);
        debugPrintln("[RS485S] ─────────────────────────────────────────────");
    }
}

/**
 * @brief Queue an input command to be sent when polled
 *
 * Call this instead of sendCommand() when in slave mode.
 * Format on wire: "LABEL VALUE\n"
 *
 * @param label Control label (e.g., "UFC_MASTER_CAUTION")
 * @param value Value string (e.g., "1")
 * @return true if queued successfully
 */
bool RS485Slave_queueCommand(const char* label, const char* value) {
    if (!rs485s_initialized) {
        debugPrintf("[RS485S] ⚠️ Not initialized, cannot queue: %s %s\n", label, value);
        return false;
    }

    return rs485s_queueCommandInternal(label, value);
}

void RS485Slave_setEnabled(bool en) {
    rs485s_enabled = en;
    if (!en) {
        // Reset to sync when disabled
        rs485s_state = RS485SlaveState::SYNC;
    }
    debugPrintf("[RS485S] %s\n", rs485s_enabled ? "Enabled" : "Disabled");
}

bool RS485Slave_isEnabled() {
    return rs485s_enabled;
}

bool RS485Slave_isInitialized() {
    return rs485s_initialized;
}

uint32_t RS485Slave_getPollCount() {
    return rs485s_pollsReceived;
}

uint32_t RS485Slave_getBroadcastCount() {
    return rs485s_broadcastsReceived;
}

uint32_t RS485Slave_getExportBytesReceived() {
    return rs485s_exportBytesRx;
}

uint32_t RS485Slave_getCommandsSent() {
    return rs485s_commandsSent;
}

uint32_t RS485Slave_getChecksumErrors() {
    return rs485s_checksumErrors;
}

size_t RS485Slave_getTxBufferPending() {
    return rs485s_txCount;
}

uint32_t RS485Slave_getTimeSinceLastPoll() {
    if (rs485s_lastPollTime == 0) return 0xFFFFFFFF;
    return millis() - rs485s_lastPollTime;
}

void RS485Slave_printStatus() {
    debugPrintln("\n[RS485S] ══════════════ SLAVE STATUS ══════════════");
    debugPrintf("[RS485S] Address: %d\n", RS485_SLAVE_ADDRESS);
    debugPrintf("[RS485S] State: %s (%d)\n", rs485s_stateName(rs485s_state), (int)rs485s_state);
    debugPrintf("[RS485S] Raw bytes RX: %lu\n", rs485s_rawBytesRx);
    debugPrintf("[RS485S] Polls received: %lu (empty=%lu, data=%lu)\n",
                rs485s_pollsReceived, rs485s_pollEmpty, rs485s_pollWithData);
    debugPrintf("[RS485S] Polls ignored (other addr): %lu\n", rs485s_pollsIgnored);
    debugPrintf("[RS485S] Broadcasts received: %lu (with data=%lu)\n",
                rs485s_broadcastsReceived, rs485s_bcastWithData);
    debugPrintf("[RS485S] Export bytes RX: %lu\n", rs485s_exportBytesRx);
    debugPrintf("[RS485S] Commands sent: %lu\n", rs485s_commandsSent);
    debugPrintf("[RS485S] TX buffer pending: %d bytes\n", rs485s_txCount);
    debugPrintf("[RS485S] Checksum errors: %lu\n", rs485s_checksumErrors);
    debugPrintf("[RS485S] Sync resets: %lu\n", rs485s_syncResets);
    debugPrintf("[RS485S] Time since last poll: %lu ms\n", RS485Slave_getTimeSinceLastPoll());
    debugPrintln("[RS485S] ═════════════════════════════════════════════\n");
}

#endif // RS485_SLAVE_ENABLED
