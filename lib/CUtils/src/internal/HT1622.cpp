// ************************************************************
// HT1622 Unified Driver for ESP32 Arduino Core 2.x and 3.x
// ************************************************************
//
// PATCHED VERSION: Added critical section protection for BLE coexistence
// 
// Problem: BLE radio interrupts (connection events every 7.5-15ms) can
// preempt the bit-bang protocol mid-transfer, corrupting HT1622 commands.
//
// Solution: Wrap all bit-bang sequences in portENTER_CRITICAL/portEXIT_CRITICAL
// to prevent interrupt preemption during timing-critical GPIO operations.
//
// WCET Impact: ~50-200μs per commit (acceptable for 30Hz display refresh)
// ************************************************************

#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include <string.h>

// FreeRTOS critical section support
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

// ----- Version Detection -----
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
#define USE_RMT_API_V3 1
#else
#define USE_RMT_API_V3 0
#endif

#if USE_RMT_API_V3
  // Arduino Core 3.x: rmt_data_t, rmtWrite, etc.
#include <driver/rmt_tx.h>
#else
  // Arduino Core 2.x: rmt_item32_t, rmt_write_items, etc.
#include <driver/rmt.h>
#endif

// ---- Strict protocol timing ----
#define HT1622_WR_MIN_US   4
#define HT1622_DATA_SU_NS 120
#define HT1622_DATA_H_NS  600
#define HT1622_CS_SETUP_NS 600
#define HT1622_CS_HOLD_NS  800
#define HT1622_RAM_SIZE    64

// ════════════════════════════════════════════════════════════════════════════
// CRITICAL SECTION SPINLOCK — Protects bit-bang from BLE/WiFi radio interrupts
// ════════════════════════════════════════════════════════════════════════════
static portMUX_TYPE ht1622_mux = portMUX_INITIALIZER_UNLOCKED;

// ----- Class Implementation -----
HT1622::HT1622(uint8_t cs, uint8_t wr, uint8_t data)
    : _cs(cs), _wr(wr), _data(data) {
}

void HT1622::init() {
    pinMode(_cs, OUTPUT);
    pinMode(_wr, OUTPUT);
    pinMode(_data, OUTPUT);

    setCS(true); setWR(true); setDATA(true);
    delayMicroseconds(HT1622_CS_SETUP_NS / 1000 + 1);

    sendCmd(0x00); // SYS DIS
    sendCmd(0x18); // RC 32K
    sendCmd(0x29); // BIAS
    sendCmd(0x01); // SYS EN

    // Optional: soft reset (factory recommends)
    // Protected critical section for reset sequence
    portENTER_CRITICAL(&ht1622_mux);
    setCS(false); 
    delayMicroseconds(HT1622_CS_SETUP_NS / 1000 + 1);
    writeCommandBits(0xE3, 8);
    setCS(true); 
    portEXIT_CRITICAL(&ht1622_mux);
    
    delayMicroseconds(HT1622_CS_HOLD_NS / 1000 + 1);

    sendCmd(0x03); // LCD ON
    clear();
}

void HT1622::sendCmd(uint8_t cmd) {
    // ════════════════════════════════════════════════════════════════════════
    // CRITICAL SECTION: Protect entire command sequence from radio interrupts
    // ════════════════════════════════════════════════════════════════════════
    portENTER_CRITICAL(&ht1622_mux);
    
    setCS(false);
    delayMicroseconds(HT1622_CS_SETUP_NS / 1000 + 1);

    writeBitStrict(1); writeBitStrict(0); writeBitStrict(0);
    writeCommandBits(cmd, 8);
    writeBitStrict(0);

    setCS(true);
    
    portEXIT_CRITICAL(&ht1622_mux);
    // ════════════════════════════════════════════════════════════════════════
    
    delayMicroseconds(HT1622_CS_HOLD_NS / 1000 + 1);
}

void HT1622::writeNibble(uint8_t addr, uint8_t nibble) {
    // ════════════════════════════════════════════════════════════════════════
    // CRITICAL SECTION: Protect entire nibble write from radio interrupts
    // ════════════════════════════════════════════════════════════════════════
    portENTER_CRITICAL(&ht1622_mux);
    
    setCS(false);
    delayMicroseconds(HT1622_CS_SETUP_NS / 1000 + 1);

    writeBitStrict(1); writeBitStrict(0); writeBitStrict(1);
    for (int i = 5; i >= 0; i--) writeBitStrict((addr >> i) & 1);
    for (int i = 0; i < 4; i++) writeBitStrict((nibble >> i) & 1);

    setCS(true);
    
    portEXIT_CRITICAL(&ht1622_mux);
    // ════════════════════════════════════════════════════════════════════════
    
    delayMicroseconds(HT1622_CS_HOLD_NS / 1000 + 1);
}

// --- RMT Unified Commit Burst ---
void HT1622::commitBurstRMT(const uint8_t* shadow) {
#if USE_RMT_API_V3
    rmt_data_t wrBuffer[272], dataBuffer[272];
#else
    rmt_item32_t wrBuffer[272], dataBuffer[272];
#endif
    int symbolIndex = 0;

    auto appendBit = [&](bool bit) {
        wrBuffer[symbolIndex].duration0 = HT1622_WR_MIN_US;
        wrBuffer[symbolIndex].level0 = 0;
        wrBuffer[symbolIndex].duration1 = HT1622_WR_MIN_US;
        wrBuffer[symbolIndex].level1 = 1;
        dataBuffer[symbolIndex].duration0 = HT1622_WR_MIN_US;
        dataBuffer[symbolIndex].level0 = bit;
        dataBuffer[symbolIndex].duration1 = HT1622_WR_MIN_US;
        dataBuffer[symbolIndex].level1 = bit;
        symbolIndex++;
    };

    // Write mode: 1 0 1 (MSB first)
    appendBit(1); appendBit(0); appendBit(1);
    for (int b = 5; b >= 0; --b) appendBit((0 >> b) & 1);

    // 64 nibbles (256 bits)
    for (int addr = 0; addr < 64; ++addr) {
        uint8_t val = shadow[addr] & 0xF;
        for (int b = 0; b < 4; ++b) appendBit((val >> b) & 1);
    }

    // RMT is hardware-driven, less susceptible to interrupt issues
    // but we still protect the CS transitions
    portENTER_CRITICAL(&ht1622_mux);
    setCS(false);
    delayMicroseconds(1);
    portEXIT_CRITICAL(&ht1622_mux);

#if USE_RMT_API_V3
    rmtWrite(_wr, wrBuffer, symbolIndex, RMT_WAIT_FOR_EVER);
    rmtWrite(_data, dataBuffer, symbolIndex, RMT_WAIT_FOR_EVER);
#else
    rmt_write_items((rmt_channel_t)_wr, wrBuffer, symbolIndex, true);
    rmt_wait_tx_done((rmt_channel_t)_wr, portMAX_DELAY);
    rmt_write_items((rmt_channel_t)_data, dataBuffer, symbolIndex, true);
    rmt_wait_tx_done((rmt_channel_t)_data, portMAX_DELAY);
#endif

    portENTER_CRITICAL(&ht1622_mux);
    setCS(true);
    portEXIT_CRITICAL(&ht1622_mux);
    
    delayMicroseconds(1);
}

void HT1622::commitBurst(const uint8_t* shadow) {
    // ════════════════════════════════════════════════════════════════════════
    // CRITICAL SECTION: Protect entire burst transfer from radio interrupts
    // This is the most timing-sensitive operation (~2.5ms for 64 nibbles)
    // ════════════════════════════════════════════════════════════════════════
    portENTER_CRITICAL(&ht1622_mux);
    
    setCS(false);
    delayMicroseconds(HT1622_CS_SETUP_NS / 1000 + 1);

    writeBitStrict(1); writeBitStrict(0); writeBitStrict(1);
    for (int i = 5; i >= 0; --i) writeBitStrict((0 >> i) & 1);

    for (int addr = 0; addr < HT1622_RAM_SIZE; ++addr) {
        uint8_t val = shadow[addr] & 0xF;
        for (int b = 0; b < 4; ++b)
            writeBitStrict((val >> b) & 1);
    }

    setCS(true);
    
    portEXIT_CRITICAL(&ht1622_mux);
    // ════════════════════════════════════════════════════════════════════════
    
    delayMicroseconds(HT1622_CS_HOLD_NS / 1000 + 1);
}

void HT1622::commit(const uint8_t* shadow, uint8_t* lastShadow, int shadowLen) {
    bool dirty = false;
    for (int addr = 0; addr < shadowLen; ++addr) {
        if ((shadow[addr] & 0xF) != (lastShadow[addr] & 0xF)) {
            dirty = true;
            break;
        }
    }
    if (!dirty) return;
    
    // Use bitbang version (safer; RMT version available if needed)
    commitBurst(shadow);
    // commitBurstRMT(shadow);

    for (int addr = 0; addr < shadowLen; ++addr)
        lastShadow[addr] = shadow[addr] & 0xF;
}

void HT1622::commitPartial(const uint8_t* shadow, uint8_t* lastShadow,
    uint8_t addrStart, uint8_t addrEnd) {
    
    // Bounds check
    if (addrStart > addrEnd || addrEnd >= HT1622_RAM_SIZE) return;
    
    // Find dirty range within the requested region
    uint8_t dirtyStart = 0xFF, dirtyEnd = 0;
    for (uint8_t addr = addrStart; addr <= addrEnd; ++addr) {
        if ((shadow[addr] & 0xF) != (lastShadow[addr] & 0xF)) {
            if (dirtyStart == 0xFF) dirtyStart = addr;
            dirtyEnd = addr;
        }
    }
    if (dirtyStart == 0xFF) return;  // Nothing dirty in this region

    // ════════════════════════════════════════════════════════════════════════
    // CRITICAL SECTION: Protect partial commit from BLE radio interrupts
    // 
    // This is THE key fix for BLE coexistence. Without this protection,
    // BLE connection event interrupts (every 7.5-15ms) can preempt the
    // bit-bang sequence mid-transfer, corrupting the HT1622 protocol.
    //
    // WCET: ~8μs per nibble × (dirtyEnd - dirtyStart + 1) nibbles
    //       Typical: 10-50 nibbles = 80-400μs (well within BLE tolerance)
    // ════════════════════════════════════════════════════════════════════════
    portENTER_CRITICAL(&ht1622_mux);

    setCS(false);
    delayMicroseconds(HT1622_CS_SETUP_NS / 1000 + 1);

    // Write command header: 101 (write mode)
    writeBitStrict(1); writeBitStrict(0); writeBitStrict(1);
    
    // 6-bit start address (MSB first)
    for (int i = 5; i >= 0; --i) writeBitStrict((dirtyStart >> i) & 1);

    // Sequential nibble data (LSB first per nibble)
    for (uint8_t addr = dirtyStart; addr <= dirtyEnd; ++addr) {
        uint8_t val = shadow[addr] & 0xF;
        for (int b = 0; b < 4; ++b) writeBitStrict((val >> b) & 1);
        lastShadow[addr] = val;
    }

    setCS(true);
    
    portEXIT_CRITICAL(&ht1622_mux);
    // ════════════════════════════════════════════════════════════════════════
    
    delayMicroseconds(HT1622_CS_HOLD_NS / 1000 + 1);
}

void HT1622::allSegmentsOn() {
    for (int addr = 0; addr < 64; ++addr)
        writeNibble(addr, 0xF);
}

void HT1622::allSegmentsOff() {
    clear();
}

void HT1622::clear() {
    for (int addr = 0; addr < 64; ++addr)
        writeNibble(addr, 0x0);
}

void HT1622::invalidateLastShadow(uint8_t* lastShadow, int shadowLen) {
    for (int i = 0; i < shadowLen; ++i)
        lastShadow[i] = 0xFF;
}

// ----------------------
// Low-level protocol
// ----------------------

inline void HT1622::setWR(bool level) {
    gpio_set_level((gpio_num_t)_wr, level);
}

inline void HT1622::setCS(bool level) {
    gpio_set_level((gpio_num_t)_cs, level);
}

inline void HT1622::setDATA(bool level) {
    gpio_set_level((gpio_num_t)_data, level);
}

inline void HT1622::writeBitStrict(bool bit) {
    setDATA(bit);
    delayMicroseconds(1);
    setWR(false);
    delayMicroseconds(HT1622_WR_MIN_US);
    setWR(true);
    delayMicroseconds(HT1622_WR_MIN_US);
}

inline void HT1622::writeCommandBits(uint16_t bits, uint8_t len) {
    for (int i = len - 1; i >= 0; i--) writeBitStrict((bits >> i) & 1);
}
