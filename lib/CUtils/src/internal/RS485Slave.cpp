/**
 * @file RS485Slave.cpp
 * @brief RS-485 Slave Driver for CockpitOS - v3.3 ISR-Only
 *
 * ARCHITECTURE (v3.3 - ISR-Only, bare-metal UART):
 *
 *   - UART RX interrupt fires immediately when byte arrives (FIFO threshold=1)
 *   - RXFIFO_TOUT safety net catches bytes in FIFO drain race window
 *   - State machine runs IN the ISR - zero polling latency
 *   - Response TX is NON-BLOCKING: FIFO loaded, TX_DONE interrupt handles cleanup
 *   - ISR cost is O(1) constant time (~8us) regardless of response size
 *   - Surpasses AVR's TXC architecture via 128-byte FIFO burst + zero-delay warmup
 *   - Uses periph_module_enable (no driver install) for bare-metal UART access
 *   - RISC-V fence instructions for FIFO read and txBuffer coherency on C3/C6/H2
 *   - FreeRTOS task only used for export data processing (not RX)
 *
 * V3.0: ISR-driven RX eliminated batch-read timestamp blindness
 * V3.1: TX_DONE non-blocking response (~8us constant ISR cost)
 * V3.2: Removed driver-mode fallback (ISR-only), added RXFIFO_TOUT safety net
 * V3.3: TX_DONE race fix (state→clear→FIFO→enable ordering), zero-delay warmup,
 *        state-before-RX-enable in TX_DONE handler, RISC-V txBuffer fence
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
#include "driver/uart.h"        // For uart_param_config, uart_set_pin
#include "rom/ets_sys.h"        // For ets_delay_us()
#include "esp_timer.h"

// Direct hardware access (bare-metal UART)
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

// FreeRTOS includes (for task mode)
#if RS485_USE_TASK
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

// ============================================================================
// UART HARDWARE MAPPING
// ============================================================================

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

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

// TX timing uses config defines directly:
//   RS485_TX_WARMUP_DELAY_US        (manual DE warmup before TX)
//   RS485_TX_WARMUP_AUTO_DELAY_US   (auto-direction warmup, typically 0)
// NOTE: Cooldown delays are NOT used — TX_DONE ISR releases bus immediately

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
    // Skip other slave's response (skipRemaining = length+2 covers checksum)
    RX_SKIP_LENGTH,
    RX_SKIP_DATA,
    // TX state
    TX_WAITING_DONE         // FIFO loaded, waiting for TX_DONE interrupt
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

// Static TX frame buffer for non-blocking TX.
// Safe as static because the slave is half-duplex — only one response in flight.
// Avoids allocating 132 bytes on the ISR stack.
static uint8_t DRAM_ATTR txFrameBuf[RS485_TX_BUFFER_SIZE + 4];

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
static volatile uint16_t skipRemaining = 0;

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
static uint32_t statTxDrops = 0;           // Task context only — NOT volatile
static volatile uint32_t statExportOverflows = 0;  // Export buffer overflow events (ISR)

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
// DE PIN CONTROL (ISR-safe, direct register access)
// ============================================================================

#if RS485_DE_PIN >= 0
static inline void IRAM_ATTR setDE_ISR(bool high) {
    gpio_ll_set_level(&GPIO, (gpio_num_t)RS485_DE_PIN, high ? 1 : 0);
}
#else
#define setDE_ISR(x)
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
        statExportBytes = statExportBytes + 1;

        // Feed byte-by-byte to CockpitOS DCS-BIOS parser
        processDcsBiosExportByte(c);
    }
}


// ############################################################################
//
//    ISR IMPLEMENTATION (bare-metal UART)
//
// ############################################################################

// ============================================================================
// SEND RESPONSE — NON-BLOCKING (called from ISR when polled)
// ============================================================================
// V3.3 Pattern: DE assert → disableRx → build frame → state → clear → FIFO → enable → return
//
// When TX_DONE fires: flush echo → DE release → state → enableRx → back to RX_WAIT
//
// DE is asserted FIRST — the frame build loop (~0.5-15us) provides natural
// settling time, eliminating the old ets_delay_us() blocking delay.
// TX_DONE fires when the last bit has physically left the shift register.
// CRITICAL: state → clear → FIFO → enable ordering prevents TX_DONE event loss
// (same fix as master v2.4 — see detailed comment inside function).

static void IRAM_ATTR sendResponseISR() {
    uint16_t toSend = txCount_val;
    if (toSend > 253) toSend = 253;

    // Pre-DE delay: match AVR slave tx_delay_byte() bus-silent gap before responding.
    // AVR slave burns ~40us (one phantom byte) with DE LOW before first real byte.
    #if RS485_TX_PRE_DE_DELAY_US > 0
    ets_delay_us(RS485_TX_PRE_DE_DELAY_US);
    #endif

    // Assert DE — transceiver starts settling into TX mode immediately.
    // The frame build loop below takes ~0.5-15us at 240MHz, which is more than
    // enough settling time for any RS485 transceiver (MAX485 = 200ns typical).
    #if RS485_DE_PIN >= 0
    setDE_ISR(true);
    #endif

    // Disable RX interrupts during TX to prevent echo bytes triggering ISR
    uart_ll_disable_intr_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);

    // Build complete response in static buffer (not stack — saves ~132 bytes ISR stack)
    // NOTE: This loop provides natural settling time for the DE pin — no delay needed
    uint8_t txLen = 0;

    if (toSend == 0) {
        txFrameBuf[txLen++] = 0x00;
    } else {
        uint8_t checksum = (uint8_t)toSend;
        txFrameBuf[txLen++] = (uint8_t)toSend;
        txFrameBuf[txLen++] = MSGTYPE_DCSBIOS;
        checksum ^= MSGTYPE_DCSBIOS;

        // RISC-V fence: ensure task-context writes to txBuffer (from queueCommand)
        // are visible before we read them. portDISABLE_INTERRUPTS on RISC-V
        // does not imply a memory fence. Xtensa RSIL has implicit barrier semantics.
#ifdef __riscv
        __asm__ __volatile__("fence" ::: "memory");
#endif
        for (uint16_t i = 0; i < toSend; i++) {
            uint8_t c = txBuffer[txTail % RS485_TX_BUFFER_SIZE];
            txFrameBuf[txLen++] = c;
            checksum ^= c;
            txTail = txTail + 1;
            txCount_val = txCount_val - 1;
        }

        #if RS485_ARDUINO_COMPAT
        txFrameBuf[txLen++] = CHECKSUM_FIXED;
        #else
        txFrameBuf[txLen++] = checksum;
        #endif

        statCommandsSent = statCommandsSent + 1;
    }

    // CRITICAL ORDERING: state → clear → FIFO → enable
    //
    // Why this order matters (matches master v2.4 fix):
    //   1. Set state FIRST — TX_DONE ISR must see TX_WAITING_DONE before it can fire
    //   2. Clear stale TX_DONE from previous cycle BEFORE loading new data
    //   3. Load FIFO — hardware starts transmitting (40us per byte at 250kbps)
    //   4. Enable TX_DONE interrupt LAST — even if delayed between FIFO load
    //      and enable, the TX_DONE status bit accumulates in int_raw. When we
    //      enable int_ena, int_st = int_raw AND int_ena fires the ISR immediately.
    //
    // The old order (FIFO → clear → enable → state) had a fatal race:
    //   If execution was delayed after FIFO load but before clear, the UART
    //   could finish TX, setting int_raw TX_DONE. Then clear would ERASE that
    //   pending event, and enable would find nothing. State stuck forever.
    state = SlaveState::TX_WAITING_DONE;
    uart_ll_clr_intsts_mask(uartHw, UART_INTR_TX_DONE);

    // Load TX FIFO in one burst — hardware shifts bytes out autonomously
    uart_ll_write_txfifo(uartHw, txFrameBuf, txLen);

    // Enable TX_DONE AFTER FIFO is loaded — cannot miss the event
    uart_ll_ena_intr_mask(uartHw, UART_INTR_TX_DONE);
}

// ============================================================================
// UART RX INTERRUPT HANDLER
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

        // Disable TX_DONE interrupt — one-shot, don't need it until next response
        uart_ll_disable_intr_mask(uartHw, UART_INTR_TX_DONE);
        uart_ll_clr_intsts_mask(uartHw, UART_INTR_TX_DONE);

        // Set state BEFORE re-enabling RX — ensures any incoming byte
        // processed by the RX loop below sees the correct state
        state = SlaveState::RX_WAIT_ADDRESS;

        // Re-enable RX interrupts — receiver is active again
        uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
        uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
    }

    // ================================================================
    // RXFIFO_FULL / RXFIFO_TOUT: Incoming byte(s) — process state machine
    // ================================================================
    // Timestamp once per ISR entry — not per byte. At 250kbps, bytes are
    // 40us apart; the difference within a single drain is negligible for
    // sync detection. Saves ~60ns/byte on Xtensa, ~150ns/byte on RISC-V.
    int64_t now = esp_timer_get_time();

    while (uart_ll_get_rxfifo_len(uartHw) > 0) {
        uint8_t c;
        uart_ll_read_rxfifo(uartHw, &c, 1);
#ifdef __riscv
        // RISC-V (C3, C6, H2): memory fence ensures FIFO read pointer update
        // propagates before next FIFO length check. Without this, the CPU can
        // read faster than the pointer updates, causing stale/duplicate reads.
        __asm__ __volatile__("fence");
#endif

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
                        statBroadcasts = statBroadcasts + 1;
                        state = SlaveState::RX_WAIT_ADDRESS;
                    }
                    else if (packetAddr == RS485_SLAVE_ADDRESS) {
                        // Poll for us — load FIFO and arm TX_DONE, return immediately
                        if (packetMsgType == MSGTYPE_DCSBIOS) {
                            statPolls = statPolls + 1;
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
                        statExportOverflows++;
                        state = SlaveState::RX_SYNC;
                        lastRxTime = now;
                        exportReadPos = 0;
                        exportWritePos = 0;
                        goto done_byte;  // Skip remaining processing for this byte
                    }
                    exportBuffer[exportWritePos] = c;
                    exportWritePos = (exportWritePos + 1) % RS485_EXPORT_BUFFER_SIZE;
                }
                packetDataIdx = packetDataIdx + 1;

                if (packetDataIdx >= packetLength) {
                    state = SlaveState::RX_WAIT_CHECKSUM;
                }
                break;

            case SlaveState::RX_WAIT_CHECKSUM:
                // Checksum intentionally ignored (AVR behavior, checksum is not validated in protocol)

                // Handle completed packet
                if (packetAddr == ADDR_BROADCAST) {
                    statBroadcasts = statBroadcasts + 1;
                    state = SlaveState::RX_WAIT_ADDRESS;
                }
                else if (packetAddr == RS485_SLAVE_ADDRESS) {
                    if (packetMsgType == MSGTYPE_DCSBIOS) {
                        statPolls = statPolls + 1;
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
                // skipRemaining = length + 2 (MsgType + data + checksum),
                // so the checksum byte is consumed here — no separate
                // RX_SKIP_CHECKSUM state needed.
                skipRemaining = skipRemaining - 1;
                if (skipRemaining == 0) {
                    state = SlaveState::RX_WAIT_ADDRESS;
                }
                break;

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
// HARDWARE INITIALIZATION (bare-metal UART + ISR)
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
    uart_config_t uart_config = {};
    uart_config.baud_rate = RS485_BAUD;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;
    uart_config.source_clk = UART_SCLK_DEFAULT;

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
    // Safety net: RXFIFO_TOUT catches bytes that arrive between FIFO drain
    // and ISR return (RISC-V FIFO pointer race). 10 bit-periods = ~400us at
    // 250kbps — well below the 500us sync timeout.
    uart_ll_set_rx_tout(uartHw, 10);
    debugPrintln("[RS485S]   [5] RX FIFO threshold=1, TOUT=10 OK");

    debugPrintln("[RS485S]   [6] Clearing and enabling interrupts...");
    uart_ll_clr_intsts_mask(uartHw, UART_LL_INTR_MASK);
    uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
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
// SLAVE LOOP (export data processing + status printing)
// ============================================================================
// RX and TX are handled by the ISR.
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

        debugPrintf("[RS485S] Polls=%lu Bcasts=%lu Export=%lu Cmds=%lu TxPend=%d TxDrop=%lu ExOvf=%lu\n",
                    statPolls, statBroadcasts, statExportBytes, statCommandsSent, txCount(), statTxDrops, statExportOverflows);

        if (lastPollMs > 0) {
            debugPrintf("[RS485S] Last poll: %lu ms ago\n", millis() - lastPollMs);
        }
    }
    #endif
}



// ############################################################################
//
//    PUBLIC API
//
// ############################################################################

// ============================================================================
// INITIALIZATION
// ============================================================================

bool RS485Slave_init() {
    if (initialized) return true;

    // Initialize state
    txHead = 0;
    txTail = 0;
    txCount_val = 0;
    exportWritePos = 0;
    exportReadPos = 0;
    state = SlaveState::RX_SYNC;
    lastRxTime = esp_timer_get_time();

    // Initialize hardware
    bool hwOk = initRS485Hardware_ISR();

    if (!hwOk) {
        debugPrintln("[RS485S] ERROR: Hardware initialization failed!");
        return false;
    }

    initialized = true;

    debugPrintln("[RS485S] ======================================================");
    debugPrintf("[RS485S] SLAVE INITIALIZED (ISR Mode, bare-metal UART)\n");
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
    debugPrintln("[RS485S]   RX: ISR-driven (FIFO threshold=1 + RXFIFO_TOUT safety net)");
    debugPrintln("[RS485S]   TX: Non-blocking (FIFO burst + TX_DONE interrupt)");

    // Create FreeRTOS task if enabled
    // Task only drains export buffer (RX/TX handled by ISR)
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
        statTxDrops++;
        #if RS485_DEBUG_VERBOSE
        debugPrintf("[RS485S] TX buffer full, dropping: %s %s\n", label, value);
        #endif
        return false;
    }

    // Add to ring buffer (interrupt-safe via critical section)
    portDISABLE_INTERRUPTS();

    for (size_t i = 0; i < labelLen; i++) {
        txBuffer[txHead % RS485_TX_BUFFER_SIZE] = label[i];
        txHead = txHead + 1;
    }
    txBuffer[txHead % RS485_TX_BUFFER_SIZE] = ' ';
    txHead = txHead + 1;
    for (size_t i = 0; i < valueLen; i++) {
        txBuffer[txHead % RS485_TX_BUFFER_SIZE] = value[i];
        txHead = txHead + 1;
    }
    txBuffer[txHead % RS485_TX_BUFFER_SIZE] = '\n';
    txHead = txHead + 1;
    txCount_val = txCount_val + needed;

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
    debugPrintln("[RS485S] Mode: ISR (bare-metal UART)");
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
        // Free ISR and disable UART interrupts
        if (uartIntrHandle) {
            esp_intr_free(uartIntrHandle);
            uartIntrHandle = nullptr;
        }
        uart_ll_disable_intr_mask(uartHw, UART_LL_INTR_MASK);
        // NOTE: We don't call periph_module_disable() here because
        // other code might still need the UART peripheral
    }

    initialized = false;
    debugPrintln("[RS485S] Stopped");
}

#endif // RS485_SLAVE_ENABLED
