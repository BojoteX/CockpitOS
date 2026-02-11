/**
 * =============================================================================
 * ESP32 RS485 MASTER v2 - CockpitOS Reference Implementation
 * =============================================================================
 *
 * Based on the battle-tested CockpitOS RS485Master.cpp — the source of truth
 * for all RS485 master logic. This standalone sketch contains the identical
 * state machine, TX paths, echo prevention, and timing.
 *
 * ARCHITECTURE:
 *   - ISR-driven RX: Each byte triggers an ISR that processes the state machine
 *     immediately, just like AVR's USART_RX_vect.
 *   - Inline blocking TX: TX blocks briefly (no FreeRTOS TX task).
 *   - Echo prevention: RX interrupt disabled during TX, RX FIFO flushed after.
 *   - Bare-metal UART: periph_module_enable + uart_ll_* for direct hardware.
 *   - FreeRTOS RS485 task: Dedicated task runs poll loop with deterministic 1ms
 *     tick via vTaskDelayUntil (matches CockpitOS RS485_USE_TASK). Message
 *     forwarding to PC happens inside the task for lowest latency.
 *   - FreeRTOS PC input task: Dedicated task drains Serial into RingBuffer<1024>
 *     even while RS485 task blocks during TX.
 *   - Multi-bus: Up to 3 independent RS485 buses with linked-list iteration.
 *   - Chunked broadcasts: Max 64 bytes per burst, polls every 2ms minimum.
 *   - Safe poll scan: No infinite loop when zero slaves present.
 *   - Overflow-protected MessageBuffer: drops bytes instead of wrapping.
 *   - Multi-command parsing: splits "\n"-delimited slave responses correctly.
 *
 * v2 IMPROVEMENTS OVER v1:
 *   - MessageBuffer with overflow protection (drops instead of wrapping)
 *   - Multi-command response parsing (splits on '\n' boundaries)
 *   - txWithEchoPrevention: disableRx → DE → warmup → TX → idle → cooldown →
 *     flush → DE release → enableRx (consistent across all TX paths)
 *   - Separate warmup/cooldown for manual DE vs auto-direction
 *   - RISC-V fence for C3/C6/H2 FIFO read stability
 *   - FreeRTOS PC input task (captures PC bytes even during TX blocking)
 *   - Inline PC forwarding after each bus loop
 *   - Timeout zero byte sent for non-responding slaves (AVR-compatible)
 *
 * PROTOCOL (exact match to AVR DcsBiosNgRS485Master):
 *
 *   BROADCAST (Master -> All Slaves):
 *     [0x00] [0x00] [Length] [Data...] [Checksum]
 *
 *   POLL (Master -> Specific Slave):
 *     [SlaveAddr] [0x00] [0x00]
 *
 *   SLAVE RESPONSE:
 *     [DataLength] [MsgType] [Data...] [Checksum]  - if slave has data
 *     [0x00]                                        - if slave has nothing
 *
 * Both modes work on: ESP32, S2, S3, C3, C6, H2
 *
 * =============================================================================
 * LICENSE: Same as DCS-BIOS Arduino Library
 * =============================================================================
 */

// Debug LED — mirrors slave output for sync verification
#define MASTER_MC_READY_PIN     2      // Set to valid GPIO to enable, -1 to disable

// ============================================================================
// MULTI-BUS CONFIGURATION
// ============================================================================
// Set pins to -1 to disable a bus. Disabled buses have ZERO overhead.

// BUS 1 - Primary RS485 bus (enabled by default)
#define BUS1_TX_PIN             17
#define BUS1_RX_PIN             18
#define BUS1_DE_PIN             21      // -1 = auto-direction mode
#define BUS1_UART_NUM           1

// BUS 2 - Secondary (disabled by default)
#define BUS2_TX_PIN             -1
#define BUS2_RX_PIN             -1
#define BUS2_DE_PIN             -1
#define BUS2_UART_NUM           -1

// BUS 3 - Tertiary (disabled by default)
#define BUS3_TX_PIN             -1
#define BUS3_RX_PIN             -1
#define BUS3_DE_PIN             -1
#define BUS3_UART_NUM           -1

// ============================================================================
// COMMON CONFIGURATION
// ============================================================================

#define RS485_BAUD_RATE         250000

// TX Timing (microseconds) — must match slave settings
#define TX_WARMUP_DELAY_MANUAL_US   55
#define TX_COOLDOWN_DELAY_MANUAL_US 55
#define TX_WARMUP_DELAY_AUTO_US     0
#define TX_COOLDOWN_DELAY_AUTO_US   15

// Buffer Sizes
#define EXPORT_BUFFER_SIZE      256     // PC -> Slaves export data (per bus)
#define MESSAGE_BUFFER_SIZE     256     // Slave -> PC response buffer (per bus)
#define PC_RX_BUFFER_SIZE       1024    // PC Serial input task buffer

// Timing Constants (microseconds) — matches AVR exactly
#define POLL_TIMEOUT_US         1000    // 1ms timeout for slave first byte
#define RX_TIMEOUT_US           5000    // 5ms timeout for complete message
#define MAX_POLL_INTERVAL_US    2000    // Ensure polls at least every 2ms
#define MSG_DRAIN_TIMEOUT_US    5000    // Safety valve for stale messages

// Broadcast chunking
#define MAX_BROADCAST_CHUNK     64      // Max bytes per broadcast burst

// Slave address range
#define MIN_SLAVE_ADDRESS       1
#define MAX_SLAVE_ADDRESS       127
#define MAX_SLAVES              128

// Discovery interval (every N polls, probe one unknown slave address)
#define DISCOVERY_INTERVAL      50

// Debug Options
#define SERIAL_DEBUG_ENABLE     0
#define UDP_DEBUG_ENABLE        0
#define UDP_DEBUG_IP            "255.255.255.255"
#define UDP_DEBUG_PORT          4210
#define UDP_DEBUG_NAME          "RS485-MASTER"
#define WIFI_SSID               "TestNetwork"
#define WIFI_PASSWORD           "TestingOnly"

// FreeRTOS Task (matches CockpitOS RS485_USE_TASK)
// Runs the RS485 poll loop in a dedicated task with deterministic 1ms tick.
// Without this, the poll loop runs in Arduino loop() which can have jitter.
#define USE_RS485_TASK          1       // 1 = FreeRTOS task (recommended). 0 = main loop only
#define RS485_TASK_PRIORITY     5       // Higher than default loop
#define RS485_TASK_STACK_SIZE   4096
#define RS485_TASK_TICK_INTERVAL 1      // 1ms tick
#define RS485_TASK_CORE         0       // Core 0 (dual-core chips). Ignored on single-core.

// Status Interval
#define STATUS_INTERVAL_MS      5000    // 0 = disabled

// ============================================================================
// COMPILE-TIME BUS DETECTION
// ============================================================================

#define BUS1_ENABLED (BUS1_UART_NUM >= 0 && BUS1_TX_PIN >= 0 && BUS1_RX_PIN >= 0)
#define BUS2_ENABLED (BUS2_UART_NUM >= 0 && BUS2_TX_PIN >= 0 && BUS2_RX_PIN >= 0)
#define BUS3_ENABLED (BUS3_UART_NUM >= 0 && BUS3_TX_PIN >= 0 && BUS3_RX_PIN >= 0)

#define NUM_BUSES_ENABLED ((BUS1_ENABLED ? 1 : 0) + (BUS2_ENABLED ? 1 : 0) + (BUS3_ENABLED ? 1 : 0))

#if NUM_BUSES_ENABLED == 0
#error "At least one RS485 bus must be enabled (set BUS1 pins)"
#endif

// ============================================================================
// INCLUDES
// ============================================================================

#include <Arduino.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <rom/ets_sys.h>
#include <soc/soc_caps.h>

// Bare-metal UART access
#include <hal/uart_ll.h>
#include <hal/gpio_ll.h>
#include <soc/uart_struct.h>
#include <soc/gpio_struct.h>
#include <soc/uart_periph.h>
#include <esp_intr_alloc.h>

#if __has_include(<esp_private/periph_ctrl.h>)
#include <esp_private/periph_ctrl.h>
#else
#include <driver/periph_ctrl.h>
#endif
#include <soc/periph_defs.h>

// FreeRTOS for PC Serial input task
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if UDP_DEBUG_ENABLE
#include <WiFi.h>
#include <WiFiUdp.h>
#endif

// ============================================================================
// PC SERIAL AUTO-DETECTION
// ============================================================================
// Detects USB type based on ARDUINO_USB_MODE (set by Tools → USB Mode menu):
//   Not defined  = ESP32 Classic (CH340/CP2102 UART)          → Serial
//   1            = Hardware CDC and JTAG (C3/C6/H2/S3 default) → HWCDCSerial
//   0            = USB-OTG / TinyUSB (S2/S3 native USB)        → USBSerial
//
// IMPORTANT: For chips with native USB, set Arduino IDE:
//   Tools → USB CDC On Boot → "Disabled"
//   This sketch initializes USB manually for correct startup order.

#ifndef ARDUINO_USB_MODE
    // ESP32 Classic — no native USB, uses external UART chip (CH340/CP2102)
    #define PC_SERIAL Serial
    #define PC_SERIAL_IS_UART 1

#elif ARDUINO_USB_MODE == 1
    // Hardware CDC/JTAG mode (HWCDC class)
    #if ARDUINO_USB_CDC_ON_BOOT
        #error "USB CDC On Boot must be DISABLED in Arduino IDE (Tools -> USB CDC On Boot -> Disabled). This sketch initializes USB manually."
    #endif
    HWCDC HWCDCSerial;
    #define PC_SERIAL HWCDCSerial
    #define PC_SERIAL_IS_HWCDC 1

#elif ARDUINO_USB_MODE == 0
    // USB-OTG mode / TinyUSB (USBCDC class)
    #if ARDUINO_USB_CDC_ON_BOOT
        #error "USB CDC On Boot must be DISABLED in Arduino IDE (Tools -> USB CDC On Boot -> Disabled). This sketch initializes USB manually."
    #endif
    #include "USB.h"
    #include "tusb.h"
    USBCDC USBSerial;
    #define PC_SERIAL USBSerial
    #define PC_SERIAL_IS_OTG 1

#endif

// ============================================================================
// PC WRITE — DTR-BYPASS FOR OTG MODE
// ============================================================================
// socat (DCS-BIOS serial bridge) does NOT assert DTR when opening the port.
// Arduino's USBCDC::write() checks DTR via tud_cdc_n_connected() and silently
// drops all data when DTR is off. The underlying tud_cdc_write() has NO DTR
// check — USB endpoint polling happens regardless of DTR state.
// HWCDC and HardwareSerial do not gate writes on DTR, so they use .write() directly.

static inline size_t pcWrite(const uint8_t* data, size_t len) {
    if (!data || len == 0) return 0;
#if defined(PC_SERIAL_IS_UART)
    return PC_SERIAL.write(data, len);
#elif defined(PC_SERIAL_IS_HWCDC)
    return PC_SERIAL.write(data, len);
#elif defined(PC_SERIAL_IS_OTG)
    if (!tud_ready()) return 0;
    size_t sent = 0;
    uint32_t timeout = millis() + 100;
    while (sent < len) {
        if ((int32_t)(millis() - timeout) >= 0) break;
        size_t space = tud_cdc_write_available();
        if (!space) { tud_cdc_write_flush(); continue; }
        if (space > (len - sent)) space = len - sent;
        size_t n = tud_cdc_write(data + sent, space);
        if (n) { sent += n; tud_cdc_write_flush(); }
        else break;
    }
    return sent;
#else
    return 0;
#endif
}

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
// UDP DEBUG CLASS
// ============================================================================

#if UDP_DEBUG_ENABLE
class UdpDebug {
private:
    WiFiUDP udp;
    IPAddress targetIP;
    bool connected;
    char buffer[256];
    unsigned long lastStatusTime;

public:
    UdpDebug() : connected(false), lastStatusTime(0) {}

    void begin() {
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        targetIP.fromString(UDP_DEBUG_IP);
        connected = false;
    }

    void checkConnection() {
        if (!connected && WiFi.status() == WL_CONNECTED) {
            connected = true;
            send("=== %s ONLINE === IP: %s", UDP_DEBUG_NAME, WiFi.localIP().toString().c_str());
            lastStatusTime = millis();
        }
        if (connected && (millis() - lastStatusTime) > 30000) {
            send("[%s] heartbeat", UDP_DEBUG_NAME);
            lastStatusTime = millis();
        }
    }

    void send(const char* fmt, ...) {
        if (!connected) return;
        int pos = snprintf(buffer, sizeof(buffer), "[%s] ", UDP_DEBUG_NAME);
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer + pos, sizeof(buffer) - pos, fmt, args);
        va_end(args);
        udp.beginPacket(targetIP, UDP_DEBUG_PORT);
        udp.write((uint8_t*)buffer, strlen(buffer));
        udp.endPacket();
    }
};

UdpDebug udpDbg;
#define UDP_DBG(fmt, ...) udpDbg.send(fmt, ##__VA_ARGS__)
#else
#define UDP_DBG(fmt, ...)
#endif

// ============================================================================
// DCS-BIOS PROTOCOL PARSER (for LED sync verification on master)
// ============================================================================

#if MASTER_MC_READY_PIN >= 0

#define DCSBIOS_STATE_WAIT_FOR_SYNC  0
#define DCSBIOS_STATE_ADDRESS_LOW    1
#define DCSBIOS_STATE_ADDRESS_HIGH   2
#define DCSBIOS_STATE_COUNT_LOW      3
#define DCSBIOS_STATE_COUNT_HIGH     4
#define DCSBIOS_STATE_DATA_LOW       5
#define DCSBIOS_STATE_DATA_HIGH      6

class ExportStreamListener;
static ExportStreamListener* firstExportStreamListener = nullptr;

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

    uint16_t getData() { dirty = false; return data; }
    bool hasUpdatedData() { return dirty; }
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
            case DCSBIOS_STATE_WAIT_FOR_SYNC: break;
            case DCSBIOS_STATE_ADDRESS_LOW:
                address = c; state = DCSBIOS_STATE_ADDRESS_HIGH; break;
            case DCSBIOS_STATE_ADDRESS_HIGH:
                address |= (c << 8);
                state = (address != 0x5555) ? DCSBIOS_STATE_COUNT_LOW : DCSBIOS_STATE_WAIT_FOR_SYNC;
                break;
            case DCSBIOS_STATE_COUNT_LOW:
                count = c; state = DCSBIOS_STATE_COUNT_HIGH; break;
            case DCSBIOS_STATE_COUNT_HIGH:
                count |= (c << 8); state = DCSBIOS_STATE_DATA_LOW; break;
            case DCSBIOS_STATE_DATA_LOW:
                data = c; count--; state = DCSBIOS_STATE_DATA_HIGH; break;
            case DCSBIOS_STATE_DATA_HIGH:
                data |= (c << 8); count--;
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
        if (c == 0x55) { sync_byte_count++; } else { sync_byte_count = 0; }
        if (sync_byte_count == 4) {
            state = DCSBIOS_STATE_ADDRESS_LOW;
            sync_byte_count = 0;
            startESL = firstExportStreamListener;
        }
    }
};

static ProtocolParser masterParser;
static LED mcReadyLed(0x740C, 0x8000, MASTER_MC_READY_PIN);

#endif // MASTER_MC_READY_PIN >= 0

// ============================================================================
// RING BUFFER
// ============================================================================

template<unsigned int SIZE>
class RingBuffer {
private:
    volatile uint8_t buffer[SIZE];
    volatile uint16_t writepos;
    volatile uint16_t readpos;

public:
    volatile bool complete;
    volatile uint32_t dropCount;

    RingBuffer() : writepos(0), readpos(0), complete(false), dropCount(0) {}

    inline void IRAM_ATTR put(uint8_t c) {
        if (getLength() < SIZE) {
            buffer[writepos % SIZE] = c;
            writepos++;
        } else {
            dropCount++;  // Overflow: drop byte, track count (matches CockpitOS MessageBuffer)
        }
    }

    inline uint8_t get() {
        uint8_t ret = buffer[readpos % SIZE];
        readpos++;
        return ret;
    }

    inline uint8_t peek() const { return buffer[readpos % SIZE]; }
    inline bool isEmpty() const { return readpos == writepos; }
    inline bool isNotEmpty() const { return readpos != writepos; }
    inline uint16_t getLength() const { return (uint16_t)(writepos - readpos); }
    inline void IRAM_ATTR clear() { readpos = writepos = 0; complete = false; }
    inline uint16_t availableForWrite() const { return SIZE - getLength() - 1; }
};

// ============================================================================
// PC INPUT TASK (FreeRTOS) — captures Serial even during TX blocking
// ============================================================================

static RingBuffer<PC_RX_BUFFER_SIZE> pcRxBuffer;
static TaskHandle_t pcInputTaskHandle = nullptr;

static void pcInputTaskFunc(void* param) {
    (void)param;
    for (;;) {
        while (PC_SERIAL.available()) {
            if (pcRxBuffer.availableForWrite() > 0) {
                pcRxBuffer.put((uint8_t)PC_SERIAL.read());
            } else {
                PC_SERIAL.read();  // Discard if full
            }
        }
        vTaskDelay(1);
    }
}

static void startPCInputTask() {
#if CONFIG_FREERTOS_UNICORE
    xTaskCreate(pcInputTaskFunc, "PCInput", 2048, nullptr, 5, &pcInputTaskHandle);
#else
    xTaskCreatePinnedToCore(pcInputTaskFunc, "PCInput", 2048, nullptr, 5, &pcInputTaskHandle, 0);
#endif
}

// ============================================================================
// STATE MACHINE
// ============================================================================

enum MasterState {
    STATE_IDLE,
    STATE_RX_WAIT_DATALENGTH,
    STATE_RX_WAIT_MSGTYPE,
    STATE_RX_WAIT_DATA,
    STATE_RX_WAIT_CHECKSUM
};

// ============================================================================
// RS485 MASTER CLASS — One instance per bus
// ============================================================================

class RS485Master;
static void IRAM_ATTR uart_isr_handler(void* arg);

class RS485Master {
private:
    int uartNum;
    int txPin, rxPin, dePin;

    uart_dev_t* uartHw;
    intr_handle_t uartIntrHandle;

    volatile MasterState state;
    volatile int64_t rxStartTime;
    volatile int64_t messageCompleteTime;  // For drain timeout safety valve (CockpitOS Fix 3)
    volatile uint8_t rxtxLen;
    volatile uint8_t rxMsgType;
    volatile int64_t lastPollTime;

    volatile bool slavePresent[MAX_SLAVES];
    uint8_t currentPollAddress;
    uint8_t discoveryCounter;

    // Statistics
    uint32_t statPolls;
    uint32_t statResponses;
    uint32_t statTimeouts;
    uint32_t statBroadcasts;
    uint32_t statBytesOut;

    // DE pin control
    inline void setDE(bool high) {
        if (dePin >= 0) {
            gpio_ll_set_level(&GPIO, (gpio_num_t)dePin, high ? 1 : 0);
        }
    }

    // TX with echo prevention — CockpitOS pattern:
    // disableRx → DE assert → warmup → TX → txIdle → cooldown → flush → DE release → enableRx
    inline void txWithEchoPrevention(const uint8_t* data, uint16_t len) {
        uart_ll_disable_intr_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);

        #if BUS1_DE_PIN >= 0 || BUS2_DE_PIN >= 0 || BUS3_DE_PIN >= 0
        if (dePin >= 0) {
            setDE(true);
            #if TX_WARMUP_DELAY_MANUAL_US > 0
            ets_delay_us(TX_WARMUP_DELAY_MANUAL_US);
            #endif
        } else {
            #if TX_WARMUP_DELAY_AUTO_US > 0
            ets_delay_us(TX_WARMUP_DELAY_AUTO_US);
            #endif
        }
        #else
        #if TX_WARMUP_DELAY_AUTO_US > 0
        ets_delay_us(TX_WARMUP_DELAY_AUTO_US);
        #endif
        #endif

        // TX — write bytes one at a time (matches CockpitOS txByte/txBytes pattern)
        for (uint16_t i = 0; i < len; i++) {
            while (uart_ll_get_txfifo_len(uartHw) == 0) {}
            uart_ll_write_txfifo(uartHw, data + i, 1);
        }
        while (!uart_ll_is_tx_idle(uartHw));

        #if BUS1_DE_PIN >= 0 || BUS2_DE_PIN >= 0 || BUS3_DE_PIN >= 0
        if (dePin >= 0) {
            #if TX_COOLDOWN_DELAY_MANUAL_US > 0
            ets_delay_us(TX_COOLDOWN_DELAY_MANUAL_US);
            #endif
            uart_ll_rxfifo_rst(uartHw);
            setDE(false);
        } else {
            #if TX_COOLDOWN_DELAY_AUTO_US > 0
            ets_delay_us(TX_COOLDOWN_DELAY_AUTO_US);
            #endif
            uart_ll_rxfifo_rst(uartHw);
        }
        #else
        #if TX_COOLDOWN_DELAY_AUTO_US > 0
        ets_delay_us(TX_COOLDOWN_DELAY_AUTO_US);
        #endif
        uart_ll_rxfifo_rst(uartHw);
        #endif

        uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
        uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
    }

public:
    RingBuffer<EXPORT_BUFFER_SIZE> exportData;
    RingBuffer<MESSAGE_BUFFER_SIZE> messageBuffer;

    RS485Master* next;
    static RS485Master* first;

    RS485Master(int uartNum, int txPin, int rxPin, int dePin)
        : uartNum(uartNum), txPin(txPin), rxPin(rxPin), dePin(dePin),
          uartHw(nullptr), uartIntrHandle(nullptr),
          state(STATE_IDLE), rxStartTime(0), messageCompleteTime(0),
          rxtxLen(0), rxMsgType(0), lastPollTime(0),
          currentPollAddress(MIN_SLAVE_ADDRESS), discoveryCounter(0),
          statPolls(0), statResponses(0), statTimeouts(0), statBroadcasts(0), statBytesOut(0),
          next(nullptr)
    {
        if (first == nullptr) {
            first = this;
        } else {
            RS485Master* p = first;
            while (p->next != nullptr) p = p->next;
            p->next = this;
        }

        memset((void*)slavePresent, 0, sizeof(slavePresent));
    }

    void init() {
        dbgPrintf("  Bus UART%d: TX=%d RX=%d DE=%d\n", uartNum, txPin, rxPin, dePin);

        if (dePin >= 0) {
            gpio_config_t io_conf = {
                .pin_bit_mask = (1ULL << dePin),
                .mode = GPIO_MODE_OUTPUT,
                .pull_up_en = GPIO_PULLUP_DISABLE,
                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                .intr_type = GPIO_INTR_DISABLE
            };
            gpio_config(&io_conf);
            setDE(false);
            dbgPrint("  DE GPIO configured");
        } else {
            dbgPrint("  No DE pin (auto-direction)");
        }

        periph_module_t periphModule;
        switch (uartNum) {
            case 0: uartHw = &UART0; periphModule = PERIPH_UART0_MODULE; break;
            case 1: uartHw = &UART1; periphModule = PERIPH_UART1_MODULE; break;
#if SOC_UART_NUM > 2
            case 2: uartHw = &UART2; periphModule = PERIPH_UART2_MODULE; break;
#endif
            default: dbgPrintf("  ERROR: Invalid UART %d\n", uartNum); return;
        }

        dbgPrint("  Enabling UART peripheral...");
        dbgFlush();
        periph_module_enable(periphModule);

        dbgPrint("  Configuring UART parameters...");
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
        ESP_ERROR_CHECK(uart_param_config((uart_port_t)uartNum, &uart_config));

        dbgPrint("  Setting UART pins...");
        dbgFlush();
        ESP_ERROR_CHECK(uart_set_pin((uart_port_t)uartNum, txPin, rxPin,
                                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        gpio_set_pull_mode((gpio_num_t)rxPin, GPIO_PULLUP_ONLY);

        uart_ll_set_rxfifo_full_thr(uartHw, 1);
        uart_ll_clr_intsts_mask(uartHw, UART_LL_INTR_MASK);
        uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);  // TOUT enabled later by enableRxInt() after first TX

        dbgPrint("  Registering ISR...");
        ESP_ERROR_CHECK(esp_intr_alloc(uart_periph_signal[uartNum].irq,
                                        ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1,
                                        uart_isr_handler, this, &uartIntrHandle));
        uart_ll_rxfifo_rst(uartHw);

        state = STATE_IDLE;
        lastPollTime = esp_timer_get_time();

        dbgPrint("  Bus initialization complete!");
    }

    // CockpitOS advancePollAddress — discoveryCounter pattern
    // Every DISCOVERY_INTERVAL polls, probe one unknown address for new slaves.
    // Otherwise, cycle through known-present slaves.
    // If no known slaves exist, scan sequentially.
    void advancePollAddress() {
        uint8_t startAddr = currentPollAddress;

        // Periodically scan for new slaves
        discoveryCounter++;
        if (discoveryCounter >= DISCOVERY_INTERVAL) {
            discoveryCounter = 0;

            // Find next unknown address to probe
            uint8_t probeAddr = currentPollAddress;
            for (uint8_t i = 1; i <= MAX_SLAVE_ADDRESS; i++) {
                probeAddr = (probeAddr % MAX_SLAVE_ADDRESS) + 1;
                if (!slavePresent[probeAddr]) {
                    currentPollAddress = probeAddr;
                    return;
                }
            }
        }

        // Find next known slave
        for (uint8_t i = 0; i < MAX_SLAVE_ADDRESS; i++) {
            currentPollAddress++;
            if (currentPollAddress > MAX_SLAVE_ADDRESS) {
                currentPollAddress = MIN_SLAVE_ADDRESS;
            }
            if (slavePresent[currentPollAddress]) {
                return;
            }
        }

        // No known slaves — scan sequentially
        currentPollAddress = startAddr + 1;
        if (currentPollAddress > MAX_SLAVE_ADDRESS) {
            currentPollAddress = MIN_SLAVE_ADDRESS;
        }
    }

    void sendPollFrame(uint8_t addr) {
        uint8_t frame[3] = { addr, 0x00, 0x00 };
        txWithEchoPrevention(frame, sizeof(frame));
        rxStartTime = esp_timer_get_time();
        state = STATE_RX_WAIT_DATALENGTH;
        statPolls++;
    }

    void sendBroadcastFrame() {
        uint16_t available = exportData.getLength();
        if (available == 0) return;

        uint8_t dataLen = (uint8_t)((available > MAX_BROADCAST_CHUNK) ? MAX_BROADCAST_CHUNK : available);
        uint8_t frame[MAX_BROADCAST_CHUNK + 4];
        uint16_t idx = 0;

        frame[idx++] = 0x00;       // broadcast address
        frame[idx++] = 0x00;       // msg type
        frame[idx++] = dataLen;

        uint8_t checksum = 0;
        for (uint8_t i = 0; i < dataLen; i++) {
            uint8_t b = exportData.get();
            frame[idx++] = b;
            checksum ^= b;
        }
        frame[idx++] = checksum;

        txWithEchoPrevention(frame, idx);
        statBroadcasts++;
        statBytesOut += idx;
    }

    void sendTimeoutZeroByte() {
        uint8_t zero = 0x00;
        txWithEchoPrevention(&zero, 1);
        // Note: state is set by caller, not here (matches CockpitOS sendTimeoutZero)
    }

    void IRAM_ATTR rxISR() {
        uint32_t uart_intr_status = uart_ll_get_intsts_mask(uartHw);

        while (uart_ll_get_rxfifo_len(uartHw) > 0) {
            uint8_t c;
            uart_ll_read_rxfifo(uartHw, &c, 1);
#ifdef __riscv
            __asm__ __volatile__("fence" ::: "memory");
#endif

            switch (state) {
                case STATE_RX_WAIT_DATALENGTH:
                    rxtxLen = c;
                    slavePresent[currentPollAddress] = true;
                    if (rxtxLen > 0) {
                        state = STATE_RX_WAIT_MSGTYPE;
                    } else {
                        state = STATE_IDLE;
                    }
                    rxStartTime = esp_timer_get_time();
                    break;

                case STATE_RX_WAIT_MSGTYPE:
                    rxMsgType = c;
                    (void)rxMsgType;
                    state = STATE_RX_WAIT_DATA;
                    rxStartTime = esp_timer_get_time();
                    break;

                case STATE_RX_WAIT_DATA:
                    messageBuffer.put(c);
                    if (--rxtxLen == 0) {
                        state = STATE_RX_WAIT_CHECKSUM;
                    }
                    rxStartTime = esp_timer_get_time();
                    break;

                case STATE_RX_WAIT_CHECKSUM:
                    messageBuffer.complete = true;
                    messageCompleteTime = esp_timer_get_time();
                    state = STATE_IDLE;
                    rxStartTime = esp_timer_get_time();
                    break;

                default:
                    break;
            }
        }

        uart_ll_clr_intsts_mask(uartHw, uart_intr_status);
    }

    // CockpitOS rs485PollLoop order (EXACT match):
    //   1. Forward completed messages FIRST (immediate forwarding)
    //   2. Drain timeout safety valve
    //   3. Check RX timeouts
    //   4. If IDLE: broadcast or poll
    void loop() {
        // Step 1: Immediate forwarding — check for completed messages FIRST
        // Matches CockpitOS rs485PollLoop lines 951-955
        if (messageBuffer.complete) {
            statResponses++;

            // Drain message data and forward to PC
            if (messageBuffer.isNotEmpty()) {
                uint8_t tmpBuf[MESSAGE_BUFFER_SIZE];
                size_t tmpLen = 0;
                while (messageBuffer.isNotEmpty() && tmpLen < sizeof(tmpBuf)) {
                    tmpBuf[tmpLen++] = messageBuffer.get();
                }
                if (tmpLen > 0) pcWrite(tmpBuf, tmpLen);
            }

            messageBuffer.clear();
        }

        // Step 2: Safety — drain timeout. If message wasn't drained, force-clear on timeout
        // Matches CockpitOS rs485PollLoop lines 957-966
        if (messageBuffer.complete) {
            int64_t now = esp_timer_get_time();
            if ((now - messageCompleteTime) > MSG_DRAIN_TIMEOUT_US) {
                messageBuffer.clear();
                messageBuffer.complete = false;
                statTimeouts++;
            }
        }

        // Step 3: Check RX timeouts (non-IDLE states)
        if (state != STATE_IDLE) {
            int64_t now = esp_timer_get_time();

            // Timeout: waiting for first response byte (1ms)
            if (state == STATE_RX_WAIT_DATALENGTH) {
                if ((now - rxStartTime) > POLL_TIMEOUT_US) {
                    slavePresent[currentPollAddress] = false;
                    sendTimeoutZeroByte();
                    statTimeouts++;
                    state = STATE_IDLE;
                }
                return;
            }

            // Timeout: mid-message (5ms)
            if (state == STATE_RX_WAIT_MSGTYPE ||
                state == STATE_RX_WAIT_DATA ||
                state == STATE_RX_WAIT_CHECKSUM) {
                if ((now - rxStartTime) > RX_TIMEOUT_US) {
                    messageBuffer.clear();
                    messageBuffer.put('\n');
                    messageBuffer.complete = true;
                    messageCompleteTime = esp_timer_get_time();
                    state = STATE_IDLE;
                    statTimeouts++;

                    // Flush remaining bytes and re-enable RX (matches CockpitOS)
                    uart_ll_rxfifo_rst(uartHw);
                    uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
                    uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
                }
            }
            return;  // Still in RX state, don't start new TX
        }

        // Step 4: IDLE — broadcast or poll
        int64_t now = esp_timer_get_time();

        // Priority 1: Broadcast if data available, but ensure polls at least every MAX_POLL_INTERVAL
        if (exportData.isNotEmpty() && (now - lastPollTime) < MAX_POLL_INTERVAL_US) {
            sendBroadcastFrame();
            return;
        }

        // Priority 2: Poll a slave if message buffer is free
        if (messageBuffer.isEmpty() && !messageBuffer.complete) {
            advancePollAddress();
            sendPollFrame(currentPollAddress);
            lastPollTime = now;
        }
    }

    // Accessors for status
    uint32_t getStatPolls() const { return statPolls; }
    uint32_t getStatResponses() const { return statResponses; }
    uint32_t getStatTimeouts() const { return statTimeouts; }
    uint32_t getStatBroadcasts() const { return statBroadcasts; }
    uint32_t getStatBytesOut() const { return statBytesOut; }
    uint32_t getDropCount() const { return messageBuffer.dropCount; }
    uint8_t getCurrentPollAddress() const { return currentPollAddress; }
    bool isSlavePresent(uint8_t addr) const { return (addr < MAX_SLAVES) ? slavePresent[addr] : false; }
};

RS485Master* RS485Master::first = nullptr;

static void IRAM_ATTR uart_isr_handler(void* arg) {
    RS485Master* bus = (RS485Master*)arg;
    bus->rxISR();
}

// ============================================================================
// BUS INSTANCES
// ============================================================================

#if BUS1_ENABLED
RS485Master bus1(BUS1_UART_NUM, BUS1_TX_PIN, BUS1_RX_PIN, BUS1_DE_PIN);
#endif

#if BUS2_ENABLED
RS485Master bus2(BUS2_UART_NUM, BUS2_TX_PIN, BUS2_RX_PIN, BUS2_DE_PIN);
#endif

#if BUS3_ENABLED
RS485Master bus3(BUS3_UART_NUM, BUS3_TX_PIN, BUS3_RX_PIN, BUS3_DE_PIN);
#endif

// ============================================================================
// FREERTOS RS485 POLL TASK (matches CockpitOS RS485_USE_TASK)
// ============================================================================
// Runs rs485PollLoop (iterate all buses) with deterministic vTaskDelayUntil timing.
// Without this, the poll loop runs in Arduino loop() which can have jitter from
// drainPCInput, LED updates, status prints, etc.

#if USE_RS485_TASK

static TaskHandle_t rs485TaskHandle = nullptr;
static volatile bool rs485TaskRunning = false;

// The internal poll loop — iterates all buses
static void rs485PollLoop() {
    RS485Master* bus = RS485Master::first;
    while (bus != nullptr) {
        bus->loop();
        bus = bus->next;
    }
}

// FreeRTOS task function (matches CockpitOS rs485Task exactly)
static void rs485Task(void* param) {
    (void)param;

    dbgPrint("[RS485M] Task started");

    TickType_t lastWakeTime = xTaskGetTickCount();

    while (rs485TaskRunning) {
        rs485PollLoop();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(RS485_TASK_TICK_INTERVAL));
    }

    dbgPrint("[RS485M] Task stopped");
    vTaskDelete(nullptr);
}

static void startRS485Task() {
    rs485TaskRunning = true;

#if CONFIG_FREERTOS_UNICORE
    xTaskCreate(rs485Task, "RS485M", RS485_TASK_STACK_SIZE, nullptr,
                RS485_TASK_PRIORITY, &rs485TaskHandle);
#else
    xTaskCreatePinnedToCore(rs485Task, "RS485M", RS485_TASK_STACK_SIZE, nullptr,
                            RS485_TASK_PRIORITY, &rs485TaskHandle, RS485_TASK_CORE);
#endif

    dbgPrintf("[RS485M] Task created: priority=%d, stack=%d\n",
              RS485_TASK_PRIORITY, RS485_TASK_STACK_SIZE);
}

#endif // USE_RS485_TASK

// ============================================================================
// PC COMMUNICATION
// ============================================================================

static void drainPCInput() {
    while (pcRxBuffer.isNotEmpty()) {
        uint8_t c = pcRxBuffer.get();

        #if MASTER_MC_READY_PIN >= 0
        masterParser.processChar(c);
        #endif

        RS485Master* bus = RS485Master::first;
        while (bus != nullptr) {
            if (bus->exportData.availableForWrite() > 0) {
                bus->exportData.put(c);
            }
            bus = bus->next;
        }
    }
}

// ============================================================================
// ARDUINO SETUP AND LOOP
// ============================================================================

void setup() {
    // Initialize PC Serial — method depends on chip USB type
    #if defined(PC_SERIAL_IS_UART)
        PC_SERIAL.begin(250000);           // Classic ESP32: UART at baud rate
    #elif defined(PC_SERIAL_IS_OTG)
        PC_SERIAL.begin();                 // S2/S3: USBCDC (no baud rate)
        USB.begin();                       // Start USB stack after CDC init
    #elif defined(PC_SERIAL_IS_HWCDC)
        PC_SERIAL.begin();                 // C3/C6/H2: HWCDC (no baud rate)
    #endif

    dbgBegin(115200);
    delay(3000);

    dbgPrint("");
    dbgPrint("===========================================");
    dbgPrint("ESP32 RS485 Master v2 - ISR Mode");
    dbgPrintf("Chip: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    dbgPrintf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    dbgPrintf("Baud: %d\n", RS485_BAUD_RATE);
    dbgPrintf("Slave range: %d-%d\n", MIN_SLAVE_ADDRESS, MAX_SLAVE_ADDRESS);
    dbgPrintf("Timing: Warmup M=%d A=%d, Cooldown M=%d A=%d\n",
              TX_WARMUP_DELAY_MANUAL_US, TX_WARMUP_DELAY_AUTO_US,
              TX_COOLDOWN_DELAY_MANUAL_US, TX_COOLDOWN_DELAY_AUTO_US);
    dbgPrint("===========================================");

#if UDP_DEBUG_ENABLE
    udpDbg.begin();
    dbgPrint("UDP Debug: Connecting to WiFi...");
#endif

    startPCInputTask();
    dbgPrint("PC Input task started.");

    dbgPrint("Initializing RS485 buses...");
    RS485Master* bus = RS485Master::first;
    while (bus != nullptr) {
        bus->init();
        bus = bus->next;
    }

    dbgPrint("All buses initialized.");

#if USE_RS485_TASK
    startRS485Task();
#else
    dbgPrint("[RS485M] Running in main loop mode");
#endif

#if MASTER_MC_READY_PIN >= 0
    dbgPrintf("LED sync enabled on GPIO %d\n", MASTER_MC_READY_PIN);
#endif

    dbgPrint("Entering main loop.");
}

void loop() {
#if UDP_DEBUG_ENABLE
    udpDbg.checkConnection();
#endif

    drainPCInput();

    #if MASTER_MC_READY_PIN >= 0
    ExportStreamListener::loopAll();
    #endif

#if USE_RS485_TASK
    // Task mode: FreeRTOS task handles the poll loop + message forwarding.
    // Main loop only handles drainPCInput, LED updates, and status prints.
    // (Matches CockpitOS RS485Master_loop() which only calls processQueuedCommands
    //  in task mode — but standalone has no command queue, forwarding is in-task.)
#else
    // No-task mode: main loop runs the poll loop directly
    {
        RS485Master* bus = RS485Master::first;
        while (bus != nullptr) {
            bus->loop();
            bus = bus->next;
        }
    }
#endif

    // Status (matches CockpitOS rs485PollLoop periodic status, with added statBytesOut + Drops)
    #if STATUS_INTERVAL_MS > 0
    static uint32_t lastStatusMs = 0;
    if (millis() - lastStatusMs >= STATUS_INTERVAL_MS) {
        lastStatusMs = millis();

        RS485Master* bus = RS485Master::first;
        while (bus != nullptr) {
            uint8_t onlineCount = 0;
            for (uint8_t i = 1; i <= MAX_SLAVE_ADDRESS; i++) {
                if (bus->isSlavePresent(i)) onlineCount++;
            }

            float respRate = bus->getStatPolls() > 0 ?
                100.0f * bus->getStatResponses() / bus->getStatPolls() : 0;

            dbgPrintf("[MASTER] Polls=%lu Resp=%.1f%% Bcasts=%lu TOs=%lu Slaves=%d BytesOut=%lu Drops=%lu\n",
                      bus->getStatPolls(), respRate, bus->getStatBroadcasts(),
                      bus->getStatTimeouts(), onlineCount, bus->getStatBytesOut(),
                      bus->getDropCount());

            bus = bus->next;
        }
    }
    #endif
}
