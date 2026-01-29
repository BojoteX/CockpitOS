/**
 * @file RS485Slave.cpp
 * @brief RS-485 Slave Driver for CockpitOS (CUtils internal module)
 * 
 * Implements the DCS-BIOS RS-485 slave protocol, allowing an ESP32 running
 * CockpitOS to operate as a slave on an RS-485 bus alongside Arduino slaves.
 * 
 * Integration: Include this from CUtils.cpp via:
 *   #if RS485_SLAVE_ENABLED
 *   #include "internal/RS485Slave.cpp"
 *   #endif
 * 
 * The slave:
 * - Listens for poll packets addressed to RS485_SLAVE_ADDRESS
 * - Feeds received export data to DcsBios::parser (same as UDP/USB)
 * - Queues input commands and sends them when polled
 * 
 * Protocol Details:
 * - Baud: 250000
 * - Poll Packet: [Type:3|Addr:5][Length][ExportData...][Checksum]
 * - Response:    [Length][MsgType][InputData...][Checksum]
 */

// #include <Arduino.h>

#if RS485_SLAVE_ENABLED

// Mutual exclusion check
#if RS485_MASTER_ENABLED
#error "RS485_MASTER_ENABLED and RS485_SLAVE_ENABLED are mutually exclusive! Choose one."
#endif

#include <driver/uart.h>
#include "../RS485SlaveConfig.h"

// ============================================================================
// INTERNAL STATE
// ============================================================================

enum class RS485SlaveState : uint8_t {
    IDLE,                   // Waiting for poll header
    RX_WAIT_LENGTH,         // Received header, waiting for length
    RX_WAIT_DATA,           // Receiving export data
    RX_WAIT_CHECKSUM,       // Waiting for checksum byte
    TX_SEND_LENGTH,         // Sending response length
    TX_SEND_MSGTYPE,        // Sending message type
    TX_SEND_DATA,           // Sending command data
    TX_SEND_CHECKSUM        // Sending checksum
};

static RS485SlaveState rs485s_state = RS485SlaveState::IDLE;

// RX buffer for export data
static uint8_t rs485s_rxBuffer[512];
static size_t rs485s_rxLen = 0;
static size_t rs485s_rxExpected = 0;
static uint8_t rs485s_rxChecksum = 0;

// TX buffer for input commands (ring buffer)
static uint8_t rs485s_txBuffer[RS485_TX_BUFFER_SIZE];
static volatile size_t rs485s_txHead = 0;
static volatile size_t rs485s_txTail = 0;
static volatile size_t rs485s_txCount = 0;

// TX state during response
static size_t rs485s_txSendIdx = 0;
static size_t rs485s_txSendLen = 0;
static uint8_t rs485s_txChecksum = 0;

// Statistics
static uint32_t rs485s_pollsReceived = 0;
static uint32_t rs485s_pollsIgnored = 0;     // Not for our address
static uint32_t rs485s_exportBytesRx = 0;
static uint32_t rs485s_commandsSent = 0;
static uint32_t rs485s_checksumErrors = 0;

// Timing
static uint32_t rs485s_lastPollTime = 0;

// Flags
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
// INTERNAL FUNCTIONS
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
    
    return true;
}

/**
 * @brief Process received export data
 */
static void rs485s_processExportData() {
    if (rs485s_rxLen == 0) return;
    
    rs485s_exportBytesRx += rs485s_rxLen;
    
    // Feed to CockpitOS DCS-BIOS parser (same path as UDP/USB!)
    parseDcsBiosUdpPacket(rs485s_rxBuffer, rs485s_rxLen);
    
    #if RS485_SLAVE_DEBUG_VERBOSE
    debugPrintf("[RS485S] 📥 Received %d export bytes\n", rs485s_rxLen);
    #endif
}

/**
 * @brief Prepare response data from TX buffer
 */
static void rs485s_prepareResponse() {
    // Snapshot current TX buffer for sending
    noInterrupts();
    rs485s_txSendLen = rs485s_txCount;
    rs485s_txSendIdx = 0;
    
    // Limit to 254 bytes (length byte is uint8_t, minus msgtype)
    if (rs485s_txSendLen > 253) {
        rs485s_txSendLen = 253;
    }
    interrupts();
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
        debugPrintf("[RS485S] 📤 Sent %d bytes in response\n", rs485s_txSendLen);
    }
}

/**
 * @brief Process incoming bytes (RX state machine)
 */
static void rs485s_processRx() {
    size_t available = 0;
    uart_get_buffered_data_len(RS485S_UART_NUM, &available);
    if (available == 0) return;
    
    uint8_t byte;
    
    while (available > 0) {
        uart_read_bytes(RS485S_UART_NUM, &byte, 1, 0);
        available--;
        
        switch (rs485s_state) {
            case RS485SlaveState::IDLE: {
                // Header byte: [Type:3][Addr:5]
                uint8_t msgType = (byte >> 5) & 0x07;
                uint8_t addr = byte & 0x1F;
                
                if (msgType == 0 && addr == RS485_SLAVE_ADDRESS) {
                    // Poll for us!
                    rs485s_rxChecksum = byte;
                    rs485s_state = RS485SlaveState::RX_WAIT_LENGTH;
                    rs485s_lastPollTime = millis();
                    rs485s_pollsReceived++;
                    
                    #if RS485_SLAVE_DEBUG_VERBOSE
                    debugPrintf("[RS485S] 📨 Poll received (addr %d)\n", addr);
                    #endif
                } else if (msgType == 0) {
                    // Poll for different address - need to skip this packet
                    rs485s_pollsIgnored++;
                    // Stay in IDLE, will resync on next poll
                }
                break;
            }
            
            case RS485SlaveState::RX_WAIT_LENGTH: {
                rs485s_rxExpected = byte;
                rs485s_rxChecksum ^= byte;
                rs485s_rxLen = 0;
                
                if (rs485s_rxExpected > 0) {
                    rs485s_state = RS485SlaveState::RX_WAIT_DATA;
                } else {
                    rs485s_state = RS485SlaveState::RX_WAIT_CHECKSUM;
                }
                break;
            }
            
            case RS485SlaveState::RX_WAIT_DATA: {
                if (rs485s_rxLen < sizeof(rs485s_rxBuffer)) {
                    rs485s_rxBuffer[rs485s_rxLen++] = byte;
                }
                rs485s_rxChecksum ^= byte;
                
                if (rs485s_rxLen >= rs485s_rxExpected) {
                    rs485s_state = RS485SlaveState::RX_WAIT_CHECKSUM;
                }
                break;
            }
            
            case RS485SlaveState::RX_WAIT_CHECKSUM: {
                if (byte != rs485s_rxChecksum) {
                    debugPrintf("[RS485S] ⚠️ Checksum error: got 0x%02X, expected 0x%02X\n",
                               byte, rs485s_rxChecksum);
                    rs485s_checksumErrors++;
                }
                
                // Process received export data
                rs485s_processExportData();
                
                // Prepare and send response
                rs485s_prepareResponse();
                rs485s_state = RS485SlaveState::TX_SEND_LENGTH;
                
                // Small delay before responding (let bus settle)
                delayMicroseconds(50);
                break;
            }
            
            default:
                break;
        }
    }
}

/**
 * @brief Process outgoing response (TX state machine)
 */
static void rs485s_processTx() {
    switch (rs485s_state) {
        case RS485SlaveState::TX_SEND_LENGTH: {
            // Length = data + msgtype (if we have data)
            uint8_t len = (rs485s_txSendLen > 0) ? (rs485s_txSendLen + 1) : 0;
            uart_write_bytes(RS485S_UART_NUM, (const char*)&len, 1);
            rs485s_txChecksum = len;
            
            if (len > 0) {
                rs485s_state = RS485SlaveState::TX_SEND_MSGTYPE;
            } else {
                // No data, we're done
                uart_wait_tx_done(RS485S_UART_NUM, 10);
                rs485s_state = RS485SlaveState::IDLE;
            }
            break;
        }
        
        case RS485SlaveState::TX_SEND_MSGTYPE: {
            uint8_t msgType = 0;  // 0 = input command
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
            uart_write_bytes(RS485S_UART_NUM, (const char*)&rs485s_txChecksum, 1);
            uart_wait_tx_done(RS485S_UART_NUM, 10);
            
            rs485s_responseComplete();
            rs485s_state = RS485SlaveState::IDLE;
            break;
        }
        
        default:
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
    
    debugPrintf("[RS485S] Initializing RS-485 Slave (address %d)...\n", RS485_SLAVE_ADDRESS);
    
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    esp_err_t err = uart_driver_install(RS485S_UART_NUM, 512, 256, 0, NULL, 0);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ UART driver install failed: %d\n", err);
        return false;
    }
    
    err = uart_param_config(RS485S_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ UART config failed: %d\n", err);
        return false;
    }
    
    err = uart_set_pin(RS485S_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, RS485_EN_PIN, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ UART set pin failed: %d\n", err);
        return false;
    }
    
    err = uart_set_mode(RS485S_UART_NUM, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ❌ RS-485 mode failed: %d\n", err);
        return false;
    }
    
    // Clear buffers
    rs485s_txHead = 0;
    rs485s_txTail = 0;
    rs485s_txCount = 0;
    
    rs485s_initialized = true;
    rs485s_enabled = true;
    rs485s_state = RS485SlaveState::IDLE;
    
    debugPrintf("[RS485S] ✅ Initialized: addr=%d, %d baud, TX=%d, RX=%d, EN=%d\n",
                RS485_SLAVE_ADDRESS, RS485_BAUD, RS485_TX_PIN, RS485_RX_PIN, RS485_EN_PIN);
    
    return true;
}

void RS485Slave_loop() {
    if (!rs485s_initialized || !rs485s_enabled) return;
    
    switch (rs485s_state) {
        case RS485SlaveState::IDLE:
        case RS485SlaveState::RX_WAIT_LENGTH:
        case RS485SlaveState::RX_WAIT_DATA:
        case RS485SlaveState::RX_WAIT_CHECKSUM:
            rs485s_processRx();
            break;
            
        case RS485SlaveState::TX_SEND_LENGTH:
        case RS485SlaveState::TX_SEND_MSGTYPE:
        case RS485SlaveState::TX_SEND_DATA:
        case RS485SlaveState::TX_SEND_CHECKSUM:
            rs485s_processTx();
            break;
    }
}

/**
 * @brief Queue an input command to be sent when polled
 * 
 * Call this instead of sendCommand() when in slave mode.
 * Format on wire: "LABEL VALUE\n"
 * 
 * @param label Control label (e.g., "MASTER_ARM_SW")
 * @param value Value string (e.g., "1")
 * @return true if queued successfully
 */
bool RS485Slave_queueCommand(const char* label, const char* value) {
    if (!rs485s_initialized) return false;
    
    debugPrintf("[RS485S] 📝 Queuing: %s %s\n", label, value);
    return rs485s_queueCommandInternal(label, value);
}

void RS485Slave_setEnabled(bool en) {
    rs485s_enabled = en;
    debugPrintf("[RS485S] %s\n", rs485s_enabled ? "Enabled" : "Disabled");
}

bool RS485Slave_isEnabled() {
    return rs485s_enabled;
}

uint32_t RS485Slave_getPollCount() {
    return rs485s_pollsReceived;
}

void RS485Slave_printStatus() {
    debugPrintln("\n[RS485S] ========== SLAVE STATUS ==========");
    debugPrintf("[RS485S] Address: %d\n", RS485_SLAVE_ADDRESS);
    debugPrintf("[RS485S] Initialized: %s\n", rs485s_initialized ? "Yes" : "No");
    debugPrintf("[RS485S] Enabled: %s\n", rs485s_enabled ? "Yes" : "No");
    debugPrintf("[RS485S] Polls received: %lu\n", rs485s_pollsReceived);
    debugPrintf("[RS485S] Polls ignored (other addr): %lu\n", rs485s_pollsIgnored);
    debugPrintf("[RS485S] Export bytes RX: %lu\n", rs485s_exportBytesRx);
    debugPrintf("[RS485S] Commands sent: %lu\n", rs485s_commandsSent);
    debugPrintf("[RS485S] Checksum errors: %lu\n", rs485s_checksumErrors);
    debugPrintf("[RS485S] TX buffer pending: %d bytes\n", rs485s_txCount);
    if (rs485s_lastPollTime > 0) {
        debugPrintf("[RS485S] Last poll: %lu ms ago\n", millis() - rs485s_lastPollTime);
    }
    debugPrintln("[RS485S] ========================================\n");
}

#endif // RS485_SLAVE_ENABLED
