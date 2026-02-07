/**
 * @file RS485Slave.cpp
 * @brief RS-485 Slave Driver for CockpitOS - Complete Rewrite v2.3
 *
 * ARCHITECTURE:
 *   - ESP-IDF UART driver with DMA-backed RX buffering
 *   - Polling-based RX (uart_read_bytes) for robust data capture during TX
 *   - Echo prevention via RX buffer flush after TX
 *   - IMMEDIATE response: sendResponse() called directly from state machine
 *   - Deferred export processing: byte-by-byte parser runs in main loop
 *
 * V2.3 CRITICAL FIX - RING BUFFER FOR EXPORT DATA:
 *   Previous versions used a LINEAR buffer for export data that was reset
 *   at the start of each new packet. If the main loop was slow, new packets
 *   would OVERWRITE previous export data before it was processed = DATA LOSS!
 *
 *   The standalone slave.ino uses a RING BUFFER with separate read/write
 *   positions. This allows continuous streaming - new data appends while
 *   the parser catches up. NO DATA LOSS even with slow main loops.
 *
 *   Changes in v2.3:
 *   1. Export buffer is now a ring buffer (read/write positions, not length)
 *   2. Byte-by-byte processing like standalone (not batch parseDcsBiosUdpPacket)
 *   3. Overflow handling: force re-sync (match AVR behavior, don't silently drop)
 *   4. Buffer variables marked volatile for ISR safety
 *
 * V2.2 CRITICAL FIX:
 *   Previous versions called processExportData() (which runs parseDcsBiosUdpPacket
 *   with all DCS-BIOS callbacks) BEFORE sending the response. This added hundreds
 *   of microseconds of latency, causing desync with the master's 2ms timeout.
 *
 *   Now matches standalone slave.ino behavior:
 *   1. Poll detected → sendResponse() called IMMEDIATELY
 *   2. Export data processing deferred to main loop (after response sent)
 *
 * RATIONALE:
 *   Testing showed that for SLAVE operation, Driver mode is more stable than
 *   ISR mode. The slave can receive broadcast data DURING its TX response,
 *   and the DMA-backed RX buffer handles this overlap correctly. ISR mode
 *   has timing issues where TX blocks the ISR and bytes are lost.
 *
 *   Master uses ISR mode because it controls the bus timing and RX/TX never
 *   overlap - it needs low latency to catch slave responses quickly.
 *
 * PROTOCOL: 100% compatible with Arduino DCS-BIOS RS485 implementation
 *
 * FULLY COMPATIBLE WITH:
 *   - Arduino DCS-BIOS RS485 Master (DcsBiosNgRS485Master)
 *   - CockpitOS RS485 Master (SMART and RELAY modes)
 *   - ESP32 DCS-BIOS Library RS485 Master
 *
 * ==========================================================================
 */

#if RS485_SLAVE_ENABLED

// Mutual exclusion check
#if RS485_MASTER_ENABLED
#error "RS485_MASTER_ENABLED and RS485_SLAVE_ENABLED are mutually exclusive!"
#endif

#include "../RS485SlaveConfig.h"

// ESP-IDF includes
#include "esp_attr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"        // For ets_delay_us()
#include "esp_timer.h"

// FreeRTOS includes (for task mode)
#if RS485_USE_TASK
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

// TX Warm-up delays in MICROSECONDS (portable across all ESP32 variants)
// These give the transceiver time to switch to TX mode before data is sent
#define TX_WARMUP_DELAY_MANUAL_US    50    // Manual DE: wait after DE asserted
#define TX_WARMUP_DELAY_AUTO_US      50    // Auto-direction: wait for RX→TX switch

// Sync detection timeout
#define SYNC_TIMEOUT_US              500   // 500µs silence = sync detected

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================

static constexpr uint8_t ADDR_BROADCAST = 0;
static constexpr uint8_t MSGTYPE_DCSBIOS = 0;
static constexpr uint8_t CHECKSUM_FIXED = 0x72;

// ============================================================================
// STATE MACHINE
// ============================================================================

enum class SlaveState : uint8_t {
    // Sync state
    RX_SYNC,
    // RX states
    RX_WAIT_ADDRESS,
    RX_WAIT_MSGTYPE,
    RX_WAIT_LENGTH,
    RX_WAIT_DATA,
    RX_WAIT_CHECKSUM,
    // Skip other slave's response
    RX_SKIP_LENGTH,
    RX_SKIP_DATA,
    RX_SKIP_CHECKSUM,
    // TX states
    TX_RESPOND
};


// ============================================================================
// TX RING BUFFER (for queued input commands)
// ============================================================================

static uint8_t txBuffer[RS485_TX_BUFFER_SIZE];
static volatile uint16_t txHead = 0;
static volatile uint16_t txTail = 0;
static volatile uint16_t txCount_val = 0;

static inline uint16_t txCount() { return txCount_val; }
static inline bool txEmpty() { return txCount_val == 0; }

// ============================================================================
// EXPORT DATA RING BUFFER (for broadcast packets)
// ============================================================================
// V2.3: Changed from linear buffer to ring buffer to prevent data loss
// when new packets arrive before previous data is processed.

static volatile uint8_t exportBuffer[RS485_EXPORT_BUFFER_SIZE];
static volatile uint16_t exportWritePos = 0;
static volatile uint16_t exportReadPos = 0;

// Returns free slots in export queue (one slot kept empty to distinguish full/empty)
static inline uint16_t exportBufferAvailableForWrite() {
    return (uint16_t)((exportReadPos - exportWritePos - 1 + RS485_EXPORT_BUFFER_SIZE) % RS485_EXPORT_BUFFER_SIZE);
}

// ============================================================================
// STATE VARIABLES
// ============================================================================

static SlaveState state = SlaveState::RX_SYNC;
static bool initialized = false;
static int64_t lastRxTime = 0;

// Packet parsing
static uint8_t packetAddr = 0;
static uint8_t packetMsgType = 0;
static uint8_t packetLength = 0;
static uint8_t packetChecksum = 0;
static uint16_t packetDataIdx = 0;

// Skip state
static uint8_t skipRemaining = 0;

// V2.3 FIX: Data type for filtering what goes into export buffer
// Standalone only buffers data when rxSlaveAddress==0 && rxMsgType==0
enum RxDataType : uint8_t {
    RXDATA_IGNORE,
    RXDATA_DCSBIOS_EXPORT
};
static RxDataType rxDataType = RXDATA_IGNORE;

// Timing
static uint32_t lastPollMs = 0;
static uint32_t rxStartUs = 0;  // For RX timeout detection

// Statistics
static uint32_t statPolls = 0;
static uint32_t statBroadcasts = 0;
static uint32_t statExportBytes = 0;
static uint32_t statCommandsSent = 0;
static uint32_t statChecksumErrors = 0;

// ============================================================================
// FREERTOS TASK (when RS485_USE_TASK=1)
// ============================================================================

#if RS485_USE_TASK
static TaskHandle_t rs485TaskHandle = nullptr;
static volatile bool taskRunning = false;

// Forward declaration for the internal loop
static void rs485SlaveLoop();

// FreeRTOS task function
static void rs485SlaveTask(void* param) {
    (void)param;

    debugPrintln("[RS485S] Task started");

    TickType_t lastWakeTime = xTaskGetTickCount();

    while (taskRunning) {
        // Run the slave loop
        rs485SlaveLoop();

        // Wait for next tick interval
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(RS485_TASK_TICK_INTERVAL));
    }

    debugPrintln("[RS485S] Task stopped");
    vTaskDelete(nullptr);
}
#endif // RS485_USE_TASK

// ============================================================================
// FORWARD DECLARATION - CockpitOS integration
// ============================================================================

// V2.3: Use byte-by-byte parser function instead of batch processing
extern void processDcsBiosExportByte(uint8_t c);

// ============================================================================
// DE PIN CONTROL (Driver mode)
// ============================================================================

#if RS485_DE_PIN >= 0
static inline void setDE(bool high) {
    gpio_set_level((gpio_num_t)RS485_DE_PIN, high ? 1 : 0);
}
#else
#define setDE(x)
#endif

// ============================================================================
// PROCESS EXPORT DATA (drain ring buffer to CockpitOS parser)
// ============================================================================
// V2.3: Changed to ring buffer draining with byte-by-byte processing
// This matches the standalone slave.ino exactly.

static void processExportData() {
    while (exportReadPos != exportWritePos) {
        uint8_t c = exportBuffer[exportReadPos];
        exportReadPos = (exportReadPos + 1) % RS485_EXPORT_BUFFER_SIZE;
        statExportBytes++;

        // Feed byte-by-byte to CockpitOS DCS-BIOS parser
        processDcsBiosExportByte(c);
    }
}

// ============================================================================
// TX RESPONSE (send queued commands to master) - Driver Mode
// ============================================================================

static void sendResponse() {
    uint16_t toSend = txCount();
    if (toSend > 253) toSend = 253;  // Max that fits in Length byte

    // Build response packet
    uint8_t txBuf[RS485_TX_BUFFER_SIZE + 4];  // Length + MsgType + Data + Checksum
    uint8_t txLen = 0;

    if (toSend == 0) {
        // Empty response - just [0x00]
        txBuf[txLen++] = 0x00;
    } else {
        // Response with data: [Length][MsgType=0][Data...][Checksum]
        uint8_t checksum = (uint8_t)toSend;  // Start with length

        txBuf[txLen++] = (uint8_t)toSend;  // Length
        txBuf[txLen++] = MSGTYPE_DCSBIOS;  // MsgType
        checksum ^= MSGTYPE_DCSBIOS;

        // Copy data from TX buffer
        for (uint16_t i = 0; i < toSend; i++) {
            uint8_t c = txBuffer[txTail % RS485_TX_BUFFER_SIZE];
            txBuf[txLen++] = c;
            checksum ^= c;
            txTail++;
            txCount_val--;
        }

        // Add checksum
        #if RS485_ARDUINO_COMPAT
        txBuf[txLen++] = CHECKSUM_FIXED;
        #else
        txBuf[txLen++] = checksum;
        #endif

        statCommandsSent++;
    }

    // Enable DE for transmission
    #if RS485_DE_PIN >= 0
    setDE(true);
    ets_delay_us(TX_WARMUP_DELAY_MANUAL_US);
    #else
    ets_delay_us(TX_WARMUP_DELAY_AUTO_US);
    #endif

    // Send via driver
    uart_write_bytes((uart_port_t)RS485_UART_NUM, (const char*)txBuf, txLen);
    uart_wait_tx_done((uart_port_t)RS485_UART_NUM, pdMS_TO_TICKS(10));

    // Release DE
    setDE(false);

    // Flush any echo bytes from RX buffer
    uart_flush_input((uart_port_t)RS485_UART_NUM);

    state = SlaveState::RX_WAIT_ADDRESS;
}

// ============================================================================
// PACKET HANDLING
// ============================================================================

static void handlePacketComplete() {
    if (packetAddr == ADDR_BROADCAST) {
        // Broadcast packet - mark for deferred processing, no response
        statBroadcasts++;
        // NOTE: Export data stays in buffer, will be processed in loop AFTER
        // response timing is complete. This matches standalone behavior.
        state = SlaveState::RX_WAIT_ADDRESS;
    }
    else if (packetAddr == RS485_SLAVE_ADDRESS) {
        // Poll for us - respond IMMEDIATELY (timing critical!)
        // V2.3 FIX: Match standalone - only respond if MsgType == 0!
        if (packetMsgType == MSGTYPE_DCSBIOS) {
            statPolls++;
            lastPollMs = millis();
            sendResponse();
            // Export data will be processed in the main loop after response is sent
        } else {
            // Non-zero MsgType for us = resync (match standalone behavior)
            state = SlaveState::RX_SYNC;
        }
    }
    else {
        // Poll for another slave - skip their response
        state = SlaveState::RX_SKIP_LENGTH;
    }
}

// ============================================================================
// PROCESS RX BYTE - State machine for each received byte (Driver Mode)
// ============================================================================

static void processRxByte(uint8_t c) {
    int64_t now = esp_timer_get_time();

    // Sync detection - if gap > 500µs, reset to wait for address
    if (state == SlaveState::RX_SYNC) {
        if ((now - lastRxTime) >= SYNC_TIMEOUT_US) {
            state = SlaveState::RX_WAIT_ADDRESS;
            // Fall through to process this byte as address
        } else {
            lastRxTime = now;
            return;  // Stay in sync, discard byte
        }
    }

    switch (state) {
        case SlaveState::RX_WAIT_ADDRESS:
            // V2.3 FIX: Match standalone exactly - NO address validation!
            // The standalone slave.ino just stores the address without checking.
            // Validation was breaking sync on valid packets.
            packetAddr = c;
            packetChecksum = c;
            rxStartUs = micros();  // Start timeout tracking
            state = SlaveState::RX_WAIT_MSGTYPE;
            break;

        case SlaveState::RX_WAIT_MSGTYPE:
            // V2.3 FIX: Match standalone exactly - NO MsgType validation!
            // The standalone slave.ino just stores MsgType, doesn't validate.
            // Rejecting non-zero MsgType was breaking valid protocol messages.
            packetMsgType = c;
            packetChecksum ^= c;
            state = SlaveState::RX_WAIT_LENGTH;
            break;

        case SlaveState::RX_WAIT_LENGTH:
            packetLength = c;
            packetChecksum ^= c;
            packetDataIdx = 0;

            if (packetLength == 0) {
                // CRITICAL: Length=0 means NO DATA and NO CHECKSUM!
                // Packet is complete right now.
                handlePacketComplete();
            } else {
                // V2.3 FIX: Match standalone - only buffer export data!
                // Standalone: if (rxSlaveAddress == 0 && rxMsgType == 0) EXPORT else IGNORE
                if (packetAddr == ADDR_BROADCAST && packetMsgType == MSGTYPE_DCSBIOS) {
                    rxDataType = RXDATA_DCSBIOS_EXPORT;
                } else {
                    rxDataType = RXDATA_IGNORE;
                }
                state = SlaveState::RX_WAIT_DATA;
            }
            break;

        case SlaveState::RX_WAIT_DATA:
            // V2.3 FIX: Only buffer DCS-BIOS export data (broadcast + MsgType=0)
            // Standalone checks rxDataType, not address
            if (rxDataType == RXDATA_DCSBIOS_EXPORT) {
                if (exportBufferAvailableForWrite() == 0) {
                    // Buffer overflow! Match AVR behavior: force re-sync, don't silently drop
                    #if RS485_DEBUG_VERBOSE
                    debugPrintln("[RS485S] Export buffer overflow, forcing resync");
                    #endif
                    state = SlaveState::RX_SYNC;
                    exportWritePos = 0;
                    exportReadPos = 0;
                    lastRxTime = now;
                    break;
                }
                exportBuffer[exportWritePos] = c;
                exportWritePos = (exportWritePos + 1) % RS485_EXPORT_BUFFER_SIZE;
            }
            packetChecksum ^= c;
            packetDataIdx++;

            if (packetDataIdx >= packetLength) {
                state = SlaveState::RX_WAIT_CHECKSUM;
            }
            break;

        case SlaveState::RX_WAIT_CHECKSUM:
            // Verify checksum
            if (c != packetChecksum) {
                statChecksumErrors++;
                #if RS485_DEBUG_VERBOSE
                debugPrintf("[RS485S] Checksum error: got 0x%02X, expected 0x%02X\n", c, packetChecksum);
                #endif
                // Still process - some masters use 0x72 fixed
            }
            handlePacketComplete();
            break;

        // ============================================================
        // SKIP STATES - Skip another slave's response
        // ============================================================

        case SlaveState::RX_SKIP_LENGTH:
            if (c == 0x00) {
                // Other slave had no data - ready for next packet
                state = SlaveState::RX_WAIT_ADDRESS;
            } else {
                // Skip: [MsgType] + [Data x Length] + [Checksum]
                skipRemaining = c + 2;  // MsgType + Data + Checksum
                state = SlaveState::RX_SKIP_DATA;
            }
            break;

        case SlaveState::RX_SKIP_DATA:
            skipRemaining--;
            if (skipRemaining == 0) {
                state = SlaveState::RX_WAIT_ADDRESS;
            }
            break;

        case SlaveState::RX_SKIP_CHECKSUM:
            // (Not used - combined into RX_SKIP_DATA)
            state = SlaveState::RX_WAIT_ADDRESS;
            break;

        case SlaveState::TX_RESPOND:
            // Shouldn't receive data while transmitting
            break;

        case SlaveState::RX_SYNC:
            // Handled above
            break;

        default:
            state = SlaveState::RX_SYNC;
            break;
    }

    lastRxTime = now;
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool RS485Slave_init() {
    if (initialized) return true;

    // Initialize state
    txHead = txTail = 0;
    txCount_val = 0;
    exportWritePos = 0;
    exportReadPos = 0;
    state = SlaveState::RX_SYNC;
    lastRxTime = esp_timer_get_time();

    // Configure DE pin first (before UART)
    #if RS485_DE_PIN >= 0
    gpio_config_t de_conf = {
        .pin_bit_mask = (1ULL << RS485_DE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&de_conf);
    setDE(false);  // Start in RX mode
    #endif

    // =========================================================================
    // ESP-IDF UART DRIVER SETUP - Portable across ALL ESP32 variants
    // Uses DMA-backed RX buffer for robust data capture during TX
    // =========================================================================

    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };

    // Install driver with RX buffer (256) and TX buffer (256)
    esp_err_t err = uart_driver_install((uart_port_t)RS485_UART_NUM, 256, 256, 0, NULL, 0);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ERROR: uart_driver_install failed: %d\n", err);
        return false;
    }

    err = uart_param_config((uart_port_t)RS485_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ERROR: uart_param_config failed: %d\n", err);
        return false;
    }

    err = uart_set_pin((uart_port_t)RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ERROR: uart_set_pin failed: %d\n", err);
        return false;
    }

    // Ensure RX pin has pullup for stable idle state
    gpio_set_pull_mode((gpio_num_t)RS485_RX_PIN, GPIO_PULLUP_ONLY);

    // Flush any stale data
    uart_flush_input((uart_port_t)RS485_UART_NUM);

    initialized = true;

    debugPrintln("[RS485S] ═══════════════════════════════════════════════");
    debugPrintf("[RS485S] SLAVE INITIALIZED (Driver Mode)\n");
    debugPrintf("[RS485S]   Address: %d\n", RS485_SLAVE_ADDRESS);
    debugPrintf("[RS485S]   Baud: %d\n", RS485_BAUD);
    debugPrintf("[RS485S]   TX Pin: GPIO%d\n", RS485_TX_PIN);
    debugPrintf("[RS485S]   RX Pin: GPIO%d\n", RS485_RX_PIN);
    #if RS485_DE_PIN >= 0
    debugPrintf("[RS485S]   DE Pin: GPIO%d (manual)\n", RS485_DE_PIN);
    #else
    debugPrintln("[RS485S]   DE Pin: Auto-direction");
    #endif
    #if RS485_ARDUINO_COMPAT
    debugPrintln("[RS485S]   Protocol: Arduino-compatible (0x72 checksum)");
    #else
    debugPrintln("[RS485S]   Protocol: Full protocol (XOR checksum)");
    #endif
    debugPrintln("[RS485S]   RX: ESP-IDF UART driver (DMA-backed)");
    debugPrintln("[RS485S]   TX: uart_write_bytes + flush echo");

    // Create FreeRTOS task if enabled
    #if RS485_USE_TASK
    taskRunning = true;

    #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || \
        defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
    // Single-core chips - use xTaskCreate (no core affinity)
    BaseType_t result = xTaskCreate(
        rs485SlaveTask,
        "RS485S",
        RS485_TASK_STACK_SIZE,
        nullptr,
        RS485_TASK_PRIORITY,
        &rs485TaskHandle
    );
    #else
    // Dual-core chips - pin to specified core
    BaseType_t result = xTaskCreatePinnedToCore(
        rs485SlaveTask,
        "RS485S",
        RS485_TASK_STACK_SIZE,
        nullptr,
        RS485_TASK_PRIORITY,
        &rs485TaskHandle,
        RS485_TASK_CORE
    );
    #endif

    if (result != pdPASS) {
        debugPrintln("[RS485S] ERROR: Failed to create task!");
        taskRunning = false;
        return false;
    }

    debugPrintf("[RS485S]   Execution: FreeRTOS task (priority %d)\n", RS485_TASK_PRIORITY);
    #else
    debugPrintln("[RS485S]   Execution: Main loop");
    #endif

    debugPrintln("[RS485S] ═══════════════════════════════════════════════");

    return true;
}

// ============================================================================
// INTERNAL SLAVE LOOP (called from task or main loop) - Driver Mode
// ============================================================================

static void rs485SlaveLoop() {
    if (!initialized) return;

    // Driver mode: read bytes from UART driver and process through state machine
    uint8_t rxBuf[64];
    int len = uart_read_bytes((uart_port_t)RS485_UART_NUM, rxBuf, sizeof(rxBuf), 0);
    for (int i = 0; i < len; i++) {
        processRxByte(rxBuf[i]);
        // NOTE: sendResponse() is now called IMMEDIATELY from handlePacketComplete()
        // to match standalone slave timing. No deferred TX_RESPOND check needed.
    }

    // Process any buffered export data (deferred from packet handling)
    // This matches standalone behavior: respond first, then process export data
    processExportData();

    // Check for RX timeout (packet-level timeout, separate from sync gap detection)
    if (state != SlaveState::RX_WAIT_ADDRESS &&
        state != SlaveState::RX_SYNC) {
        uint32_t elapsed = micros() - rxStartUs;
        if (elapsed > RS485_RX_TIMEOUT_US) {
            #if RS485_DEBUG_VERBOSE
            debugPrintln("[RS485S] RX timeout, resync");
            #endif
            state = SlaveState::RX_SYNC;
        }
    }

    // Periodic status
    #if RS485_STATUS_INTERVAL_MS > 0
    static uint32_t lastStatusMs = 0;
    if (millis() - lastStatusMs >= RS485_STATUS_INTERVAL_MS) {
        lastStatusMs = millis();

        debugPrintf("[RS485S] Polls=%lu Bcasts=%lu Export=%lu Cmds=%lu TxPend=%d\n",
                    statPolls, statBroadcasts, statExportBytes, statCommandsSent, txCount());

        if (lastPollMs > 0) {
            debugPrintf("[RS485S] Last poll: %lu ms ago\n", millis() - lastPollMs);
        }
    }
    #endif
}

// ============================================================================
// PUBLIC LOOP FUNCTION
// ============================================================================

void RS485Slave_loop() {
    #if RS485_USE_TASK
    // Task mode: nothing to do here - task handles everything
    // This function exists for API compatibility
    #else
    // Main loop mode: run the slave loop directly
    rs485SlaveLoop();
    #endif
}

/**
 * Queue an input command to be sent when polled
 * Format: "LABEL VALUE\n"
 */
bool RS485Slave_queueCommand(const char* label, const char* value) {
    if (!initialized) return false;

    size_t labelLen = strlen(label);
    size_t valueLen = strlen(value);
    size_t needed = labelLen + 1 + valueLen + 1;  // space + newline

    if (needed > RS485_TX_BUFFER_SIZE - txCount()) {
        #if RS485_DEBUG_VERBOSE
        debugPrintf("[RS485S] TX buffer full, dropping: %s %s\n", label, value);
        #endif
        return false;
    }

    // Add to ring buffer (interrupt-safe via atomic operations)
    portDISABLE_INTERRUPTS();

    for (size_t i = 0; i < labelLen; i++) {
        txBuffer[txHead % RS485_TX_BUFFER_SIZE] = label[i];
        txHead++;
    }
    txBuffer[txHead % RS485_TX_BUFFER_SIZE] = ' ';
    txHead++;
    for (size_t i = 0; i < valueLen; i++) {
        txBuffer[txHead % RS485_TX_BUFFER_SIZE] = value[i];
        txHead++;
    }
    txBuffer[txHead % RS485_TX_BUFFER_SIZE] = '\n';
    txHead++;
    txCount_val += needed;

    portENABLE_INTERRUPTS();

    #if RS485_DEBUG_VERBOSE
    debugPrintf("[RS485S] Queued: %s %s (pending=%d)\n", label, value, txCount());
    #endif

    return true;
}

uint32_t RS485Slave_getPollCount() {
    return statPolls;
}

uint32_t RS485Slave_getBroadcastCount() {
    return statBroadcasts;
}

uint32_t RS485Slave_getExportBytesReceived() {
    return statExportBytes;
}

uint32_t RS485Slave_getCommandsSent() {
    return statCommandsSent;
}

size_t RS485Slave_getTxBufferPending() {
    return txCount();
}

uint32_t RS485Slave_getTimeSinceLastPoll() {
    if (lastPollMs == 0) return 0xFFFFFFFF;
    return millis() - lastPollMs;
}

void RS485Slave_printStatus() {
    debugPrintln("\n[RS485S] ══════════════ SLAVE STATUS ══════════════");
    debugPrintf("[RS485S] Address: %d\n", RS485_SLAVE_ADDRESS);
    debugPrintf("[RS485S] State: %d\n", (int)state);
    debugPrintf("[RS485S] Polls received: %lu\n", statPolls);
    debugPrintf("[RS485S] Broadcasts received: %lu\n", statBroadcasts);
    debugPrintf("[RS485S] Export bytes RX: %lu\n", statExportBytes);
    debugPrintf("[RS485S] Commands sent: %lu\n", statCommandsSent);
    debugPrintf("[RS485S] TX buffer pending: %d bytes\n", txCount());
    debugPrintf("[RS485S] Checksum errors: %lu\n", statChecksumErrors);
    debugPrintf("[RS485S] Time since last poll: %lu ms\n", RS485Slave_getTimeSinceLastPoll());
    #if RS485_USE_TASK
    debugPrintf("[RS485S] Execution: FreeRTOS task (priority %d)\n", RS485_TASK_PRIORITY);
    #else
    debugPrintln("[RS485S] Execution: Main loop");
    #endif
    debugPrintln("[RS485S] ═════════════════════════════════════════════\n");
}

void RS485Slave_stop() {
    #if RS485_USE_TASK
    if (taskRunning && rs485TaskHandle) {
        taskRunning = false;
        // Wait for task to finish
        vTaskDelay(pdMS_TO_TICKS(100));
        rs485TaskHandle = nullptr;
        debugPrintln("[RS485S] Task stopped");
    }
    #endif

    // Uninstall UART driver
    if (initialized) {
        uart_driver_delete((uart_port_t)RS485_UART_NUM);
    }

    initialized = false;
    debugPrintln("[RS485S] Stopped");
}

#endif // RS485_SLAVE_ENABLED
