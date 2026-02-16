// CUtils.h

#pragma once

#include "../../../Config.h"

// Dump descriptors
void dump_usb_descriptors();

// To print pending messages
void sendPendingSerial();

// Helper for hash lookups
constexpr uint16_t labelHash(const char* s) {
    return *s ? static_cast<uint16_t>(*s) + 31 * labelHash(s + 1) : 0;
}

// Struct for Our custom Display String Buffers and hash entry
struct DisplayBufferEntry {
    const char* label;
    char* buffer;
    uint8_t length;
    bool* dirty;
    char* last;
};
struct DisplayBufferHashEntry {
    const char* label;
    const DisplayBufferEntry* entry;
};

// —— panel discovery types ——
#define MAX_DEVICES 10
struct I2CDeviceInfo { uint8_t address; const char* label; };
extern I2CDeviceInfo  discoveredDevices[MAX_DEVICES];
extern uint8_t        discoveredDeviceCount;

// direct-index lookup table size
static constexpr uint8_t I2C_ADDR_SPACE = 0x80;
extern const char* panelNameByAddr[I2C_ADDR_SPACE];

// scan & query
void        scanConnectedPanels();
void        printDiscoveredPanels();
bool        panelExists(uint8_t targetAddr);

// —— GPIO (discrete LEDs) —— 

// Main API: These are called by LEDControl.cpp and your panel logic
void GPIO_setAllLEDs(bool state);
void GPIO_setDigital(uint8_t pin, bool activeHigh, bool state);
void GPIO_setAnalog(uint8_t pin, bool activeLow, uint8_t intensity);
void GPIO_offAnalog(uint8_t pin, bool activeLow);
void resetAllGauges();
void GPIO_setAllLEDs(bool state);  
void preconfigureGPIO();

// —— WS2812 —— (Lockshoot + AoA Indexer)
// For compatibility
#define WS2812MINI_EXPORT_LEGACY_GLOBALS 1
#define NUM_LEDS 6 // Leave this for legacy compatibility (old driver)
#include "WS2812.h"

// —— TM1637 (4-digit displays + buttons) —— 
struct TM1637Device {
    uint8_t clkPin;
    uint8_t dioPin;
    uint8_t ledData[6];
    bool    needsUpdate;
    uint8_t dirtyGrids;   // bitmask: bit i = grid i changed since last flush
};

uint8_t       TM1637_getDeviceCount();
TM1637Device* TM1637_getDeviceAt(uint8_t index);

uint8_t tm1637_readKeys(TM1637Device&);
void    tm1637_start(TM1637Device&);
void    tm1637_stop(TM1637Device&);
bool    tm1637_writeByte(TM1637Device&, uint8_t);
void    tm1637_updateDisplay(TM1637Device&);
void    tm1637_init(TM1637Device&, uint8_t clk, uint8_t dio);
void    tm1637_displaySingleLED(TM1637Device&, uint8_t grid, uint8_t seg, bool on);
void    tm1637_clearDisplay(TM1637Device&);

// global helpers (operate on all registered TM1637 devices)
void    tm1637_allOn();
void    tm1637_allOff();
void    tm1637_tick();

// registry API
TM1637Device* TM1637_getOrCreate(uint8_t clkPin, uint8_t dioPin);
TM1637Device* TM1637_findByPins(uint8_t clkPin, uint8_t dioPin);
TM1637Device* TM1637_findByDIO(uint8_t dioPin);


// —— GN1640 (matrix LED driver) —— 
void GN1640_startCondition();
void GN1640_stopCondition();
void GN1640_sendByte(uint8_t data);
void GN1640_command(uint8_t cmd);
void GN1640_init(uint8_t clkPin, uint8_t dioPin);
void GN1640_setLED(uint8_t row, uint8_t col, bool state);
void GN1640_write(uint8_t column, uint8_t value);
void GN1640_clearAll();
void GN1640_allOff();
void GN1640_allOn();
void GN1640_sweepPanel();
void GN1640_testPattern();
void GN1640_setAllLEDs(bool state);
bool GN1640_detect(uint8_t clkPin, uint8_t dioPin);  
void GN1640_tick();

// —— PCA9555 (I²C expander) —— 
extern uint8_t PCA9555_cachedPortStates[8][2];  // address 0x20–0x27 → [port0,port1]
void PCA9555_setAllLEDs(bool state);
void PCA9555_allLEDsByAddress(uint8_t addr, bool state);
void PCA9555_allOn(uint8_t addr);
void PCA9555_allOff(uint8_t addr);
void PCA9555_sweep(uint8_t addr);
void PCA9555_patternTesting(uint8_t addr);
void PCA9555_autoInitFromLEDMap(uint8_t addr);
void PCA9555_initAsOutput(uint8_t addr, uint8_t mask0, uint8_t mask1);
void initPCA9555AsInput(uint8_t addr);
bool readPCA9555(uint8_t addr, byte &p0, byte &p1);
void PCA9555_write(uint8_t addr, uint8_t port, uint8_t bit, bool state);
bool isPCA9555LoggingEnabled();
void enablePCA9555Logging(bool);
void logExpanderState(uint8_t p0, uint8_t p1);
void logPCA9555State(uint8_t addr, byte p0, byte p1);
void PCA9555_initCache();
void measureI2Cspeed(uint8_t deviceAddr);
bool shouldLogChange(uint8_t address, byte port0, byte port1);
void PCA9555_scanConnectedPanels();

// —— MatrixRotary Interface (shared across panels) ——
uint8_t MatrixRotary_readPattern(const int* strobes, int count, int dataPin);

// —— 74HC165 Shift Register Input Reader Implementation ——
void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH, uint8_t numBits);
uint64_t HC165_read();
void HC165_printBits(const char* prefix, uint64_t value, uint8_t numBits);
void HC165_printBitChanges(uint64_t prev, uint64_t curr, uint8_t numBits);
// void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH, uint8_t numBits);  // new
// uint64_t HC165_read();  // always reads as many bits as configured

// —— Analog Gauge Implementation ——
extern bool hasGauge; // If we find ONE we run the 20 ms ticker 
#define MAX_GAUGES 8
void AnalogG_initPin(uint8_t pin);
void AnalogG_pulseUs(uint8_t pin, int minPulseUs, int maxPulseUs, uint16_t value);
typedef struct { uint8_t  pin; int minPulseUs; int maxPulseUs; volatile uint16_t value; } GaugeState;
void AnalogG_registerGauge(uint8_t pin, int minPulseUs, int maxPulseUs);
void AnalogG_set(uint8_t pin, uint16_t value);  // Set desired value for a given pin
void AnalogG_tick();                            // Call every 20ms, or from a task
extern GaugeState gaugeArray[MAX_GAUGES];
extern uint8_t gaugeCount;

// —— Meta & Debug helpers —— 
void setPanelAllLEDs(const char* panelPrefix, bool state);
void setAllPanelsLEDs(bool state);
void detectAllPanels(bool &hasLockShoot, bool &hasCA, bool &hasLA,
                     bool &hasRA, bool &hasIR, bool &hasECM, bool &hasMasterARM);
void printLEDMenu();
void handleLEDSelection();
void runReplayWithPrompt();

// ---- Universal Display and LED Types ----
struct SegmentMap {
    uint8_t addr;
    uint8_t bit;
    uint8_t ledID;
};

static inline int strToIntFast(const char* s) {
    int v = 0;
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s++ - '0'); }
    return v;
}

// ---- HT1622 Display Panel Handler ----
#define HT1622_RAM_SIZE 64
class HT1622 {
public:
    HT1622(uint8_t cs, uint8_t wr, uint8_t data);

    void init();
    void sendCmd(uint8_t cmd);
    void writeNibble(uint8_t addr, uint8_t nibble);

    void commitBurstRMT(const uint8_t* shadow);
    void commitBurst(const uint8_t* shadow);
    void commit(const uint8_t* shadow, uint8_t* lastShadow, int shadowLen);
    void commitPartial(const uint8_t* shadow, uint8_t* lastShadow, uint8_t addrStart, uint8_t addrEnd);

    void clear();
    void allSegmentsOn();
    void allSegmentsOff();

    void invalidateLastShadow(uint8_t* lastShadow, int shadowLen);

private:
    uint8_t _cs, _wr, _data;

    void setCS(bool level);
    void setWR(bool level);
    void setDATA(bool level);

    void writeBitStrict(bool bit);
    void writeCommandBits(uint16_t bits, uint8_t len);
};

// ============================================================================
//  UTILITY FUNCTIONS (Minimal, pure helpers)
// ============================================================================
bool startsWith(const char* s, const char* pfx);
uint8_t hexNib(char c);
uint8_t parseHexByte(const char* s);

// ============================================================================
// RS-485 MASTER MODE
// ============================================================================
#if RS485_MASTER_ENABLED
bool RS485Master_init();
void RS485Master_loop();
void RS485Master_stop();
void RS485Master_feedExportData(const uint8_t* data, size_t len);
void RS485Master_forceFullSync();
bool RS485Master_isSlaveOnline(uint8_t address);
uint8_t RS485Master_getOnlineSlaveCount();
void RS485Master_printStatus();
#endif

// ============================================================================
// RS-485 SLAVE MODE
// ============================================================================
#if RS485_SLAVE_ENABLED
bool RS485Slave_init();
void RS485Slave_loop();
void RS485Slave_stop();
bool RS485Slave_queueCommand(const char* label, const char* value);
uint32_t RS485Slave_getPollCount();
uint32_t RS485Slave_getBroadcastCount();
uint32_t RS485Slave_getExportBytesReceived();
uint32_t RS485Slave_getCommandsSent();
size_t RS485Slave_getTxBufferPending();
uint32_t RS485Slave_getTimeSinceLastPoll();
void RS485Slave_printStatus();
#endif