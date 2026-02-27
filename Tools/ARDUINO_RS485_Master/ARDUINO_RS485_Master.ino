/*
   Arduino RS-485 Master — Self-contained with LED sync test

   Based on the official DCS-BIOS RS485 Master library code, with one addition:
   every PC byte is also fed into a local protocol parser that drives an
   output LED on pin 10. This lets you visually verify sync with a slave
   that has the same LED on pin 10.

   Hardware: Arduino Mega 2560
   TX1 = 18, RX1 = 19, Direction control = pin 2
   LED output = pin 10 (with 220Ω resistor to GND)

   DIAGNOSTIC: Pin 13 (built-in LED) toggles every time a DCS-BIOS sync
   frame is detected, so you can verify the stream is being received and
   parsed correctly even if the target LED output never comes.
*/

// ============================================================================
// USER OPTION — Select which aircraft LED output to monitor
// ============================================================================
// Uncomment ONE of the following to select the LED output this master will
// parse from the DCS-BIOS export stream and drive on LED_OUTPUT_PIN (pin 10).
//
//   HORNET_MC_READY   — FA-18C Master Arm Panel "READY" light (yellow)
//                        Address 0x740C, Mask 0x8000, Shift 15
//
//   APACHE_PLT_CMWS_ARM — AH-64D Pilot CMWS Flare Squibs ARM/SAFE indicator
//                          Address 0x8712, Mask 0x0020, Shift 5
//

// #define HORNET_MC_READY         // FA-18C  MC_READY          addr=0x740C mask=0x8000
#define APACHE_PLT_CMWS_ARM  // AH-64D  PLT_CMWS_ARM      addr=0x8712 mask=0x0020

// --- Resolve selection to address + mask ---
#if defined(HORNET_MC_READY) && defined(APACHE_PLT_CMWS_ARM)
    #error "Select only ONE aircraft LED output — not both!"
#elif defined(HORNET_MC_READY)
    #define LED_DCS_ADDRESS  0x740C
    #define LED_DCS_MASK     0x8000
#elif defined(APACHE_PLT_CMWS_ARM)
    #define LED_DCS_ADDRESS  0x8712
    #define LED_DCS_MASK     0x0020
#else
    #error "No aircraft LED output selected — uncomment one option above!"
#endif

#define UART1_TXENABLE_PIN 2
#define LED_OUTPUT_PIN 10
#define SYNC_LED_PIN 13     // Built-in LED - toggles on each sync frame

// We do NOT define DCSBIOS_RS485_MASTER here — this sketch is fully
// self-contained and does not use DcsBios.h for the master transport.
// Instead, the RS485 master logic is included inline below (from the
// official DCS-BIOS library) with the LED hook added.

#include <Arduino.h>
#include <stdint.h>
#include <avr/interrupt.h>

#if not (defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__))
    #error This sketch requires an Arduino Mega!
#endif

// ============================================================================
// PROTOCOL STATE DEFINES
// ============================================================================

#define DCSBIOS_STATE_WAIT_FOR_SYNC 0
#define DCSBIOS_STATE_ADDRESS_LOW   1
#define DCSBIOS_STATE_ADDRESS_HIGH  2
#define DCSBIOS_STATE_COUNT_LOW     3
#define DCSBIOS_STATE_COUNT_HIGH    4
#define DCSBIOS_STATE_DATA_LOW      5
#define DCSBIOS_STATE_DATA_HIGH     6

// ============================================================================
// DIRECT PORT MACROS (must be before first use in ledParser)
// ============================================================================

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// ============================================================================
// RING BUFFER (from DCS-BIOS library)
// ============================================================================

namespace DcsBios {

template<unsigned int SIZE>
class RingBuffer {
    volatile uint8_t buffer[SIZE];
    volatile uint8_t writepos;
    volatile uint8_t readpos;
public:
    RingBuffer() : writepos(0), readpos(0) {}
    volatile bool complete = false;
    __attribute__((always_inline)) void put(uint8_t c) { buffer[writepos] = c; writepos = ++writepos % SIZE; }
    __attribute__((always_inline)) bool isEmpty() { return readpos == writepos; }
    __attribute__((always_inline)) bool isNotEmpty() { return readpos != writepos; }
    __attribute__((always_inline)) uint8_t get() { uint8_t ret = buffer[readpos]; readpos = ++readpos % SIZE; return ret; }
    __attribute__((always_inline)) uint8_t getLength() { return (uint8_t)(writepos - readpos) % SIZE; }
    __attribute__((always_inline)) void clear() { readpos = 0; writepos = 0; }
    __attribute__((always_inline)) uint8_t availableForWrite() { return SIZE - getLength() - 1; }
};

// ============================================================================
// LED SYNC — Minimal protocol parser + LED driver
// ============================================================================
// Parses the raw DCS-BIOS export stream the master receives from the PC.
// Watches for LED_DCS_ADDRESS and drives LED_OUTPUT_PIN when mask matches.
//
// Uses DcsBios::LED-style approach: intercepts any address/data pair
// matching our target and applies the mask immediately.

static uint8_t  ledParserState = DCSBIOS_STATE_WAIT_FOR_SYNC;
static uint16_t ledParserAddress;
static uint16_t ledParserCount;
static uint16_t ledParserData;
static uint8_t  ledParserSyncCount = 0;

// Diagnostic: sync frame counter + LED toggle
static volatile bool syncLedState = false;

void ledParserProcessByte(uint8_t c) {
    switch (ledParserState) {
        case DCSBIOS_STATE_WAIT_FOR_SYNC:
            break;
        case DCSBIOS_STATE_ADDRESS_LOW:
            ledParserAddress = (uint16_t)c;
            ledParserState = DCSBIOS_STATE_ADDRESS_HIGH;
            break;
        case DCSBIOS_STATE_ADDRESS_HIGH:
            ledParserAddress = ((uint16_t)c << 8) | ledParserAddress;
            if (ledParserAddress != 0x5555) {
                ledParserState = DCSBIOS_STATE_COUNT_LOW;
            } else {
                ledParserState = DCSBIOS_STATE_WAIT_FOR_SYNC;
            }
            break;
        case DCSBIOS_STATE_COUNT_LOW:
            ledParserCount = (uint16_t)c;
            ledParserState = DCSBIOS_STATE_COUNT_HIGH;
            break;
        case DCSBIOS_STATE_COUNT_HIGH:
            ledParserCount = ((uint16_t)c << 8) | ledParserCount;
            ledParserState = DCSBIOS_STATE_DATA_LOW;
            break;
        case DCSBIOS_STATE_DATA_LOW:
            ledParserData = (uint16_t)c;
            ledParserCount--;
            ledParserState = DCSBIOS_STATE_DATA_HIGH;
            break;
        case DCSBIOS_STATE_DATA_HIGH:
            ledParserData = ((uint16_t)c << 8) | ledParserData;
            ledParserCount--;
            // Check if this is our LED address
            if (ledParserAddress == LED_DCS_ADDRESS) {
                if (ledParserData & LED_DCS_MASK) {
                    sbi(PORTB, 4);   // Pin 10 HIGH — direct port, ISR-safe
                } else {
                    cbi(PORTB, 4);   // Pin 10 LOW — direct port, ISR-safe
                }
            }
            ledParserAddress += 2;
            if (ledParserCount == 0) {
                ledParserState = DCSBIOS_STATE_ADDRESS_LOW;
            } else {
                ledParserState = DCSBIOS_STATE_DATA_LOW;
            }
            break;
    }

    // Sync detection (runs in parallel with state machine)
    if (c == 0x55) {
        ledParserSyncCount++;
    } else {
        ledParserSyncCount = 0;
    }
    if (ledParserSyncCount == 4) {
        ledParserState = DCSBIOS_STATE_ADDRESS_LOW;
        ledParserSyncCount = 0;
        // Toggle built-in LED on each sync — visual proof of stream parsing
        syncLedState = !syncLedState;
        if (syncLedState) {
            sbi(PORTB, 7);   // Pin 13 HIGH on Mega
        } else {
            cbi(PORTB, 7);   // Pin 13 LOW on Mega
        }
    }
}

// ============================================================================
// RS485 MASTER (from DCS-BIOS library — with LED hook in rxISR)
// ============================================================================

// Debug scope pins (directly manipulate port registers for minimum ISR overhead)
#define s8()  sbi(PORTH,5)
#define c8()  cbi(PORTH,5)
#define s9()  sbi(PORTH,6)
#define c9()  cbi(PORTH,6)

class RS485Master;
class MasterPCConnection;

// Forward declarations
extern MasterPCConnection uart0;

class RS485Master {
private:
    volatile uint8_t poll_address;
    volatile uint8_t scan_address_counter;
    volatile uint8_t poll_address_counter;
    volatile unsigned long rx_start_time;
    void advancePollAddress();

    volatile uint8_t *udr;
    volatile uint8_t *ucsra;
    volatile uint8_t *ucsrb;
    volatile uint8_t *ucsrc;

    volatile uint8_t *txen_port;
    volatile uint8_t txen_pin_mask;
    volatile uint8_t rxtx_len;
    volatile uint8_t rx_msgtype;
    volatile uint8_t checksum;
public:
    static RS485Master* firstRS485Master;
    RS485Master* nextRS485Master;

    __attribute__((always_inline)) void set_txen() { *ucsrb &= ~((1<<RXEN0) | (1<<RXCIE0)); *txen_port |= txen_pin_mask; *ucsrb |= (1<<TXEN0) | (1<<TXCIE0); };
    __attribute__((always_inline)) void clear_txen() { *ucsrb &= ~((1<<TXEN0) | (1<<TXCIE0)); *txen_port &= ~txen_pin_mask; *ucsrb |= (1<<RXEN0) | (1<<RXCIE0); };
    __attribute__((always_inline)) void tx_byte(uint8_t c) { set_txen(); *udr = c; *ucsra |= (1<<TXC0); };
    __attribute__((always_inline)) void set_udrie() { *ucsrb |= (1<<UDRIE0); }
    __attribute__((always_inline)) void clear_udrie() { *ucsrb &= ~(1<<UDRIE0); }
    DcsBios::RingBuffer<128> exportData;
    DcsBios::RingBuffer<32> messageBuffer;
    volatile bool slave_present[128];
    volatile uint8_t state;
    enum RS485State {
        IDLE,
        POLL_ADDRESS_SENT,
        POLL_MSGTYPE_SENT,
        POLL_DATALENGTH_SENT,
        TIMEOUT_ZEROBYTE_SENT,
        RX_WAIT_DATALENGTH,
        RX_WAIT_MSGTYPE,
        RX_WAIT_DATA,
        RX_WAIT_CHECKSUM,
        TX_ADDRESS_SENT,
        TX_MSGTYPE_SENT,
        TX,
        TX_CHECKSUM_SENT
    };

    RS485Master(volatile uint8_t *udr, volatile uint8_t *ucsra, volatile uint8_t *ucsrb, volatile uint8_t *ucsrc, uint8_t txen_pin);
    void __attribute__((always_inline)) rxISR();
    void __attribute__((always_inline)) udreISR();
    void __attribute__((always_inline)) txcISR();
    void loop();
};
RS485Master* RS485Master::firstRS485Master = NULL;

class MasterPCConnection {
private:
    volatile uint8_t *udr;
    volatile uint8_t *ucsra;
    volatile uint8_t *ucsrb;
    volatile uint8_t *ucsrc;
    __attribute__((always_inline)) void clear_udrie() { *ucsrb &= ~(1<<UDRIE0); }
    __attribute__((always_inline)) void advanceTxBuffer();
    volatile unsigned long tx_start_time;
public:
    RS485Master* next_tx_rs485_master;
    __attribute__((always_inline)) void set_udrie() { *ucsrb |= (1<<UDRIE0); }
    MasterPCConnection(volatile uint8_t *udr, volatile uint8_t *ucsra, volatile uint8_t *ucsrb, volatile uint8_t *ucsrc);
    __attribute__((always_inline)) void udreISR();
    __attribute__((always_inline)) void rxISR();
    void checkTimeout();
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

MasterPCConnection uart0(&UDR0, &UCSR0A, &UCSR0B, &UCSR0C);

RS485Master::RS485Master(volatile uint8_t *udr, volatile uint8_t *ucsra, volatile uint8_t *ucsrb, volatile uint8_t *ucsrc, uint8_t txen_pin) :
udr(udr), ucsra(ucsra), ucsrb(ucsrb), ucsrc(ucsrc) {
    if (firstRS485Master == NULL) {
        firstRS485Master = this;
        this->nextRS485Master = this;
        uart0.next_tx_rs485_master = this;
    } else {
        this->nextRS485Master = firstRS485Master->nextRS485Master;
        firstRS485Master->nextRS485Master = this;
    }
    txen_port = portOutputRegister(digitalPinToPort(txen_pin));
    txen_pin_mask = digitalPinToBitMask(txen_pin);
    state = IDLE;
    clear_txen();
    poll_address_counter = 1;
    scan_address_counter = 1;
    slave_present[0] = true;
    for (uint8_t i = 1; i < 128; i++) slave_present[i] = false;
}

void RS485Master::advancePollAddress() {
    poll_address_counter = (poll_address_counter + 1) % 128;
    while (!slave_present[poll_address_counter])
        poll_address_counter = (poll_address_counter + 1) % 128;
    if (poll_address_counter == 0) {
        scan_address_counter = (scan_address_counter + 1) % 128;
        while (slave_present[scan_address_counter])
            scan_address_counter = (scan_address_counter + 1) % 128;
        poll_address = scan_address_counter;
        return;
    }
    poll_address = poll_address_counter;
}

void RS485Master::loop() {
    if (state == IDLE) {
        if (exportData.isNotEmpty()) {
            rxtx_len = exportData.getLength();
            noInterrupts(); s8();
            tx_byte(0);
            state = TX_ADDRESS_SENT;
            c8(); interrupts();
            set_udrie();
        } else if (messageBuffer.isEmpty() && !messageBuffer.complete) {
            advancePollAddress();
            noInterrupts(); s8();
            tx_byte(poll_address);
            state = POLL_ADDRESS_SENT;
            set_udrie();
            c8(); interrupts();
        } else {
            uart0.checkTimeout();
        }
    }
    if (state == RX_WAIT_DATALENGTH && ((micros() - rx_start_time) > 1000)) {
        slave_present[poll_address] = false;
        noInterrupts(); s8();
        tx_byte(0);
        state = TIMEOUT_ZEROBYTE_SENT;
        c8(); interrupts();
    }
    if ((state == RX_WAIT_MSGTYPE || state == RX_WAIT_DATA || state == RX_WAIT_CHECKSUM)
    && ((micros() - rx_start_time) > 5000)) {
        messageBuffer.clear();
        messageBuffer.put('\n');
        noInterrupts(); s8();
        messageBuffer.complete = true;
        uart0.set_udrie();
        state = IDLE;
        c8(); interrupts();
    }
}

void __attribute__((always_inline)) inline RS485Master::udreISR() {
    switch(state) {
        case POLL_ADDRESS_SENT:
            state = POLL_MSGTYPE_SENT;
            tx_byte(0x0);
        break;
        case POLL_MSGTYPE_SENT:
            state = POLL_DATALENGTH_SENT;
            tx_byte(0);
            clear_udrie();
        break;
        case TX_ADDRESS_SENT:
            state = TX_MSGTYPE_SENT;
            tx_byte(0);
        break;
        case TX_MSGTYPE_SENT:
            state = TX;
            tx_byte(rxtx_len);
        break;
        case TX:
            if (rxtx_len == 0) {
                tx_byte(checksum);
                state = TX_CHECKSUM_SENT;
                clear_udrie();
            } else {
                tx_byte(exportData.get());
                rxtx_len--;
            }
        break;
    }
}

void __attribute__((always_inline)) inline RS485Master::txcISR() {
    switch (state) {
        case POLL_DATALENGTH_SENT:
            rx_start_time = micros();
            state = RX_WAIT_DATALENGTH;
            clear_txen();
        break;
        case TIMEOUT_ZEROBYTE_SENT:
            state = IDLE;
            clear_txen();
        break;
        case TX_CHECKSUM_SENT:
            state = IDLE;
            clear_txen();
        break;
    }
}

void __attribute__((always_inline)) inline RS485Master::rxISR() {
    volatile uint8_t data = *udr;
    switch(state) {
        case RX_WAIT_DATALENGTH:
            rxtx_len = data;
            slave_present[poll_address] = true;
            if (rxtx_len > 0) {
                state = RX_WAIT_MSGTYPE;
            } else {
                state = IDLE;
            }
        break;
        case RX_WAIT_MSGTYPE:
            rx_msgtype = data;
            state = RX_WAIT_DATA;
        break;
        case RX_WAIT_DATA:
            messageBuffer.put(data);
            rxtx_len--;
            uart0.set_udrie();
            if (rxtx_len == 0) {
                state = RX_WAIT_CHECKSUM;
            }
        break;
        case RX_WAIT_CHECKSUM:
            messageBuffer.complete = true;
            uart0.set_udrie();
            state = IDLE;
        break;
    }
}

// Bus instances
#ifdef UART1_TXENABLE_PIN
RS485Master uart1(&UDR1, &UCSR1A, &UCSR1B, &UCSR1C, UART1_TXENABLE_PIN);
#endif
#ifdef UART2_TXENABLE_PIN
RS485Master uart2(&UDR2, &UCSR2A, &UCSR2B, &UCSR2C, UART2_TXENABLE_PIN);
#endif
#ifdef UART3_TXENABLE_PIN
RS485Master uart3(&UDR3, &UCSR3A, &UCSR3B, &UCSR3C, UART3_TXENABLE_PIN);
#endif

DcsBios::RingBuffer<32>* pc_tx_buffer = NULL;

MasterPCConnection::MasterPCConnection(volatile uint8_t *udr, volatile uint8_t *ucsra, volatile uint8_t *ucsrb, volatile uint8_t *ucsrc)
: udr(udr), ucsra(ucsra), ucsrb(ucsrb), ucsrc(ucsrc) {
}

void __attribute__((always_inline)) inline MasterPCConnection::advanceTxBuffer() {
    RS485Master* start = next_tx_rs485_master;
    while(1) {
        next_tx_rs485_master = next_tx_rs485_master->nextRS485Master;
        if (next_tx_rs485_master->messageBuffer.isEmpty() && next_tx_rs485_master->messageBuffer.complete) {
            next_tx_rs485_master->messageBuffer.complete = false;
        }
        if (next_tx_rs485_master->messageBuffer.isNotEmpty()) {
            pc_tx_buffer = &(next_tx_rs485_master->messageBuffer);
            tx_start_time = micros();
            return;
        }
        if (next_tx_rs485_master == start) break;
    }
}

void MasterPCConnection::checkTimeout() {
    noInterrupts(); s8();
    volatile unsigned long _tx_start_time = tx_start_time;
    c8(); interrupts();
    if (pc_tx_buffer && (micros() - _tx_start_time > 5000)) {
        pc_tx_buffer->clear();
        pc_tx_buffer->put('\n');
        pc_tx_buffer->complete = true;
        set_udrie();
    }
}

void __attribute__((always_inline)) inline MasterPCConnection::udreISR() {
    if (pc_tx_buffer == NULL) advanceTxBuffer();
    if (pc_tx_buffer->isEmpty()) {
        if (pc_tx_buffer->complete) {
            pc_tx_buffer->complete = false;
            advanceTxBuffer();
            if (pc_tx_buffer == NULL) clear_udrie();
            return;
        } else {
            clear_udrie();
            return;
        }
    }
    *ucsra |= (1<<TXC0);
    *udr = pc_tx_buffer->get();
}

// ============================================================================
// PC RX ISR — with LED hook
// ============================================================================
// This is the only function that differs from the official library.
// The last line feeds each PC byte into our LED protocol parser.

void __attribute__((always_inline)) inline MasterPCConnection::rxISR() {
    volatile uint8_t c = *udr;
    #ifdef UART1_TXENABLE_PIN
    uart1.exportData.put(c);
    #endif
    #ifdef UART2_TXENABLE_PIN
    uart2.exportData.put(c);
    #endif
    #ifdef UART3_TXENABLE_PIN
    uart3.exportData.put(c);
    #endif
    ledParserProcessByte(c);  // <-- LED sync hook
}

// ============================================================================
// ISR VECTORS
// ============================================================================

ISR(USART0_UDRE_vect) { s8(); uart0.udreISR(); c8(); }
ISR(USART0_RX_vect) { s8(); s9(); uart0.rxISR(); c9(); c8(); }

#ifdef UART1_TXENABLE_PIN
ISR(USART1_UDRE_vect) { s8(); uart1.udreISR(); c8(); }
ISR(USART1_TX_vect) { s8(); uart1.txcISR(); c8(); }
ISR(USART1_RX_vect) { s8(); uart1.rxISR(); c8(); }
#endif

#ifdef UART2_TXENABLE_PIN
ISR(USART2_UDRE_vect) { s8(); uart2.udreISR(); c8(); }
ISR(USART2_TX_vect) { s8(); uart2.txcISR(); c8(); }
ISR(USART2_RX_vect) { s8(); uart2.rxISR(); c8(); }
#endif

#ifdef UART3_TXENABLE_PIN
ISR(USART3_UDRE_vect) { s8(); uart3.udreISR(); c8(); }
ISR(USART3_TX_vect) { s8(); uart3.txcISR(); c8(); }
ISR(USART3_RX_vect) { s8(); uart3.rxISR(); c8(); }
#endif

} // namespace DcsBios

// ============================================================================
// ARDUINO SETUP & LOOP
// ============================================================================

void setup() {
    // LED pins
    pinMode(LED_OUTPUT_PIN, OUTPUT);
    digitalWrite(LED_OUTPUT_PIN, LOW);
    pinMode(SYNC_LED_PIN, OUTPUT);
    digitalWrite(SYNC_LED_PIN, LOW);

    // Debug scope pins
    pinMode(8, OUTPUT);
    digitalWrite(8, 0);
    pinMode(9, OUTPUT);
    digitalWrite(9, 0);

    // Quick LED test - flash output and SYNC LEDs 3 times
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_OUTPUT_PIN, HIGH);
        digitalWrite(SYNC_LED_PIN, HIGH);
        delay(150);
        digitalWrite(LED_OUTPUT_PIN, LOW);
        digitalWrite(SYNC_LED_PIN, LOW);
        delay(150);
    }

    // RS485 bus (UART1)
    #ifdef UART1_TXENABLE_PIN
    pinMode(UART1_TXENABLE_PIN, OUTPUT);
    PRR1 &= ~(1<<PRUSART1);
    UBRR1H = 0;
    UBRR1L = 3; // 250000 bps
    UCSR1A = 0;
    UCSR1C = (1<<UCSZ00) | (1<<UCSZ01);
    UCSR1B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
    #endif

    #ifdef UART2_TXENABLE_PIN
    pinMode(UART2_TXENABLE_PIN, OUTPUT);
    PRR1 &= ~(1<<PRUSART2);
    UBRR2H = 0;
    UBRR2L = 3;
    UCSR2A = 0;
    UCSR2C = (1<<UCSZ00) | (1<<UCSZ01);
    UCSR2B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
    #endif

    #ifdef UART3_TXENABLE_PIN
    pinMode(UART3_TXENABLE_PIN, OUTPUT);
    PRR1 &= ~(1<<PRUSART3);
    UBRR3H = 0;
    UBRR3L = 3;
    UCSR3A = 0;
    UCSR3C = (1<<UCSZ00) | (1<<UCSZ01);
    UCSR3B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0) | (1<<TXCIE0);
    #endif

    // UART0 — PC connection
    PRR0 &= ~(1<<PRUSART0);
    UBRR0H = 0;
    UBRR0L = 3; // 250000 bps
    UCSR0A = 0;
    UCSR0C = (1<<UCSZ00) | (1<<UCSZ01);
    UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
}

void loop() {
    #ifdef UART1_TXENABLE_PIN
    DcsBios::uart1.loop();
    #endif
    #ifdef UART2_TXENABLE_PIN
    DcsBios::uart2.loop();
    #endif
    #ifdef UART3_TXENABLE_PIN
    DcsBios::uart3.loop();
    #endif
}
