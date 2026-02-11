/**
 * =============================================================================
 * ESP32 RS485 SLAVE v2 - CockpitOS Reference Implementation
 * =============================================================================
 *
 * Based on the battle-tested CockpitOS RS485Slave.cpp — the source of truth
 * for all RS485 slave logic. This standalone sketch contains the identical
 * state machine, TX paths, timing, and overflow protection.
 *
 * Two operating modes selected at compile time:
 *
 *   USE_ISR_MODE 1 (default):
 *     - UART RX interrupt fires immediately when byte arrives
 *     - State machine runs IN the ISR — no polling latency
 *     - Response sent immediately from ISR when poll detected
 *     - Uses periph_module_enable (no driver install) for bare-metal UART
 *     - RISC-V fence instruction for FIFO read stability on C3/C6/H2
 *
 *   USE_ISR_MODE 0 (fallback):
 *     - Uses ESP-IDF UART driver for portable RX/TX
 *     - State machine runs in main loop (tight polling)
 *     - Slightly higher latency but works on any ESP32 variant
 *     - Use this if ISR mode doesn't work on your specific chip
 *
 * Both modes work on: ESP32, S2, S3, C3, C6, H2
 *
 * v2 IMPROVEMENTS OVER v1:
 *   - TX path uses CockpitOS pattern: disableRx → DE → warmup → TX → idle →
 *     cooldown → flush → DE release → enableRx
 *   - Proper per-mode timing: manual DE vs auto-direction warmup/cooldown
 *   - FIFO flush after TX to discard echo bytes (ISR mode uses uart_ll_rxfifo_rst)
 *   - TX buffer overflow protection (drops instead of wrapping)
 *   - Combined sendResponse handles both data and zero-length in ISR mode
 *   - Arduino-compatible 0x72 checksum (matches DCS-BIOS protocol)
 *
 * =============================================================================
 * LICENSE: Same as DCS-BIOS Arduino Library
 * =============================================================================
 */

// ============================================================================
// TEST I/O PINS — Your switches, buttons, LEDs
// ============================================================================
// Set any pin to -1 to disable

#define SWITCH_PIN              1       // 2-position switch
#define BUTTON_PIN              0       // Momentary button
#define MC_READY_LED_PIN        2       // LED for MC_READY indicator

// ============================================================================
// USER CONFIGURATION
// ============================================================================

#define SLAVE_ADDRESS           1       // Unique address (1-126)

// Pin Configuration
#define RS485_TX_PIN            17      // ESP32 TX → RS485 board DI
#define RS485_RX_PIN            18      // ESP32 RX ← RS485 board RO
#define RS485_DE_PIN            21      // DE pin GPIO, or -1 for auto-direction

// UART Configuration
#define RS485_UART_NUM          1
#define RS485_BAUD_RATE         250000

// Operating Mode
// 1 = ISR-driven (lowest latency, recommended)
// 0 = Driver-based (portable fallback)
#define USE_ISR_MODE            1

// TX Timing (microseconds)
// Manual DE: warmup after DE assert, cooldown after TX before DE release
// Auto-direction: warmup before TX, cooldown after TX before FIFO flush
// Slave warmup is critical — hot RX→TX turnaround after receiving poll.
// 20µs = 80x safety margin above MAX485 turnaround spec (~250ns).
#define TX_WARMUP_DELAY_MANUAL_US   40
#define TX_COOLDOWN_DELAY_MANUAL_US 40
#define TX_WARMUP_DELAY_AUTO_US     0
#define TX_COOLDOWN_DELAY_AUTO_US   15

// Sync Detection
#define SYNC_TIMEOUT_US         500     // 500µs silence = sync detected

// RX Timeout (driver mode only — max time to wait for packet completion)
#define RX_TIMEOUT_US           5000    // 5ms — matches CockpitOS RS485SlaveConfig.h

// Buffer Sizes
#define TX_BUFFER_SIZE          128     // Ring buffer for outgoing input commands
#define EXPORT_BUFFER_SIZE      512     // Ring buffer for broadcast export data

// Protocol
#define ARDUINO_COMPAT          1       // 1 = fixed 0x72 checksum (Arduino master compat)
                                        // 0 = proper XOR checksum

// Debug Options
#define SERIAL_DEBUG_ENABLE     0       // 1 = Serial debug output
#define UDP_DEBUG_ENABLE        0       // 1 = WiFi UDP debug output
#define WIFI_SSID               "TestNetwork"
#define WIFI_PASSWORD           "TestingOnly"

// FreeRTOS Task (matches CockpitOS RS485_USE_TASK)
// ISR mode:    Task drains export buffer to DCS-BIOS parser on guaranteed schedule
// Driver mode: Task runs full slave loop (RX + TX + export)
#define USE_RS485_TASK          1       // 1 = FreeRTOS task (recommended). 0 = main loop only
#define RS485_TASK_PRIORITY     5       // Higher than default loop
#define RS485_TASK_STACK_SIZE   4096
#define RS485_TASK_TICK_INTERVAL 1      // 1ms tick
#define RS485_TASK_CORE         0       // Core 0 (dual-core chips). Ignored on single-core.

// Status Interval
#define STATUS_INTERVAL_MS      5000    // 0 = disabled

// ============================================================================
// INCLUDES
// ============================================================================

#include <Arduino.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <soc/soc_caps.h>
#include <esp_timer.h>

#if USE_ISR_MODE
#include <hal/uart_ll.h>
#include <hal/gpio_ll.h>
#include <soc/uart_struct.h>
#include <soc/gpio_struct.h>
#include <soc/uart_periph.h>
#include <esp_intr_alloc.h>
#include <esp_rom_gpio.h>
#if __has_include(<esp_private/periph_ctrl.h>)
#include <esp_private/periph_ctrl.h>
#else
#include <driver/periph_ctrl.h>
#endif
#include <soc/periph_defs.h>
#endif // USE_ISR_MODE

// ============================================================================
// VALIDATION
// ============================================================================

#if SLAVE_ADDRESS < 1 || SLAVE_ADDRESS > 126
#error "SLAVE_ADDRESS must be between 1 and 126"
#endif

// ============================================================================
// UART HARDWARE MAPPING (ISR mode)
// ============================================================================

#if USE_ISR_MODE
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
#endif // USE_ISR_MODE

// ============================================================================
// UDP DEBUG
// ============================================================================

#if UDP_DEBUG_ENABLE
#include <WiFi.h>
#include <WiFiUdp.h>

static WiFiUDP udpDbg;
static IPAddress udpDbgTarget(255, 255, 255, 255);
static bool udpDbgConnected = false;
static char udpDbgBuf[256];

void udpDbgInit() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void udpDbgCheck() {
    if (!udpDbgConnected && WiFi.status() == WL_CONNECTED) {
        udpDbgConnected = true;
        udpDbgSend("=== SLAVE %d ONLINE (%s) === IP=%s",
                    SLAVE_ADDRESS,
                    USE_ISR_MODE ? "ISR MODE" : "DRIVER MODE",
                    WiFi.localIP().toString().c_str());
    }
}

void udpDbgSend(const char* fmt, ...) {
    if (!udpDbgConnected) return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(udpDbgBuf, sizeof(udpDbgBuf), fmt, args);
    va_end(args);
    udpDbg.beginPacket(udpDbgTarget, 4210);
    udpDbg.write((uint8_t*)udpDbgBuf, strlen(udpDbgBuf));
    udpDbg.endPacket();
}
#else
#define udpDbgInit()
#define udpDbgCheck()
#define udpDbgSend(...)
#endif

// ============================================================================
// SERIAL DEBUG MACROS
// ============================================================================

#if SERIAL_DEBUG_ENABLE
#define dbgBegin(baud)          Serial.begin(baud)
#define dbgPrint(msg)           Serial.println(msg)
#define dbgPrintf(fmt, ...)     Serial.printf(fmt, ##__VA_ARGS__)
#define dbgFlush()              Serial.flush()
#else
#define dbgBegin(baud)
#define dbgPrint(msg)
#define dbgPrintf(fmt, ...)
#define dbgFlush()
#endif

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================

static constexpr uint8_t ADDR_BROADCAST = 0;
static constexpr uint8_t MSGTYPE_DCSBIOS = 0;
static constexpr uint8_t CHECKSUM_FIXED = 0x72;

// ============================================================================
// DCS-BIOS PROTOCOL PARSER (for export data — runs in main loop)
// ============================================================================

#define DCSBIOS_STATE_WAIT_FOR_SYNC  0
#define DCSBIOS_STATE_ADDRESS_LOW    1
#define DCSBIOS_STATE_ADDRESS_HIGH   2
#define DCSBIOS_STATE_COUNT_LOW      3
#define DCSBIOS_STATE_COUNT_HIGH     4
#define DCSBIOS_STATE_DATA_LOW       5
#define DCSBIOS_STATE_DATA_HIGH      6

class ExportStreamListener;
extern ExportStreamListener* firstExportStreamListener;

class ExportStreamListener {
protected:
    uint16_t firstAddressOfInterest;
    uint16_t lastAddressOfInterest;

public:
    ExportStreamListener* nextExportStreamListener;

    ExportStreamListener(uint16_t firstAddr, uint16_t lastAddr) {
        firstAddressOfInterest = firstAddr & ~0x01;
        lastAddressOfInterest = lastAddr & ~0x01;

        if (firstExportStreamListener == nullptr) {
            firstExportStreamListener = this;
            nextExportStreamListener = nullptr;
            return;
        }

        ExportStreamListener** prevNextPtr = &firstExportStreamListener;
        ExportStreamListener* nextESL = firstExportStreamListener;

        while (nextESL &&
               ((nextESL->lastAddressOfInterest < lastAddressOfInterest) ||
                (nextESL->lastAddressOfInterest == lastAddressOfInterest &&
                 nextESL->firstAddressOfInterest < firstAddressOfInterest))) {
            prevNextPtr = &(nextESL->nextExportStreamListener);
            nextESL = nextESL->nextExportStreamListener;
        }

        nextExportStreamListener = nextESL;
        *prevNextPtr = this;
    }

    inline uint16_t getFirstAddressOfInterest() { return firstAddressOfInterest; }
    inline uint16_t getLastAddressOfInterest() { return lastAddressOfInterest; }

    virtual void onDcsBiosWrite(unsigned int address, unsigned int value) {}
    virtual void onConsistentData() {}
    virtual void loop() {}

    static void loopAll() {
        ExportStreamListener* el = firstExportStreamListener;
        while (el) {
            el->loop();
            el = el->nextExportStreamListener;
        }
    }
};

ExportStreamListener* firstExportStreamListener = nullptr;

class IntegerBuffer : public ExportStreamListener {
private:
    volatile uint16_t data;
    uint16_t mask;
    uint8_t shift;
    void (*callback)(unsigned int);
    volatile bool dirty;
    volatile bool receivedData;

public:
    IntegerBuffer(uint16_t address, uint16_t mask, uint8_t shift, void (*callback)(unsigned int))
        : ExportStreamListener(address, address), mask(mask), shift(shift),
          callback(callback), data(0), dirty(false), receivedData(false) {}

    virtual void onDcsBiosWrite(unsigned int address, unsigned int value) override {
        uint16_t newValue = (value & mask) >> shift;
        if (!receivedData || data != newValue) {
            data = newValue;
            dirty = true;
            receivedData = true;
        }
    }

    virtual void loop() override {
        if (dirty && callback) {
            dirty = false;
            callback(data);
        }
    }

    uint16_t getData() {
        dirty = false;
        return data;
    }

    bool hasUpdatedData() { return dirty; }
};

template<unsigned int LENGTH>
class StringBuffer : public ExportStreamListener {
private:
    char receivingBuffer[LENGTH + 1];
    char userBuffer[LENGTH + 1];
    volatile bool receivingDirty;
    bool userDirty;
    void (*callback)(char*);

    void setChar(unsigned int index, unsigned char value) {
        if (receivingBuffer[index] == value) return;
        receivingBuffer[index] = value;
        receivingDirty = true;
    }

public:
    StringBuffer(uint16_t address, void (*callback)(char*))
        : ExportStreamListener(address, address + LENGTH), callback(callback),
          receivingDirty(false), userDirty(false) {
        memset(receivingBuffer, ' ', LENGTH);
        receivingBuffer[LENGTH] = '\0';
        userBuffer[LENGTH] = '\0';
    }

    virtual void onDcsBiosWrite(unsigned int address, unsigned int data) override {
        unsigned int index = address - firstAddressOfInterest;
        setChar(index, ((char*)&data)[0]);
        if (LENGTH > index + 1) {
            setChar(index + 1, ((char*)&data)[1]);
        }
    }

    virtual void onConsistentData() override {
        if (receivingDirty) {
            memcpy(userBuffer, receivingBuffer, LENGTH);
            receivingDirty = false;
            userDirty = true;
        }
    }

    virtual void loop() override {
        if (userDirty && callback) {
            userDirty = false;
            callback(userBuffer);
        }
    }

    char* getData() {
        userDirty = false;
        return userBuffer;
    }

    bool hasUpdatedData() { return userDirty; }
};

class ProtocolParser {
private:
    uint8_t state;
    uint16_t address;
    uint16_t count;
    uint16_t data;
    uint8_t sync_byte_count;
    ExportStreamListener* startESL;

public:
    ProtocolParser() : state(DCSBIOS_STATE_WAIT_FOR_SYNC), sync_byte_count(0), startESL(nullptr) {}

    void processChar(uint8_t c) {
        switch (state) {
            case DCSBIOS_STATE_WAIT_FOR_SYNC:
                break;
            case DCSBIOS_STATE_ADDRESS_LOW:
                address = c;
                state = DCSBIOS_STATE_ADDRESS_HIGH;
                break;
            case DCSBIOS_STATE_ADDRESS_HIGH:
                address |= (c << 8);
                state = (address != 0x5555) ? DCSBIOS_STATE_COUNT_LOW : DCSBIOS_STATE_WAIT_FOR_SYNC;
                break;
            case DCSBIOS_STATE_COUNT_LOW:
                count = c;
                state = DCSBIOS_STATE_COUNT_HIGH;
                break;
            case DCSBIOS_STATE_COUNT_HIGH:
                count |= (c << 8);
                state = DCSBIOS_STATE_DATA_LOW;
                break;
            case DCSBIOS_STATE_DATA_LOW:
                data = c;
                count--;
                state = DCSBIOS_STATE_DATA_HIGH;
                break;
            case DCSBIOS_STATE_DATA_HIGH:
                data |= (c << 8);
                count--;

                while (startESL && startESL->getLastAddressOfInterest() < address) {
                    startESL->onConsistentData();
                    startESL = startESL->nextExportStreamListener;
                }

                if (startESL) {
                    ExportStreamListener* el = startESL;
                    while (el) {
                        if (el->getFirstAddressOfInterest() > address) break;
                        if (el->getFirstAddressOfInterest() <= address &&
                            el->getLastAddressOfInterest() >= address) {
                            el->onDcsBiosWrite(address, data);
                        }
                        el = el->nextExportStreamListener;
                    }
                }

                address += 2;
                state = (count == 0) ? DCSBIOS_STATE_ADDRESS_LOW : DCSBIOS_STATE_DATA_LOW;
                break;
        }

        if (c == 0x55) {
            sync_byte_count++;
        } else {
            sync_byte_count = 0;
        }

        if (sync_byte_count == 4) {
            state = DCSBIOS_STATE_ADDRESS_LOW;
            sync_byte_count = 0;
            startESL = firstExportStreamListener;
        }
    }
};

// ============================================================================
// INPUT POLLING SYSTEM
// ============================================================================

static volatile bool messageSentOrQueued = false;

class PollingInput {
private:
    virtual void resetState() = 0;
    virtual void pollInput() = 0;
    unsigned long pollingIntervalMs;
    unsigned long lastPollTime;
    PollingInput* nextPollingInput;

public:
    static PollingInput* firstPollingInput;

    static void setMessageSentOrQueued() { messageSentOrQueued = true; }

    PollingInput(unsigned long pollIntervalMs = 0)
        : pollingIntervalMs(pollIntervalMs), lastPollTime(0) {
        nextPollingInput = firstPollingInput;
        firstPollingInput = this;
    }

    static void pollInputs() {
        PollingInput* pi = firstPollingInput;
        if (!pi) return;

        unsigned long now = millis();
        PollingInput* lastSender = nullptr;

        do {
            messageSentOrQueued = false;

            if (now - pi->lastPollTime >= pi->pollingIntervalMs) {
                pi->pollInput();
                if (messageSentOrQueued) {
                    lastSender = pi;
                }
                pi->lastPollTime = now;
            }

            if (pi->nextPollingInput == nullptr) {
                pi->nextPollingInput = firstPollingInput;
            }

            pi = pi->nextPollingInput;
        } while (pi != firstPollingInput);

        if (lastSender && (firstPollingInput != pi)) {
            firstPollingInput = lastSender->nextPollingInput;
        }
    }

    static void resetAllStates() {
        PollingInput* pi = firstPollingInput;
        if (!pi) return;

        do {
            pi->resetState();
            pi = pi->nextPollingInput;
        } while (pi != firstPollingInput);
    }
};

PollingInput* PollingInput::firstPollingInput = nullptr;

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================

static ProtocolParser parser;

// TX ring buffer (for queued input commands — interrupt-safe)
static uint8_t txBuffer[TX_BUFFER_SIZE];
static volatile uint16_t txHead = 0;
static volatile uint16_t txTail = 0;
static volatile uint16_t txCountVal = 0;

static inline uint16_t txCount() { return txCountVal; }
static inline bool txEmpty() { return txCountVal == 0; }

// Export data ring buffer (ISR writes, main loop reads)
static volatile uint8_t exportBuffer[EXPORT_BUFFER_SIZE];
static volatile uint16_t exportWritePos = 0;
static volatile uint16_t exportReadPos = 0;

static inline uint16_t IRAM_ATTR exportBufferAvailableForWrite() {
    return (uint16_t)((exportReadPos - exportWritePos - 1 + EXPORT_BUFFER_SIZE) % EXPORT_BUFFER_SIZE);
}

// ============================================================================
// tryToSendDcsBiosMessage — queues input commands for sending when polled
// ============================================================================

bool tryToSendDcsBiosMessage(const char* msg, const char* arg) {
    size_t msgLen = strlen(msg);
    size_t argLen = strlen(arg);
    size_t needed = msgLen + 1 + argLen + 1;  // space + newline

    if (needed > TX_BUFFER_SIZE - txCount()) {
        return false;  // Buffer full
    }

    // Add to ring buffer (interrupt-safe via critical section)
    portDISABLE_INTERRUPTS();

    for (size_t i = 0; i < msgLen; i++) {
        txBuffer[txHead % TX_BUFFER_SIZE] = msg[i];
        txHead++;
    }
    txBuffer[txHead % TX_BUFFER_SIZE] = ' ';
    txHead++;
    for (size_t i = 0; i < argLen; i++) {
        txBuffer[txHead % TX_BUFFER_SIZE] = arg[i];
        txHead++;
    }
    txBuffer[txHead % TX_BUFFER_SIZE] = '\n';
    txHead++;
    txCountVal += needed;

    portENABLE_INTERRUPTS();

    PollingInput::setMessageSentOrQueued();
    return true;
}

// ============================================================================
// RS485 STATE MACHINE
// ============================================================================

enum class SlaveState : uint8_t {
    RX_SYNC,
    RX_WAIT_ADDRESS,
    RX_WAIT_MSGTYPE,
    RX_WAIT_LENGTH,
    RX_WAIT_DATA,
    RX_WAIT_CHECKSUM,
    // Skip other slave's response
    RX_SKIP_LENGTH,
    RX_SKIP_DATA,
    RX_SKIP_CHECKSUM
};

enum RxDataType : uint8_t {
    RXDATA_IGNORE,
    RXDATA_DCSBIOS_EXPORT
};

// All volatile for ISR access
static volatile SlaveState rs485State = SlaveState::RX_SYNC;
static volatile int64_t lastRxTime = 0;
static volatile uint8_t packetAddr = 0;
static volatile uint8_t packetMsgType = 0;
static volatile uint8_t packetLength = 0;
static volatile uint16_t packetDataIdx = 0;
static volatile uint8_t skipRemaining = 0;
static volatile RxDataType rxDataType = RXDATA_IGNORE;

// Driver mode: RX timeout tracking
static uint32_t rxStartUs = 0;

// Statistics
static volatile uint32_t statPolls = 0;
static volatile uint32_t statBroadcasts = 0;
static volatile uint32_t statExportBytes = 0;
static volatile uint32_t statCommandsSent = 0;
static volatile uint32_t lastPollMs = 0;

// ============================================================================
// DE PIN CONTROL
// ============================================================================

#if RS485_DE_PIN >= 0
static inline void setDE(bool high) {
    gpio_set_level((gpio_num_t)RS485_DE_PIN, high ? 1 : 0);
}
#if USE_ISR_MODE
static inline void IRAM_ATTR setDE_ISR(bool high) {
    gpio_ll_set_level(&GPIO, (gpio_num_t)RS485_DE_PIN, high ? 1 : 0);
}
#endif
#else
#define setDE(x)
#if USE_ISR_MODE
#define setDE_ISR(x)
#endif
#endif

// ============================================================================
// PROCESS EXPORT DATA (drain ring buffer to protocol parser)
// ============================================================================

static void processExportData() {
    while (exportReadPos != exportWritePos) {
        uint8_t c = exportBuffer[exportReadPos];
        exportReadPos = (exportReadPos + 1) % EXPORT_BUFFER_SIZE;
        statExportBytes++;
        parser.processChar(c);
    }
}

// ============================================================================
// FREERTOS RS485 TASK (matches CockpitOS RS485_USE_TASK pattern)
// ============================================================================
// ISR mode:    Task only drains export buffer (RX/TX handled by ISR)
// Driver mode: Task runs full slave loop (RX + TX + export)

#if USE_RS485_TASK

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static TaskHandle_t rs485TaskHandle = nullptr;
static volatile bool rs485TaskRunning = false;

// Forward declaration — rs485SlaveLoop defined after ISR/driver mode sections
static void rs485SlaveLoop();

static void rs485SlaveTask(void* param) {
    (void)param;
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (rs485TaskRunning) {
        rs485SlaveLoop();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(RS485_TASK_TICK_INTERVAL));
    }

    vTaskDelete(nullptr);
}

static void startRS485Task() {
    rs485TaskRunning = true;

#if CONFIG_FREERTOS_UNICORE
    xTaskCreate(rs485SlaveTask, "RS485S", RS485_TASK_STACK_SIZE, nullptr,
                RS485_TASK_PRIORITY, &rs485TaskHandle);
#else
    xTaskCreatePinnedToCore(rs485SlaveTask, "RS485S", RS485_TASK_STACK_SIZE, nullptr,
                            RS485_TASK_PRIORITY, &rs485TaskHandle, RS485_TASK_CORE);
#endif

    dbgPrintf("  RS485 task started: priority=%d, stack=%d\n",
              RS485_TASK_PRIORITY, RS485_TASK_STACK_SIZE);
}

#endif // USE_RS485_TASK

// ############################################################################
//
//    ISR MODE IMPLEMENTATION
//
// ############################################################################

#if USE_ISR_MODE

// ============================================================================
// ISR-MODE: SEND RESPONSE (from ISR when polled)
// ============================================================================
// Pattern: disableRx → DE assert → warmup → TX → txIdle → cooldown → flush → DE release → enableRx

static void IRAM_ATTR sendResponseISR() {
    uint16_t toSend = txCountVal;
    if (toSend > 253) toSend = 253;

    // Disable RX interrupt during TX
    uart_ll_disable_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);

    // Warmup
    #if RS485_DE_PIN >= 0
    setDE_ISR(true);
    #if TX_WARMUP_DELAY_MANUAL_US > 0
    ets_delay_us(TX_WARMUP_DELAY_MANUAL_US);
    #endif
    #else
    #if TX_WARMUP_DELAY_AUTO_US > 0
    ets_delay_us(TX_WARMUP_DELAY_AUTO_US);
    #endif
    #endif

    // Build response in local buffer
    uint8_t txBuf[TX_BUFFER_SIZE + 4];
    uint8_t txLen = 0;

    if (toSend == 0) {
        txBuf[txLen++] = 0x00;
    } else {
        uint8_t checksum = (uint8_t)toSend;
        txBuf[txLen++] = (uint8_t)toSend;
        txBuf[txLen++] = MSGTYPE_DCSBIOS;
        checksum ^= MSGTYPE_DCSBIOS;

        for (uint16_t i = 0; i < toSend; i++) {
            uint8_t c = txBuffer[txTail % TX_BUFFER_SIZE];
            txBuf[txLen++] = c;
            checksum ^= c;
            txTail++;
            txCountVal--;
        }

        #if ARDUINO_COMPAT
        txBuf[txLen++] = CHECKSUM_FIXED;
        #else
        txBuf[txLen++] = checksum;
        #endif

        statCommandsSent++;
    }

    // TX
    uart_ll_write_txfifo(uartHw, txBuf, txLen);
    while (!uart_ll_is_tx_idle(uartHw));

    // Cooldown + flush
    #if RS485_DE_PIN >= 0
    #if TX_COOLDOWN_DELAY_MANUAL_US > 0
    ets_delay_us(TX_COOLDOWN_DELAY_MANUAL_US);
    #endif
    uart_ll_rxfifo_rst(uartHw);
    setDE_ISR(false);
    #else
    #if TX_COOLDOWN_DELAY_AUTO_US > 0
    ets_delay_us(TX_COOLDOWN_DELAY_AUTO_US);
    #endif
    uart_ll_rxfifo_rst(uartHw);
    #endif

    // Re-enable RX interrupt
    uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL);
    uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);

    rs485State = SlaveState::RX_WAIT_ADDRESS;
}

// ============================================================================
// ISR-MODE: HANDLE COMPLETED PACKET
// ============================================================================

static inline void IRAM_ATTR handlePacketCompleteISR(int64_t now) {
    if (packetAddr == ADDR_BROADCAST) {
        statBroadcasts++;
        rs485State = SlaveState::RX_WAIT_ADDRESS;
    }
    else if (packetAddr == SLAVE_ADDRESS) {
        if (packetMsgType == MSGTYPE_DCSBIOS) {
            statPolls++;
            lastPollMs = (uint32_t)(now / 1000);
            sendResponseISR();
        } else {
            rs485State = SlaveState::RX_SYNC;
        }
    }
    else {
        rs485State = SlaveState::RX_SKIP_LENGTH;
    }
}

// ============================================================================
// ISR-MODE: UART RX INTERRUPT HANDLER
// ============================================================================

static void IRAM_ATTR uart_isr_handler(void* arg) {
    uint32_t uart_intr_status = uart_ll_get_intsts_mask(uartHw);

    while (uart_ll_get_rxfifo_len(uartHw) > 0) {
        uint8_t c;
        uart_ll_read_rxfifo(uartHw, &c, 1);
#ifdef __riscv
        __asm__ __volatile__("fence" ::: "memory");
#endif

        int64_t now = esp_timer_get_time();

        // Sync detection
        if (rs485State == SlaveState::RX_SYNC) {
            if ((now - lastRxTime) >= SYNC_TIMEOUT_US) {
                rs485State = SlaveState::RX_WAIT_ADDRESS;
            } else {
                lastRxTime = now;
                continue;
            }
        }

        switch (rs485State) {
            case SlaveState::RX_WAIT_ADDRESS:
                packetAddr = c;
                rs485State = SlaveState::RX_WAIT_MSGTYPE;
                break;

            case SlaveState::RX_WAIT_MSGTYPE:
                packetMsgType = c;
                rs485State = SlaveState::RX_WAIT_LENGTH;
                break;

            case SlaveState::RX_WAIT_LENGTH:
                packetLength = c;
                packetDataIdx = 0;

                if (packetLength == 0) {
                    handlePacketCompleteISR(now);
                } else {
                    if (packetAddr == ADDR_BROADCAST && packetMsgType == MSGTYPE_DCSBIOS) {
                        rxDataType = RXDATA_DCSBIOS_EXPORT;
                    } else {
                        rxDataType = RXDATA_IGNORE;
                    }
                    rs485State = SlaveState::RX_WAIT_DATA;
                }
                break;

            case SlaveState::RX_WAIT_DATA:
                if (rxDataType == RXDATA_DCSBIOS_EXPORT) {
                    if (exportBufferAvailableForWrite() == 0) {
                        // Overflow — force re-sync (AVR behavior)
                        rs485State = SlaveState::RX_SYNC;
                        lastRxTime = now;
                        exportReadPos = 0;
                        exportWritePos = 0;
                        goto done_byte;
                    }
                    exportBuffer[exportWritePos] = c;
                    exportWritePos = (exportWritePos + 1) % EXPORT_BUFFER_SIZE;
                }
                packetDataIdx++;
                if (packetDataIdx >= packetLength) {
                    rs485State = SlaveState::RX_WAIT_CHECKSUM;
                }
                break;

            case SlaveState::RX_WAIT_CHECKSUM:
                handlePacketCompleteISR(now);
                break;

            // Skip another slave's response
            case SlaveState::RX_SKIP_LENGTH:
                if (c == 0x00) {
                    rs485State = SlaveState::RX_WAIT_ADDRESS;
                } else {
                    skipRemaining = c + 2;
                    rs485State = SlaveState::RX_SKIP_DATA;
                }
                break;

            case SlaveState::RX_SKIP_DATA:
                skipRemaining--;
                if (skipRemaining == 0) {
                    rs485State = SlaveState::RX_WAIT_ADDRESS;
                }
                break;

            case SlaveState::RX_SKIP_CHECKSUM:
                rs485State = SlaveState::RX_WAIT_ADDRESS;
                break;

            case SlaveState::RX_SYNC:
                break;

            default:
                rs485State = SlaveState::RX_SYNC;
                break;
        }

done_byte:
        lastRxTime = now;
    }

    uart_ll_clr_intsts_mask(uartHw, uart_intr_status);
}

// ============================================================================
// ISR-MODE: HARDWARE INITIALIZATION
// ============================================================================

static void initRS485Hardware() {
    dbgPrint("  [1] Configuring DE GPIO pin...");
#if RS485_DE_PIN >= 0
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RS485_DE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    setDE_ISR(false);
    dbgPrint("  [1] DE GPIO configured OK");
#else
    dbgPrint("  [1] No DE pin (auto-direction)");
#endif

    dbgPrint("  [2] Enabling UART peripheral module...");
    dbgFlush();
    periph_module_enable(RS485_PERIPH_MODULE);

    dbgPrint("  [3] Configuring UART parameters...");
    dbgFlush();
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };
    ESP_ERROR_CHECK(uart_param_config((uart_port_t)RS485_UART_NUM, &uart_config));

    dbgPrint("  [4] Setting UART pins...");
    dbgFlush();
    ESP_ERROR_CHECK(uart_set_pin((uart_port_t)RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    gpio_set_pull_mode((gpio_num_t)RS485_RX_PIN, GPIO_PULLUP_ONLY);
    dbgPrint("  [4] UART pins configured OK");

    dbgPrint("  [5] Configuring RX FIFO threshold...");
    uart_ll_set_rxfifo_full_thr(uartHw, 1);
    dbgPrint("  [5] RX FIFO threshold=1 OK");

    dbgPrint("  [6] Clearing and enabling interrupts...");
    uart_ll_clr_intsts_mask(uartHw, UART_LL_INTR_MASK);
    uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);
    dbgPrint("  [6] Interrupts configured OK");

    dbgPrint("  [7] Registering ISR...");
    ESP_ERROR_CHECK(esp_intr_alloc(uart_periph_signal[RS485_UART_NUM].irq,
                                    ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1,
                                    uart_isr_handler, NULL, &uartIntrHandle));
    dbgPrint("  [7] ISR registered OK");

    rs485State = SlaveState::RX_SYNC;
    lastRxTime = esp_timer_get_time();

    dbgPrint("  RS485 ISR mode initialization complete!");
    udpDbgSend("RS485 ISR mode initialized, DE pin=%d", RS485_DE_PIN);
}

// ############################################################################
//
//    DRIVER MODE IMPLEMENTATION (fallback)
//
// ############################################################################

#else // !USE_ISR_MODE

// ============================================================================
// DRIVER-MODE: TX RESPONSE
// ============================================================================

static void sendZeroLengthResponse() {
    uint8_t zero = 0x00;

    #if RS485_DE_PIN >= 0
    setDE(true);
    #if TX_WARMUP_DELAY_MANUAL_US > 0
    ets_delay_us(TX_WARMUP_DELAY_MANUAL_US);
    #endif
    #else
    #if TX_WARMUP_DELAY_AUTO_US > 0
    ets_delay_us(TX_WARMUP_DELAY_AUTO_US);
    #endif
    #endif

    uart_write_bytes((uart_port_t)RS485_UART_NUM, (const char*)&zero, 1);
    uart_wait_tx_done((uart_port_t)RS485_UART_NUM, pdMS_TO_TICKS(10));

    #if RS485_DE_PIN >= 0
    #if TX_COOLDOWN_DELAY_MANUAL_US > 0
    ets_delay_us(TX_COOLDOWN_DELAY_MANUAL_US);
    #endif
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    setDE(false);
    #else
    #if TX_COOLDOWN_DELAY_AUTO_US > 0
    ets_delay_us(TX_COOLDOWN_DELAY_AUTO_US);
    #endif
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    #endif

    rs485State = SlaveState::RX_WAIT_ADDRESS;
}

static void sendResponse() {
    uint16_t toSend = txCount();
    if (toSend == 0) {
        sendZeroLengthResponse();
        return;
    }
    if (toSend > 253) toSend = 253;

    uint8_t txBuf[TX_BUFFER_SIZE + 4];
    uint8_t txLen = 0;
    uint8_t checksum = (uint8_t)toSend;

    txBuf[txLen++] = (uint8_t)toSend;
    txBuf[txLen++] = MSGTYPE_DCSBIOS;
    checksum ^= MSGTYPE_DCSBIOS;

    for (uint16_t i = 0; i < toSend; i++) {
        uint8_t c = txBuffer[txTail % TX_BUFFER_SIZE];
        txBuf[txLen++] = c;
        checksum ^= c;
        txTail++;
        txCountVal--;
    }

    #if ARDUINO_COMPAT
    txBuf[txLen++] = CHECKSUM_FIXED;
    #else
    txBuf[txLen++] = checksum;
    #endif

    statCommandsSent++;

    #if RS485_DE_PIN >= 0
    setDE(true);
    #if TX_WARMUP_DELAY_MANUAL_US > 0
    ets_delay_us(TX_WARMUP_DELAY_MANUAL_US);
    #endif
    #else
    #if TX_WARMUP_DELAY_AUTO_US > 0
    ets_delay_us(TX_WARMUP_DELAY_AUTO_US);
    #endif
    #endif

    uart_write_bytes((uart_port_t)RS485_UART_NUM, (const char*)txBuf, txLen);
    uart_wait_tx_done((uart_port_t)RS485_UART_NUM, pdMS_TO_TICKS(10));

    #if RS485_DE_PIN >= 0
    #if TX_COOLDOWN_DELAY_MANUAL_US > 0
    ets_delay_us(TX_COOLDOWN_DELAY_MANUAL_US);
    #endif
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    setDE(false);
    #else
    #if TX_COOLDOWN_DELAY_AUTO_US > 0
    ets_delay_us(TX_COOLDOWN_DELAY_AUTO_US);
    #endif
    uart_flush_input((uart_port_t)RS485_UART_NUM);
    #endif

    rs485State = SlaveState::RX_WAIT_ADDRESS;
}

// ============================================================================
// DRIVER-MODE: PACKET HANDLING
// ============================================================================

static void handlePacketComplete() {
    if (packetAddr == ADDR_BROADCAST) {
        statBroadcasts++;
        rs485State = SlaveState::RX_WAIT_ADDRESS;
    }
    else if (packetAddr == SLAVE_ADDRESS) {
        if (packetMsgType == MSGTYPE_DCSBIOS) {
            statPolls++;
            lastPollMs = millis();
            sendResponse();
        } else {
            rs485State = SlaveState::RX_SYNC;
        }
    }
    else {
        rs485State = SlaveState::RX_SKIP_LENGTH;
    }
}

// ============================================================================
// DRIVER-MODE: PROCESS RX BYTE
// ============================================================================

static void processRxByte(uint8_t c) {
    int64_t now = esp_timer_get_time();

    if (rs485State == SlaveState::RX_SYNC) {
        if ((now - lastRxTime) >= SYNC_TIMEOUT_US) {
            rs485State = SlaveState::RX_WAIT_ADDRESS;
        } else {
            lastRxTime = now;
            return;
        }
    }

    switch (rs485State) {
        case SlaveState::RX_WAIT_ADDRESS:
            packetAddr = c;
            rxStartUs = micros();  // Start RX timeout tracking (CockpitOS driver mode line 834)
            rs485State = SlaveState::RX_WAIT_MSGTYPE;
            break;

        case SlaveState::RX_WAIT_MSGTYPE:
            packetMsgType = c;
            rs485State = SlaveState::RX_WAIT_LENGTH;
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
                rs485State = SlaveState::RX_WAIT_DATA;
            }
            break;

        case SlaveState::RX_WAIT_DATA:
            if (rxDataType == RXDATA_DCSBIOS_EXPORT) {
                if (exportBufferAvailableForWrite() == 0) {
                    rs485State = SlaveState::RX_SYNC;
                    exportWritePos = 0;
                    exportReadPos = 0;
                    lastRxTime = now;
                    break;
                }
                exportBuffer[exportWritePos] = c;
                exportWritePos = (exportWritePos + 1) % EXPORT_BUFFER_SIZE;
            }
            packetDataIdx++;
            if (packetDataIdx >= packetLength) {
                rs485State = SlaveState::RX_WAIT_CHECKSUM;
            }
            break;

        case SlaveState::RX_WAIT_CHECKSUM:
            handlePacketComplete();
            break;

        case SlaveState::RX_SKIP_LENGTH:
            if (c == 0x00) {
                rs485State = SlaveState::RX_WAIT_ADDRESS;
            } else {
                skipRemaining = c + 2;
                rs485State = SlaveState::RX_SKIP_DATA;
            }
            break;

        case SlaveState::RX_SKIP_DATA:
            skipRemaining--;
            if (skipRemaining == 0) {
                rs485State = SlaveState::RX_WAIT_ADDRESS;
            }
            break;

        case SlaveState::RX_SKIP_CHECKSUM:
            rs485State = SlaveState::RX_WAIT_ADDRESS;
            break;

        case SlaveState::RX_SYNC:
            break;

        default:
            rs485State = SlaveState::RX_SYNC;
            break;
    }

    lastRxTime = now;
}

// ============================================================================
// DRIVER-MODE: HARDWARE INITIALIZATION
// ============================================================================

static void initRS485Hardware() {
    dbgPrint("  [1] Configuring DE GPIO pin...");
#if RS485_DE_PIN >= 0
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RS485_DE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    setDE(false);
    dbgPrint("  [1] DE GPIO configured OK");
#else
    dbgPrint("  [1] No DE pin (auto-direction)");
#endif

    dbgPrint("  [2] Installing UART driver...");

    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install((uart_port_t)RS485_UART_NUM, 256, 256, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config((uart_port_t)RS485_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin((uart_port_t)RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN,
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    gpio_set_pull_mode((gpio_num_t)RS485_RX_PIN, GPIO_PULLUP_ONLY);
    uart_flush_input((uart_port_t)RS485_UART_NUM);

    dbgPrint("  [2] UART driver installed OK");

    rs485State = SlaveState::RX_SYNC;
    lastRxTime = esp_timer_get_time();

    dbgPrint("  RS485 driver mode initialization complete!");
    udpDbgSend("RS485 driver mode, DE pin=%d", RS485_DE_PIN);
}

#endif // USE_ISR_MODE

// ============================================================================
// RS485 SLAVE LOOP — called from FreeRTOS task or Arduino loop()
// ============================================================================
// Matches CockpitOS rs485SlaveLoop() exactly:
//   ISR mode:    Only drains export buffer (RX/TX handled by ISR)
//   Driver mode: Full loop — RX bytes, process, export drain, RX timeout

static void rs485SlaveLoop() {
#if !USE_ISR_MODE
    // Driver mode: read bytes and process through state machine
    {
        uint8_t rxBuf[64];
        int len = uart_read_bytes((uart_port_t)RS485_UART_NUM, rxBuf, sizeof(rxBuf), 0);
        for (int i = 0; i < len; i++) {
            processRxByte(rxBuf[i]);
        }
    }
#endif

    // Process any buffered export data (ISR queues bytes, we drain them here)
    processExportData();

#if !USE_ISR_MODE
    // Driver mode: RX timeout — if stuck mid-packet, force resync
    // Matches CockpitOS rs485SlaveLoop lines 1003-1013
    if (rs485State != SlaveState::RX_WAIT_ADDRESS &&
        rs485State != SlaveState::RX_SYNC) {
        uint32_t elapsed = micros() - rxStartUs;
        if (elapsed > RX_TIMEOUT_US) {
            rs485State = SlaveState::RX_SYNC;
        }
    }
#endif

    // Periodic status
    #if STATUS_INTERVAL_MS > 0
    static uint32_t lastStatusMs = 0;
    if (millis() - lastStatusMs >= STATUS_INTERVAL_MS) {
        lastStatusMs = millis();
        dbgPrintf("[SLAVE] Polls=%lu Bcasts=%lu Export=%lu Cmds=%lu TxPend=%d\n",
                  statPolls, statBroadcasts, statExportBytes, statCommandsSent, txCount());
        if (lastPollMs > 0) {
            dbgPrintf("[SLAVE] Last poll: %lu ms ago\n", millis() - lastPollMs);
        }
        udpDbgSend("Polls=%lu Bcasts=%lu Export=%lu Cmds=%lu",
                    statPolls, statBroadcasts, statExportBytes, statCommandsSent);
    }
    #endif
}

// ============================================================================
// INPUT/OUTPUT CLASSES
// ============================================================================

#define POLL_EVERY_TIME 0

class Switch2Pos : public PollingInput {
private:
    const char* msg;
    uint8_t pin;
    int8_t lastState;
    int8_t debounceSteadyState;
    bool reverse;
    unsigned long debounceDelay;
    unsigned long lastDebounceTime;

    void resetState() override {
        lastState = (lastState == 0) ? -1 : 0;
    }

    void pollInput() override {
        int8_t state = digitalRead(pin);
        if (reverse) state = !state;

        unsigned long now = millis();

        if (state != debounceSteadyState) {
            lastDebounceTime = now;
            debounceSteadyState = state;
        }

        if ((now - lastDebounceTime) >= debounceDelay) {
            if (debounceSteadyState != lastState) {
                if (tryToSendDcsBiosMessage(msg, state == HIGH ? "0" : "1")) {
                    lastState = debounceSteadyState;
                }
            }
        }
    }

public:
    Switch2Pos(const char* msg, uint8_t pin, bool reverse = false, unsigned long debounceDelay = 5)
        : PollingInput(POLL_EVERY_TIME), msg(msg), pin(pin), reverse(reverse),
          debounceDelay(debounceDelay), lastDebounceTime(0) {
        pinMode(pin, INPUT_PULLUP);
        lastState = digitalRead(pin);
        if (reverse) lastState = !lastState;
        debounceSteadyState = lastState;
    }
};

class LED : public IntegerBuffer {
private:
    uint8_t pin;

public:
    LED(uint16_t address, uint16_t mask, uint8_t pin)
        : IntegerBuffer(address, mask, 0, nullptr), pin(pin) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    void loop() override {
        if (hasUpdatedData()) {
            digitalWrite(pin, getData() ? HIGH : LOW);
        }
    }
};

// ============================================================================
// TEST I/O INSTANCES
// ============================================================================
// Modify these to match your panel wiring and DCS-BIOS addresses.

#if SWITCH_PIN >= 0
Switch2Pos masterArmSw("MASTER_ARM_SW", SWITCH_PIN);
#endif

#if BUTTON_PIN >= 0
Switch2Pos ufcOpt1Btn("UFC_OS1", BUTTON_PIN);
#endif

#if MC_READY_LED_PIN >= 0
LED mcReadyLed(0x740C, 0x8000, MC_READY_LED_PIN);
#endif

// ============================================================================
// ARDUINO SETUP AND LOOP
// ============================================================================

void setup() {
    dbgBegin(115200);
    delay(3000);
    dbgPrint("");
    dbgPrint("===========================================");
    dbgPrintf("ESP32 RS485 Slave v2 - %s\n", USE_ISR_MODE ? "ISR Mode" : "Driver Mode");
    dbgPrintf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    dbgPrintf("CPU: %d MHz, Flash: %d MB\n", ESP.getCpuFreqMHz(), ESP.getFlashChipSize() / 1024 / 1024);
    dbgPrintf("Slave Address: %d\n", SLAVE_ADDRESS);
    dbgPrintf("Baud Rate: %d\n", RS485_BAUD_RATE);
    dbgPrintf("TX Pin: %d, RX Pin: %d, DE Pin: %d\n", RS485_TX_PIN, RS485_RX_PIN, RS485_DE_PIN);
    dbgPrintf("TX Buffer: %d, Export Buffer: %d\n", TX_BUFFER_SIZE, EXPORT_BUFFER_SIZE);
    dbgPrintf("Timing: Warmup M=%d A=%d, Cooldown M=%d A=%d\n",
              TX_WARMUP_DELAY_MANUAL_US, TX_WARMUP_DELAY_AUTO_US,
              TX_COOLDOWN_DELAY_MANUAL_US, TX_COOLDOWN_DELAY_AUTO_US);
    dbgPrint("===========================================");

    udpDbgInit();

    dbgPrint("Initializing RS485 hardware...");
    initRS485Hardware();
    dbgPrint("RS485 hardware initialized!");

#if USE_RS485_TASK
    startRS485Task();
#else
    dbgPrint("  RS485 running in main loop mode");
#endif

    dbgPrint("Entering main loop...");
}

void loop() {
    udpDbgCheck();

#if USE_RS485_TASK
    // Task mode: FreeRTOS task handles export processing + driver mode RX
    // Main loop only handles input polling + LED updates
#else
    // No-task mode: main loop does everything
    rs485SlaveLoop();
#endif

    // Poll inputs (switches, buttons)
    PollingInput::pollInputs();

    // Update outputs (LEDs)
    ExportStreamListener::loopAll();
}
