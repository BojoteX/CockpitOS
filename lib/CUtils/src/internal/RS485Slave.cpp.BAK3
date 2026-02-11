/**
 * @file RS485Slave.cpp
 * @brief RS-485 Slave Driver for CockpitOS - v3.1 TX_DONE Non-Blocking
 *
 * ARCHITECTURE (v3.1 - Dual-Path):
 *
 *   RS485_USE_ISR_MODE 1 (default, recommended):
 *     - UART RX interrupt fires immediately when byte arrives (FIFO threshold=1)
 *     - State machine runs IN the ISR - zero polling latency
 *     - Response TX is NON-BLOCKING: FIFO loaded, TX_DONE interrupt handles cleanup
 *     - ISR cost is O(1) constant time (~8us) regardless of response size
 *     - Matches AVR's TXC interrupt architecture, surpasses it via 128-byte FIFO
 *     - Uses periph_module_enable (no driver install) for bare-metal UART access
 *     - RISC-V fence instruction for FIFO read stability on C3/C6/H2
 *     - ELIMINATES driver-mode timestamp blindness (batch reading issue)
 *     - FreeRTOS task only used for export data processing (not RX)
 *
 *   RS485_USE_ISR_MODE 0 (fallback):
 *     - Uses ESP-IDF UART driver for portable RX/TX
 *     - State machine runs in task/main loop (polling-based)
 *     - May have ~10% glitches due to batch-read timestamp blindness
 *     - Use this if ISR mode doesn't work on your specific chip
 *
 * V3.0 IMPROVEMENT - ISR-DRIVEN RX:
 *   ISR mode processes each byte at actual arrival time, eliminating
 *   batch-read timestamp blindness from the driver-based polling approach.
 *
 * V3.1 IMPROVEMENT - TX_DONE NON-BLOCKING RESPONSE:
 *   Previous ISR mode blocked the CPU for the entire TX duration (spin-wait
 *   on uart_ll_is_tx_idle). V3.1 uses UART_INTR_TX_DONE hardware interrupt:
 *   load TX FIFO, arm interrupt, return immediately. TX_DONE ISR handles
 *   bus turnaround (echo flush, DE release, RX re-enable). Reduces ISR
 *   blocking from ~220us average to ~8us constant. Enables WiFi/BLE on
 *   slave devices and makes single-core chips (C3/C6/H2) fully viable.
 *
 * PROTOCOL: 100% compatible with Arduino DCS-BIOS RS485 implementation
 *
 * FULLY COMPATIBLE WITH:
 *   - Arduino DCS-BIOS RS485 Master (DcsBiosNgRS485Master)
 *   - CockpitOS RS485 Master (SMART and RELAY modes)
 *   - ESP32 DCS-BIOS Library RS485 Master
 *   - Standalone ESP32 RS485 Master
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
#include "driver/gpio.h"
#include "rom/ets_sys.h"        // For ets_delay_us()
#include "esp_timer.h"

// UART driver (always needed for uart_param_config, uart_set_pin)
#include "driver/uart.h"

#if RS485_USE_ISR_MODE
// ISR mode needs direct hardware access
#include "hal/uart_ll.h"
#include "hal/gpio_ll.h"
#include "soc/uart_struct.h"
#include "soc/gpio_struct.h"
#include "soc/uart_periph.h"
#include "esp_intr_alloc.h"
#include "esp_rom_gpio.h"
// Peripheral module control - handle both old and new ESP-IDF locations
#if __has_include("esp_private/periph_ctrl.h")
#include "esp_private/periph_ctrl.h"
#else
#include "driver/periph_ctrl.h"
#endif
#include "soc/periph_defs.h"
#endif // RS485_USE_ISR_MODE

// FreeRTOS includes (for task mode)
#if RS485_USE_TASK
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

// ============================================================================
// UART HARDWARE MAPPING (ISR mode only)
// ============================================================================

#if RS485_USE_ISR_MODE
#if RS485_UART_NUM == 0
static uart_dev_t* const uartHw = &UART0;
#define RS485_PERIPH_MODULE PERIPH_UART0_MODULE
#elif RS485_UART_NUM == 1
static uart_dev_t* const uartHw = &UART1;
#define RS485_PERIPH_MODULE PERIPH_UART1_MODULE
#elif RS485_UART_NUM == 2
static uart_dev_t* const uartHw = &UART2;
#define RS485_PERIPH_MODULE PERIPH_UART2_MODULE
#else
#error "Invalid RS485_UART_NUM (must be 0, 1, or 2)"
#endif
static intr_handle_t uartIntrHandle = nullptr;
#endif // RS485_USE_ISR_MODE

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

// TX timing uses config defines directly:
//   RS485_TX_WARMUP_DELAY_US        (manual DE warmup)
//   RS485_TX_WARMUP_AUTO_DELAY_US   (auto-direction warmup)
//   RS485_TX_COOLDOWN_DELAY_US      (manual DE cooldown)
//   RS485_TX_COOLDOWN_AUTO_DELAY_US (auto-direction cooldown)

// Sync detection timeout (from config - do not hardcode here)
#define SYNC_TIMEOUT_US              RS485_SYNC_TIMEOUT_US

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
    TX_RESPOND,             // Driver mode: blocking TX in progress
    TX_WAITING_DONE         // ISR mode: FIFO loaded, waiting for TX_DONE interrupt
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

// Static TX frame buffer for ISR-mode non-blocking TX.
// Safe as static because the slave is half-duplex — only one response in flight.
// Avoids allocating 132 bytes on the ISR stack.
#if RS485_USE_ISR_MODE
static uint8_t DRAM_ATTR txFrameBuf[RS485_TX_BUFFER_SIZE + 4];
#endif

// ============================================================================
// EXPORT DATA RING BUFFER (for broadcast packets)
// ============================================================================
// Ring buffer for export data - ISR writes, task/loop reads.
// One slot kept empty to distinguish full vs empty.

static volatile uint8_t exportBuffer[RS485_EXPORT_BUFFER_SIZE];
static volatile uint16_t exportWritePos = 0;
static volatile uint16_t exportReadPos = 0;

// Returns free slots in export queue
static inline uint16_t IRAM_ATTR exportBufferAvailableForWrite() {
    return (uint16_t)((exportReadPos - exportWritePos - 1 + RS485_EXPORT_BUFFER_SIZE) % RS485_EXPORT_BUFFER_SIZE);
}

// ============================================================================
// STATE VARIABLES (volatile for ISR access in ISR mode)
// ============================================================================

static volatile SlaveState state = SlaveState::RX_SYNC;
static bool initialized = false;
static volatile int64_t lastRxTime = 0;

// Packet parsing
static volatile uint8_t packetAddr = 0;
static volatile uint8_t packetMsgType = 0;
static volatile uint8_t packetLength = 0;
static volatile uint16_t packetDataIdx = 0;

// Skip state
static volatile uint8_t skipRemaining = 0;

// Data type for filtering what goes into export buffer
enum RxDataType : uint8_t {
    RXDATA_IGNORE,
    RXDATA_DCSBIOS_EXPORT
};
static volatile RxDataType rxDataType = RXDATA_IGNORE;

// Timing
static volatile uint32_t lastPollMs = 0;

// Statistics
static volatile uint32_t statPolls = 0;
static volatile uint32_t statBroadcasts = 0;
static volatile uint32_t statExportBytes = 0;
static volatile uint32_t statCommandsSent = 0;

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

// Byte-by-byte parser function for DCS-BIOS export data
extern void processDcsBiosExportByte(uint8_t c);

// ============================================================================
// DE PIN CONTROL
// ============================================================================

#if RS485_DE_PIN >= 0

// Regular (non-ISR) DE control
static inline void setDE(bool high) {
    gpio_set_level((gpio_num_t)RS485_DE_PIN, high ? 1 : 0);
}

#if RS485_USE_ISR_MODE
// ISR-safe DE control (direct register access, no function call overhead)
static inline void IRAM_ATTR setDE_ISR(bool high) {
    gpio_ll_set_level(&GPIO, (gpio_num_t)RS485_DE_PIN, high ? 1 : 0);
}
#endif

#else
#define setDE(x)
#if RS485_USE_ISR_MODE
#define setDE_ISR(x)
#endif
#endif

// ============================================================================
// PROCESS EXPORT DATA (drain ring buffer to CockpitOS parser)
// ============================================================================
// Drains the export ring buffer byte-by-byte to the DCS-BIOS protocol parser.
// Runs in FreeRTOS task or main loop - NOT in ISR context.

static void processExportData() {
    while (exportReadPos != exportWritePos) {
        uint8_t c = exportBuffer[exportReadPos];
        exportReadPos = (exportReadPos + 1) % RS485_EXPORT_BUFFER_SIZE;
        statExportBytes++;

        // Feed byte-by-byte to CockpitOS DCS-BIOS parser
        processDcsBiosExportByte(c);
    }
}


// ############################################################################
//
//    ISR MODE IMPLEMENTATION
//
// ############################################################################

#if RS485_USE_ISR_MODE

// ============================================================================
// ISR-MODE: TX HELPERS
// ============================================================================

// ============================================================================
// ISR-MODE: SEND RESPONSE — NON-BLOCKING (called from ISR when polled)
// ============================================================================
// V3.1 Pattern: disableRx → DE assert → warmup → build frame → load FIFO →
//               arm TX_DONE → return (CPU free!)
//
// When TX_DONE fires: flush echo → DE release → enableRx → back to RX_WAIT
//
// Cooldown delay is ELIMINATED. TX_DONE fires when the last bit has physically
// left the shift register — the bus can be released immediately.
// Auto-direction warmup is eliminated (hardware switches in nanoseconds).

static void IRAM_ATTR sendResponseISR() {
    uint16_t toSend = txCount_val;
    if (toSend > 253) toSend = 253;

    // Disable RX interrupt during TX to prevent echo bytes triggering ISR
    uart_ll_disable_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);

    // Warmup — only needed for manual DE pin devices (transceiver settling)
    // Auto-direction devices switch in nanoseconds on TX activity, no delay needed
    #if RS485_DE_PIN >= 0
    setDE_ISR(true);
    #if RS485_TX_WARMUP_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_DELAY_US);
    #endif
    #endif

    // Build complete response in static buffer (not stack — saves ~132 bytes ISR stack)
    uint8_t txLen = 0;

    if (toSend == 0) {
        txFrameBuf[txLen++] = 0x00;
    } else {
        uint8_t checksum = (uint8_t)toSend;
        txFrameBuf[txLen++] = (uint8_t)toSend;
        txFrameBuf[txLen++] = MSGTYPE_DCSBIOS;
        checksum ^= MSGTYPE_DCSBIOS;

        for (uint16_t i = 0; i < toSend; i++) {
            uint8_t c = txBuffer[txTail % RS485_TX_BUFFER_SIZE];
            txFrameBuf[txLen++] = c;
            checksum ^= c;
            txTail++;
            txCount_val--;
        }

        #if RS485_ARDUINO_COMPAT
        txFrameBuf[txLen++] = CHECKSUM_FIXED;
        #else
        txFrameBuf[txLen++] = checksum;
        #endif

        statCommandsSent++;
    }

    // Load TX FIFO in one burst — hardware shifts bytes out autonomously
    uart_ll_write_txfifo(uartHw, txFrameBuf, txLen);

    // Arm TX_DONE interrupt — fires when last bit leaves the shift register
    // Clear any stale TX_DONE flag first, then enable
    uart_ll_clr_intsts_mask(uartHw, UART_INTR_TX_DONE);
    uart_ll_ena_intr_mask(uartHw, UART_INTR_TX_DONE);

    // Mark state — ISR handler will complete the bus turnaround when TX_DONE fires
    state = SlaveState::TX_WAITING_DONE;

    // Return immediately — CPU is FREE while hardware shifts out the bytes
    // TX_DONE ISR will handle: flush echo, DE LOW, re-enable RX, back to RX_WAIT
}

// ============================================================================
// ISR-MODE: UART RX INTERRUPT HANDLER
// ============================================================================
// Fires immediately when byte arrives (FIFO threshold = 1).
// Processes the full state machine in ISR context for minimum latency.
// Sends response immediately when our address is polled.

static void IRAM_ATTR uart_isr_handler(void* arg) {
    uint32_t uart_intr_status = uart_ll_get_intsts_mask(uartHw);

    // ================================================================
    // TX_DONE: Transmission complete — do bus turnaround
    // ================================================================
    // Fires when the last bit of the last byte has physically left the
    // UART shift register. This is the earliest safe moment to release
    // the bus. No cooldown delay needed — the transceiver switches in
    // nanoseconds once DE goes LOW.
    if (uart_intr_status & UART_INTR_TX_DONE) {
        // Flush any echo bytes that accumulated in RX FIFO during TX
        uart_ll_rxfifo_rst(uartHw);

        // Release the bus (DE LOW for manual direction devices)
        #if RS485_DE_PIN >= 0
        setDE_ISR(false);
        #endif

        // Re-enable RX interrupt — receiver is active again
        uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL);
        uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);

        // Disable TX_DONE interrupt — one-shot, don't need it until next response
        uart_ll_disable_intr_mask(uartHw, UART_INTR_TX_DONE);
        uart_ll_clr_intsts_mask(uartHw, UART_INTR_TX_DONE);

        // Back to receive mode
        state = SlaveState::RX_WAIT_ADDRESS;
    }

    // ================================================================
    // RXFIFO_FULL: Incoming byte — process state machine
    // ================================================================
    while (uart_ll_get_rxfifo_len(uartHw) > 0) {
        uint8_t c;
        uart_ll_read_rxfifo(uartHw, &c, 1);
#ifdef __riscv
        // RISC-V (C3, C6, H2): memory fence ensures FIFO read pointer update
        // propagates before next FIFO length check. Without this, the CPU can
        // read faster than the pointer updates, causing stale/duplicate reads.
        __asm__ __volatile__("fence");
#endif

        int64_t now = esp_timer_get_time();

        // Sync detection - if gap > 500us, reset to wait for address
        if (state == SlaveState::RX_SYNC) {
            if ((now - lastRxTime) >= SYNC_TIMEOUT_US) {
                state = SlaveState::RX_WAIT_ADDRESS;
                // Fall through to process this byte as address
            } else {
                lastRxTime = now;
                continue;  // Stay in sync, discard byte
            }
        }

        switch (state) {
            case SlaveState::RX_WAIT_ADDRESS:
                packetAddr = c;
                state = SlaveState::RX_WAIT_MSGTYPE;
                break;

            case SlaveState::RX_WAIT_MSGTYPE:
                packetMsgType = c;
                state = SlaveState::RX_WAIT_LENGTH;
                break;

            case SlaveState::RX_WAIT_LENGTH:
                packetLength = c;
                packetDataIdx = 0;

                if (packetLength == 0) {
                    // CRITICAL: Length=0 means NO DATA and NO CHECKSUM!
                    // Packet is complete right now.
                    if (packetAddr == ADDR_BROADCAST) {
                        // Broadcast with no data - ignore
                        statBroadcasts++;
                        state = SlaveState::RX_WAIT_ADDRESS;
                    }
                    else if (packetAddr == RS485_SLAVE_ADDRESS) {
                        // Poll for us — load FIFO and arm TX_DONE, return immediately
                        if (packetMsgType == MSGTYPE_DCSBIOS) {
                            statPolls++;
                            lastPollMs = (uint32_t)(now / 1000);
                            sendResponseISR();
                            // state is now TX_WAITING_DONE — exit the RX loop
                            // Any further RX bytes are echo and will be flushed by TX_DONE handler
                            goto done_rx;
                        } else {
                            state = SlaveState::RX_SYNC;
                        }
                    }
                    else {
                        // Poll for another slave - skip their response
                        state = SlaveState::RX_SKIP_LENGTH;
                    }
                } else {
                    // Has data - determine type for buffering
                    if (packetAddr == ADDR_BROADCAST && packetMsgType == MSGTYPE_DCSBIOS) {
                        rxDataType = RXDATA_DCSBIOS_EXPORT;
                    } else {
                        rxDataType = RXDATA_IGNORE;
                    }
                    state = SlaveState::RX_WAIT_DATA;
                }
                break;

            case SlaveState::RX_WAIT_DATA:
                // Queue export data for main loop processing
                if (rxDataType == RXDATA_DCSBIOS_EXPORT) {
                    if (exportBufferAvailableForWrite() == 0) {
                        // Buffer overflow! Force re-sync (match AVR behavior)
                        state = SlaveState::RX_SYNC;
                        lastRxTime = now;
                        exportReadPos = 0;
                        exportWritePos = 0;
                        goto done_byte;  // Skip remaining processing for this byte
                    }
                    exportBuffer[exportWritePos] = c;
                    exportWritePos = (exportWritePos + 1) % RS485_EXPORT_BUFFER_SIZE;
                }
                packetDataIdx++;

                if (packetDataIdx >= packetLength) {
                    state = SlaveState::RX_WAIT_CHECKSUM;
                }
                break;

            case SlaveState::RX_WAIT_CHECKSUM:
                // Checksum intentionally ignored (AVR behavior, checksum is not validated in protocol)

                // Handle completed packet
                if (packetAddr == ADDR_BROADCAST) {
                    statBroadcasts++;
                    state = SlaveState::RX_WAIT_ADDRESS;
                }
                else if (packetAddr == RS485_SLAVE_ADDRESS) {
                    if (packetMsgType == MSGTYPE_DCSBIOS) {
                        statPolls++;
                        lastPollMs = (uint32_t)(now / 1000);
                        sendResponseISR();
                        // state is now TX_WAITING_DONE — exit the RX loop
                        goto done_rx;
                    } else {
                        state = SlaveState::RX_SYNC;
                    }
                }
                else {
                    state = SlaveState::RX_SKIP_LENGTH;
                }
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
                    skipRemaining = c + 2;
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
                state = SlaveState::RX_WAIT_ADDRESS;
                break;

            case SlaveState::TX_RESPOND:
            case SlaveState::TX_WAITING_DONE:
            case SlaveState::RX_SYNC:
                // TX_WAITING_DONE: should not receive RX bytes here (RX int is disabled)
                // If we somehow do, just ignore — TX_DONE handler will clean up
                break;

            default:
                state = SlaveState::RX_SYNC;
                break;
        }

done_byte:
        lastRxTime = now;
    }

done_rx:
    // Clear handled interrupt status
    uart_ll_clr_intsts_mask(uartHw, uart_intr_status);
}

// ============================================================================
// ISR-MODE: HARDWARE INITIALIZATION
// ============================================================================

static bool initRS485Hardware_ISR() {
    debugPrintln("[RS485S]   [1] Configuring DE GPIO pin...");

#if RS485_DE_PIN >= 0
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RS485_DE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    setDE_ISR(false);  // Start in RX mode
    debugPrintln("[RS485S]   [1] DE GPIO configured OK");
#else
    debugPrintln("[RS485S]   [1] No DE pin (auto-direction)");
#endif

    // =========================================================================
    // BARE-METAL UART SETUP
    // Uses periph_module_enable + uart_param_config WITHOUT installing the
    // full UART driver. This avoids stale interrupt allocation issues and
    // gives us direct FIFO access for lowest possible latency.
    // =========================================================================

    debugPrintln("[RS485S]   [2] Enabling UART peripheral module...");
    periph_module_enable(RS485_PERIPH_MODULE);

    debugPrintln("[RS485S]   [3] Configuring UART parameters...");
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };

    esp_err_t err = uart_param_config((uart_port_t)RS485_UART_NUM, &uart_config);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ERROR: uart_param_config failed: %d\n", err);
        return false;
    }

    debugPrintln("[RS485S]   [4] Setting UART pins...");
    err = uart_set_pin((uart_port_t)RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ERROR: uart_set_pin failed: %d\n", err);
        return false;
    }

    // Ensure RX pin has pullup for stable idle state
    gpio_set_pull_mode((gpio_num_t)RS485_RX_PIN, GPIO_PULLUP_ONLY);
    debugPrintln("[RS485S]   [4] UART pins configured OK");

    debugPrintln("[RS485S]   [5] Configuring RX FIFO threshold...");
    // Trigger interrupt on every byte for lowest latency
    uart_ll_set_rxfifo_full_thr(uartHw, 1);
    debugPrintln("[RS485S]   [5] RX FIFO threshold=1 OK");

    debugPrintln("[RS485S]   [6] Clearing and enabling interrupts...");
    uart_ll_clr_intsts_mask(uartHw, UART_LL_INTR_MASK);
    uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);
    debugPrintln("[RS485S]   [6] Interrupts configured OK");

    debugPrintln("[RS485S]   [7] Registering ISR...");
    err = esp_intr_alloc(uart_periph_signal[RS485_UART_NUM].irq,
                         ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1,
                         uart_isr_handler, NULL, &uartIntrHandle);
    if (err != ESP_OK) {
        debugPrintf("[RS485S] ERROR: esp_intr_alloc failed: %d\n", err);
        return false;
    }
    debugPrintln("[RS485S]   [7] ISR registered OK");

    state = SlaveState::RX_SYNC;
    lastRxTime = esp_timer_get_time();

    return true;
}

// ============================================================================
// ISR-MODE: SLAVE LOOP (only processes export data + status printing)
// ============================================================================
// In ISR mode, RX and TX are handled by the ISR.
// The task/loop only needs to drain the export buffer to the DCS-BIOS parser.

static void rs485SlaveLoop() {
    if (!initialized) return;

    // Process any buffered export data
    // ISR queues bytes into exportBuffer, we drain them here to the DCS-BIOS parser
    processExportData();

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


#else // !RS485_USE_ISR_MODE

// ############################################################################
//
//    DRIVER MODE IMPLEMENTATION (fallback)
//
// ############################################################################

// ============================================================================
// DRIVER-MODE: TIMING
// ============================================================================

static uint32_t rxStartUs = 0;  // For RX timeout detection

// ============================================================================
// DRIVER-MODE: TX RESPONSE (send queued commands to master)
// ============================================================================

// Fast path: no input data queued — send single 0x00 byte (99%+ of polls)
// Pattern: DE assert → warmup → TX → txDone → flush → DE release (NO cooldown)
static void sendZeroLengthResponse() {
    uint8_t zero = 0x00;

    // Warmup
    #if RS485_DE_PIN >= 0
    setDE(true);
    #if RS485_TX_WARMUP_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_DELAY_US);
    #endif
    #else
    #if RS485_TX_WARMUP_AUTO_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_AUTO_DELAY_US);
    #endif
    #endif

    // Blocking TX
    uart_write_bytes((uart_port_t)RS485_UART_NUM, (const char*)&zero, 1);
    uart_wait_tx_done((uart_port_t)RS485_UART_NUM, pdMS_TO_TICKS(10));

    // TX physically complete — flush echo + DE release IMMEDIATELY.
    // No cooldown delay. Matches ISR-mode TX_DONE behavior.
    #if RS485_DE_PIN >= 0
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    setDE(false);  // DE release
    #else
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    #endif

    state = SlaveState::RX_WAIT_ADDRESS;
}

// Slow path: input data queued — build and send full response packet
// Pattern: build packet → DE assert → warmup → TX → txDone → flush → DE release (NO cooldown)
static void sendResponse() {
    uint16_t toSend = txCount();
    if (toSend == 0) {
        sendZeroLengthResponse();
        return;
    }
    if (toSend > 253) toSend = 253;

    // Build response packet: [Length][MsgType=0][Data...][Checksum]
    uint8_t txBuf[RS485_TX_BUFFER_SIZE + 4];
    uint8_t txLen = 0;
    uint8_t checksum = (uint8_t)toSend;

    txBuf[txLen++] = (uint8_t)toSend;
    txBuf[txLen++] = MSGTYPE_DCSBIOS;
    checksum ^= MSGTYPE_DCSBIOS;

    for (uint16_t i = 0; i < toSend; i++) {
        uint8_t c = txBuffer[txTail % RS485_TX_BUFFER_SIZE];
        txBuf[txLen++] = c;
        checksum ^= c;
        txTail++;
        txCount_val--;
    }

    #if RS485_ARDUINO_COMPAT
    txBuf[txLen++] = CHECKSUM_FIXED;
    #else
    txBuf[txLen++] = checksum;
    #endif

    statCommandsSent++;

    // Warmup
    #if RS485_DE_PIN >= 0
    setDE(true);
    #if RS485_TX_WARMUP_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_DELAY_US);
    #endif
    #else
    #if RS485_TX_WARMUP_AUTO_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_AUTO_DELAY_US);
    #endif
    #endif

    // Blocking TX
    uart_write_bytes((uart_port_t)RS485_UART_NUM, (const char*)txBuf, txLen);
    uart_wait_tx_done((uart_port_t)RS485_UART_NUM, pdMS_TO_TICKS(10));

    // TX physically complete — flush echo + DE release IMMEDIATELY.
    // No cooldown delay. Matches ISR-mode TX_DONE behavior.
    #if RS485_DE_PIN >= 0
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    setDE(false);  // DE release
    #else
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    #endif

    state = SlaveState::RX_WAIT_ADDRESS;
}

// ============================================================================
// DRIVER-MODE: PACKET HANDLING
// ============================================================================

static void handlePacketComplete() {
    if (packetAddr == ADDR_BROADCAST) {
        statBroadcasts++;
        state = SlaveState::RX_WAIT_ADDRESS;
    }
    else if (packetAddr == RS485_SLAVE_ADDRESS) {
        if (packetMsgType == MSGTYPE_DCSBIOS) {
            statPolls++;
            lastPollMs = millis();
            sendResponse();
        } else {
            state = SlaveState::RX_SYNC;
        }
    }
    else {
        state = SlaveState::RX_SKIP_LENGTH;
    }
}

// ============================================================================
// DRIVER-MODE: PROCESS RX BYTE - State machine for each received byte
// ============================================================================

static void processRxByte(uint8_t c) {
    int64_t now = esp_timer_get_time();

    // Sync detection - if gap > 500us, reset to wait for address
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
            packetAddr = c;
            rxStartUs = micros();
            state = SlaveState::RX_WAIT_MSGTYPE;
            break;

        case SlaveState::RX_WAIT_MSGTYPE:
            packetMsgType = c;
            state = SlaveState::RX_WAIT_LENGTH;
            break;

        case SlaveState::RX_WAIT_LENGTH:
            packetLength = c;
            packetDataIdx = 0;

            if (packetLength == 0) {
                handlePacketComplete();
            } else {
                if (packetAddr == ADDR_BROADCAST && packetMsgType == MSGTYPE_DCSBIOS) {
                    rxDataType = RXDATA_DCSBIOS_EXPORT;
                } else {
                    rxDataType = RXDATA_IGNORE;
                }
                state = SlaveState::RX_WAIT_DATA;
            }
            break;

        case SlaveState::RX_WAIT_DATA:
            if (rxDataType == RXDATA_DCSBIOS_EXPORT) {
                if (exportBufferAvailableForWrite() == 0) {
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
            packetDataIdx++;

            if (packetDataIdx >= packetLength) {
                state = SlaveState::RX_WAIT_CHECKSUM;
            }
            break;

        case SlaveState::RX_WAIT_CHECKSUM:
            // Checksum intentionally ignored (AVR behavior, checksum is not validated in protocol)
            handlePacketComplete();
            break;

        // ============================================================
        // SKIP STATES - Skip another slave's response
        // ============================================================

        case SlaveState::RX_SKIP_LENGTH:
            if (c == 0x00) {
                state = SlaveState::RX_WAIT_ADDRESS;
            } else {
                skipRemaining = c + 2;
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
            state = SlaveState::RX_WAIT_ADDRESS;
            break;

        case SlaveState::TX_RESPOND:
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
// DRIVER-MODE: HARDWARE INITIALIZATION
// ============================================================================

static bool initRS485Hardware_Driver() {
    // Configure DE pin
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

    // ESP-IDF UART DRIVER SETUP
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

    state = SlaveState::RX_SYNC;
    lastRxTime = esp_timer_get_time();

    return true;
}

// ============================================================================
// DRIVER-MODE: SLAVE LOOP
// ============================================================================

static void rs485SlaveLoop() {
    if (!initialized) return;

    // Driver mode: read bytes from UART driver and process through state machine
    uint8_t rxBuf[64];
    int len = uart_read_bytes((uart_port_t)RS485_UART_NUM, rxBuf, sizeof(rxBuf), 0);
    for (int i = 0; i < len; i++) {
        processRxByte(rxBuf[i]);
    }

    // Process any buffered export data (deferred from packet handling)
    processExportData();

    // Check for RX timeout (packet-level timeout)
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

#endif // RS485_USE_ISR_MODE


// ############################################################################
//
//    PUBLIC API (shared between ISR and Driver modes)
//
// ############################################################################

// ============================================================================
// INITIALIZATION
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

    // Initialize hardware (mode-specific)
    bool hwOk;
#if RS485_USE_ISR_MODE
    hwOk = initRS485Hardware_ISR();
#else
    hwOk = initRS485Hardware_Driver();
#endif

    if (!hwOk) {
        debugPrintln("[RS485S] ERROR: Hardware initialization failed!");
        return false;
    }

    initialized = true;

    debugPrintln("[RS485S] ======================================================");
#if RS485_USE_ISR_MODE
    debugPrintf("[RS485S] SLAVE INITIALIZED (ISR Mode)\n");
#else
    debugPrintf("[RS485S] SLAVE INITIALIZED (Driver Mode)\n");
#endif
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
#if RS485_USE_ISR_MODE
    debugPrintln("[RS485S]   RX: ISR-driven (bare-metal UART, FIFO threshold=1)");
    debugPrintln("[RS485S]   TX: Non-blocking (FIFO burst + TX_DONE interrupt)");
#else
    debugPrintln("[RS485S]   RX: ESP-IDF UART driver (DMA-backed)");
    debugPrintln("[RS485S]   TX: uart_write_bytes + RX_SYNC transition");
#endif

    // Create FreeRTOS task if enabled
    // In ISR mode: task only drains export buffer (RX/TX handled by ISR)
    // In Driver mode: task runs the full slave loop (RX + TX + export)
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

    debugPrintln("[RS485S] ======================================================");

    return true;
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

    // Add to ring buffer (interrupt-safe via critical section)
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
    debugPrintln("\n[RS485S] ============== SLAVE STATUS ==============");
    debugPrintf("[RS485S] Address: %d\n", RS485_SLAVE_ADDRESS);
    debugPrintf("[RS485S] State: %d\n", (int)state);
#if RS485_USE_ISR_MODE
    debugPrintln("[RS485S] Mode: ISR (bare-metal UART)");
#else
    debugPrintln("[RS485S] Mode: Driver (ESP-IDF UART)");
#endif
    debugPrintf("[RS485S] Polls received: %lu\n", statPolls);
    debugPrintf("[RS485S] Broadcasts received: %lu\n", statBroadcasts);
    debugPrintf("[RS485S] Export bytes RX: %lu\n", statExportBytes);
    debugPrintf("[RS485S] Commands sent: %lu\n", statCommandsSent);
    debugPrintf("[RS485S] TX buffer pending: %d bytes\n", txCount());
    debugPrintf("[RS485S] Time since last poll: %lu ms\n", RS485Slave_getTimeSinceLastPoll());
    #if RS485_USE_TASK
    debugPrintf("[RS485S] Execution: FreeRTOS task (priority %d)\n", RS485_TASK_PRIORITY);
    #else
    debugPrintln("[RS485S] Execution: Main loop");
    #endif
    debugPrintln("[RS485S] ==============================================\n");
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

    if (initialized) {
#if RS485_USE_ISR_MODE
        // Free ISR and disable UART interrupts
        if (uartIntrHandle) {
            esp_intr_free(uartIntrHandle);
            uartIntrHandle = nullptr;
        }
        uart_ll_disable_intr_mask(uartHw, UART_LL_INTR_MASK);
        // NOTE: We don't call periph_module_disable() here because
        // other code might still need the UART peripheral
#else
        // Uninstall UART driver
        uart_driver_delete((uart_port_t)RS485_UART_NUM);
#endif
    }

    initialized = false;
    debugPrintln("[RS485S] Stopped");
}

#endif // RS485_SLAVE_ENABLED
