# CockpitOS / OpenHornet Integration Technical Reference

## Hardware Ecosystem Comparison

### OpenHornet I/O Components

| Component | Type | OH Usage | CockpitOS Status |
|-----------|------|----------|------------------|
| MCP23017 | 16-bit I2C GPIO | ABSIS_MPC (8x = 128 GPIO) | **NOT SUPPORTED** - needs driver |
| PCA9555 | 16-bit I2C GPIO | UFC, DDI, AMPCD | **SUPPORTED** |
| PCA9548A | I2C multiplexer | Standby Altimeter | **NOT SUPPORTED** - needs driver |
| PCA9685 | 16-ch PWM (I2C) | Servos, backlighting | **NOT SUPPORTED** - needs driver |
| MAX7219 | 8-digit LED (SPI) | EWI, Caution Lights | **NOT SUPPORTED** - needs driver |
| WS2812B | Addressable RGB | IFEI display | **SUPPORTED** |
| L298N | H-Bridge | Motor control | **NOT SUPPORTED** |
| X40_8798 | Microstepper gauge | Dual-needle instruments | **NOT SUPPORTED** |
| Servos | PWM | Various | **PARTIAL** - servo gauge exists |
| Bipolar steppers | 4-wire | Gauge needles | **NOT SUPPORTED** |

### CockpitOS Components NOT in OpenHornet

| Component | CockpitOS Support | Notes |
|-----------|-------------------|-------|
| 74HC165 | Shift register input | OH uses I2C expanders instead |
| TM1637 | 6-digit 7-seg + buttons | OH uses MAX7219 |
| GN1640T | LED matrix | Not used in OH |
| HT1622 | Segment LCD | Not used in OH |

---

## Architecture Integration Paths

### Path A: CockpitOS as OH Slave Replacement

Replace existing OH Arduino slaves with ESP32 running CockpitOS.

**Scope:** Individual panel boards (UFC, DDI, AMPCD, IFEI, Caution Panel, etc.)

**Requirements:**
1. Add MCP23017 driver (for ABSIS_MPC compatibility)
2. Add MAX7219 driver (for EWI/Caution panels)
3. RS-485 interface to OH master

**RS-485 Slave Protocol Integration:**
```
OH Master (existing) <--RS-485--> CockpitOS Slave (new)
                                      |
                                      +-- Local I2C bus (PCA9555, MCP23017, PCA9685)
                                      +-- Local SPI bus (MAX7219)
                                      +-- Local WS2812B
                                      +-- Local GPIO
```

**Advantages:**
- Drop-in replacement for existing OH slave boards
- Preserves OH master architecture
- Per-panel ESP32 provides local processing power

**Work Required:**
- RS-485 slave protocol parser/responder
- I2C command relay from RS-485 to local bus
- New Label Sets per OH panel type

---

### Path B: CockpitOS as OH Master Replacement

Replace OH master with ESP32 running CockpitOS, keeping existing slave hardware.

**Scope:** Central controller communicating with existing OH slave boards

**Requirements:**
1. RS-485 master protocol implementation
2. I2C command forwarding to slaves
3. DCS-BIOS bridge (already exists)

**Architecture:**
```
DCS World <--USB/WiFi/Serial--> CockpitOS Master <--RS-485--> OH Slave 1
                                                          +-> OH Slave 2
                                                          +-> OH Slave N
```

**Advantages:**
- Unified firmware across cockpit
- CockpitOS DCS-BIOS parsing handles protocol complexity
- Single point of configuration

**Work Required:**
- RS-485 master bus arbitration
- Slave address management
- Command routing layer

---

### Path C: Full CockpitOS Native (No RS-485)

Direct ESP32 per panel, each with native I2C/SPI peripherals, USB/WiFi to DCS.

**Scope:** Complete cockpit rebuild using CockpitOS architecture

**Architecture:**
```
DCS World <--USB Hub--> ESP32 Panel 1 (own I2C bus)
                    +-> ESP32 Panel 2 (own I2C bus)
                    +-> ESP32 Panel N (own I2C bus)
```

**Advantages:**
- Simplest wiring (no RS-485 bus)
- Each panel fully autonomous
- Uses existing CockpitOS Label Set paradigm

**Disadvantages:**
- Incompatible with existing OH PCBs
- Requires more ESP32 modules
- More USB endpoints or WiFi connections

---

## Required Driver Development

### Priority 1: MCP23017 (Critical for ABSIS_MPC)

**Similarity to PCA9555:** High - both are 16-bit I2C GPIO expanders

**Implementation approach:**
```cpp
// Proposed MCP23017 driver interface (mirrors PCA9555)
void MCP23017_init(uint8_t address);
uint16_t MCP23017_readAll(uint8_t address);
void MCP23017_writeAll(uint8_t address, uint16_t value);
void MCP23017_configureInput(uint8_t address, uint16_t mask);
void MCP23017_enablePullups(uint8_t address, uint16_t mask);
```

**Key differences from PCA9555:**
- Register addresses differ (IODIRA=0x00 vs PCA IODIR=0x06)
- MCP23017 has interrupt-on-change feature
- MCP23017 has internal pullups (GPPU register)

**File locations:**
- `lib/CUtils/src/MCP23017.h` (new)
- `lib/CUtils/src/MCP23017.cpp` (new)
- Update `InputManager.cpp` to scan MCP23017

**InputMapping.h integration:**
```cpp
{ "UFC_IP_BTN", "MCP_0x20", 0, 3, -1, "UFC_IP_BTN", 1, "momentary", 0 },
```

---

### Priority 2: MAX7219 (EWI/Caution Lights)

**Protocol:** SPI (not I2C)

**Implementation approach:**
```cpp
void MAX7219_init(uint8_t csPin);
void MAX7219_setDigit(uint8_t csPin, uint8_t digit, uint8_t value, bool dp);
void MAX7219_setRow(uint8_t csPin, uint8_t row, uint8_t pattern);
void MAX7219_setBrightness(uint8_t csPin, uint8_t level);  // 0-15
void MAX7219_clear(uint8_t csPin);
```

**Cascading support:** Multiple MAX7219 on same SPI bus with individual CS pins

**LEDMapping.h integration:**
```cpp
enum LEDDeviceType {
    // ... existing ...
    DEVICE_MAX7219,
};

struct LEDMapping {
    // ... existing union, add:
    struct { uint8_t csPin; uint8_t digit; uint8_t segment; } max7219Info;
};
```

---

### Priority 3: PCA9685 (PWM for Servos/Backlighting)

**Protocol:** I2C, 16 channels, 12-bit PWM

**Implementation approach:**
```cpp
void PCA9685_init(uint8_t address, uint16_t freq);  // freq typically 50Hz for servos
void PCA9685_setPWM(uint8_t address, uint8_t channel, uint16_t on, uint16_t off);
void PCA9685_setServo(uint8_t address, uint8_t channel, uint16_t pulseUs);
void PCA9685_setDuty(uint8_t address, uint8_t channel, uint8_t percent);
```

**LEDMapping.h integration for backlighting:**
```cpp
struct { uint8_t address; uint8_t channel; } pca9685Info;
```

**Gauge integration for servos:**
```cpp
// Extend existing DEVICE_GAUGE or add DEVICE_PCA9685_SERVO
{ "AOA_GAUGE", DEVICE_PCA9685_SERVO, {.pca9685Info = {0x40, 0, 1000, 2000}}, true, false },
```

---

### Priority 4: PCA9548A (I2C Multiplexer)

**Use case:** Standby Altimeter gauge (allows multiple devices with same I2C address)

**Implementation approach:**
```cpp
void PCA9548A_selectChannel(uint8_t muxAddress, uint8_t channel);  // 0-7
void PCA9548A_disableAll(uint8_t muxAddress);
uint8_t PCA9548A_getChannel(uint8_t muxAddress);
```

**Integration pattern:**
```cpp
// Before accessing device behind mux:
PCA9548A_selectChannel(0x70, 2);  // Select channel 2
PCA9555_readAll(0x20);            // Now 0x20 is the one on channel 2
```

---

### Priority 5: Stepper Motor Support (X40_8798 Gauges)

**Juken Swiss X40_8798:** Microstepper for aviation instrument needles

**Implementation approach:**
```cpp
struct StepperConfig {
    uint8_t pinA, pinB, pinC, pinD;  // 4-wire bipolar
    uint16_t stepsPerRev;
    uint16_t currentPos;
    uint16_t targetPos;
};

void Stepper_init(StepperConfig* cfg);
void Stepper_setTarget(StepperConfig* cfg, uint16_t position);
void Stepper_tick(StepperConfig* cfg);  // Call from loop, moves one step if needed
void Stepper_home(StepperConfig* cfg);  // Zero position
```

**Integration with gauges:**
```cpp
enum LEDDeviceType {
    // ... existing ...
    DEVICE_STEPPER,
};
```

---

## Label Set Configuration for OH Panels

### Example: OH IFEI Panel Label Set

```cpp
// LABEL_SET_OH_IFEI/InputMapping.h
static const InputMapping InputMappings[] = {
    // IFEI uses WS2812B for display - no button inputs from display
    // Physical buttons via local GPIO or I2C expander
    { "IFEI_QTY_BTN"  , "GPIO", PIN(4), 0, -1, "IFEI_QTY_BTN"  , 1, "momentary", 0 },
    { "IFEI_UP_BTN"   , "GPIO", PIN(5), 0, -1, "IFEI_UP_BTN"   , 1, "momentary", 0 },
    { "IFEI_DWN_BTN"  , "GPIO", PIN(6), 0, -1, "IFEI_DWN_BTN"  , 1, "momentary", 0 },
    { "IFEI_ZONE_BTN" , "GPIO", PIN(7), 0, -1, "IFEI_ZONE_BTN" , 1, "momentary", 0 },
    { "IFEI_ET_BTN"   , "GPIO", PIN(8), 0, -1, "IFEI_ET_BTN"   , 1, "momentary", 0 },
    { "IFEI_MODE_BTN" , "GPIO", PIN(9), 0, -1, "IFEI_MODE_BTN" , 1, "momentary", 0 },
    // Brightness potentiometer
    { "IFEI_BRT"      , "GPIO", PIN(3), 0, -1, "IFEI_BRT"      , 65535, "analog", 0 },
};
```

```cpp
// LABEL_SET_OH_IFEI/LEDMapping.h
// WS2812B-2020 addressable LEDs for IFEI display
static const LEDMapping panelLEDs[] = {
    // Dozens of WS2812B for IFEI segments - indices match OH IFEI PCB layout
    { "IFEI_BINGO_1", DEVICE_WS2812, {.ws2812Info = {0, WS2812B_PIN, 0, 255, 0, 200}}, true, false },
    { "IFEI_BINGO_2", DEVICE_WS2812, {.ws2812Info = {1, WS2812B_PIN, 0, 255, 0, 200}}, true, false },
    // ... continue for all IFEI segments
};
```

### Example: OH Caution/Advisory Panel Label Set

```cpp
// LABEL_SET_OH_CAUTION/LEDMapping.h
// Uses MAX7219 for indicator lights
static const LEDMapping panelLEDs[] = {
    { "CAUTION_FIRE_LEFT"  , DEVICE_MAX7219, {.max7219Info = {PIN(10), 0, 0}}, false, false },
    { "CAUTION_FIRE_APU"   , DEVICE_MAX7219, {.max7219Info = {PIN(10), 0, 1}}, false, false },
    { "CAUTION_FIRE_RIGHT" , DEVICE_MAX7219, {.max7219Info = {PIN(10), 0, 2}}, false, false },
    // ... continue for all caution indicators
};
```

### Example: OH UFC Panel Label Set (PCA9555 + MCP23017)

```cpp
// LABEL_SET_OH_UFC/InputMapping.h
static const InputMapping InputMappings[] = {
    // UFC Function Select buttons via PCA9555 (already supported)
    { "UFC_1"         , "PCA_0x20", 0, 0, -1, "UFC_1"         , 1, "momentary", 0 },
    { "UFC_2"         , "PCA_0x20", 0, 1, -1, "UFC_2"         , 1, "momentary", 0 },
    { "UFC_3"         , "PCA_0x20", 0, 2, -1, "UFC_3"         , 1, "momentary", 0 },
    // ... continue

    // Additional GPIO via MCP23017 (needs driver)
    { "UFC_ADF1"      , "MCP_0x21", 0, 0, -1, "UFC_ADF1"      , 1, "momentary", 0 },
    { "UFC_ADF2"      , "MCP_0x21", 0, 1, -1, "UFC_ADF2"      , 1, "momentary", 0 },
};
```

---

## RS-485 Protocol Integration

### OH RS-485 Bus Characteristics

- Half-duplex differential signaling
- Multi-drop topology (1 master, N slaves)
- Typical baud: 115200 or 250000
- Slave addressing scheme

### CockpitOS RS-485 Interface

**Hardware:** MAX485 or similar transceiver

**Pin assignments (CustomPins.h):**
```cpp
#define RS485_TX_PIN    PIN(17)
#define RS485_RX_PIN    PIN(16)
#define RS485_DE_PIN    PIN(4)   // Driver Enable (HIGH=transmit)
```

**Slave mode implementation:**
```cpp
void RS485_init(uint8_t slaveAddress);
void RS485_poll();  // Called from main loop
void RS485_onCommand(uint8_t cmd, uint8_t* data, uint8_t len);  // Callback
void RS485_respond(uint8_t* data, uint8_t len);
```

**Master mode implementation:**
```cpp
void RS485_initMaster();
bool RS485_sendToSlave(uint8_t addr, uint8_t cmd, uint8_t* data, uint8_t len);
bool RS485_readFromSlave(uint8_t addr, uint8_t cmd, uint8_t* buf, uint8_t maxLen);
void RS485_broadcastSync();  // Sync all slaves
```

---

## I2C Bus Topology Considerations

### OH Topology (Multiple Devices per Board)

```
ESP32 I2C Master
    |
    +-- PCA9555 @ 0x20 (buttons)
    +-- PCA9555 @ 0x21 (buttons)
    +-- MCP23017 @ 0x22 (GPIO)
    +-- PCA9685 @ 0x40 (PWM)
    +-- PCA9548A @ 0x70 (mux)
            |
            +-- Ch0: Device @ 0x20 (behind mux)
            +-- Ch1: Device @ 0x20 (behind mux)
```

### CockpitOS I2C Scanning Enhancement

Current `PCA9555_scan()` only finds PCA9555. Need unified scanner:

```cpp
struct I2CDeviceInfo {
    uint8_t address;
    enum { DEV_PCA9555, DEV_MCP23017, DEV_PCA9685, DEV_PCA9548A, DEV_UNKNOWN } type;
};

void I2C_scanBus(I2CDeviceInfo* devices, uint8_t maxDevices);
```

---

## Display System Mapping

### OH IFEI WS2812B Layout

The OH IFEI uses WS2812B-2020 arranged as pseudo-7-segment displays.

**Segment mapping approach:**
```cpp
// IFEI_SegmentMap.h for OH IFEI
// Maps logical segments to WS2812B indices
static const SegmentMap IFEI_BINGO_MAP = {
    .digitCount = 5,
    .segsPerDigit = 7,
    .indices = { 0, 1, 2, 3, 4, 5, 6,   // Digit 0
                 7, 8, 9, 10, 11, 12, 13, // Digit 1
                 // ...
               }
};
```

**DisplayMapping.cpp integration:**
```cpp
const DisplayFieldDefLabel fieldDefs[] = {
    { "IFEI_BINGO", &IFEI_BINGO_MAP, 5, 7, 0, 99999, FIELD_NUMERIC, 0,
      &ifei, DISPLAY_IFEI, renderIFEIDispatcher, clearIFEIDispatcher, FIELD_RENDER_BINGO },
    // ... other IFEI fields
};
```

---

## Config.h Additions for OH Support

```cpp
// OpenHornet hardware enables
#define ENABLE_MCP23017       0  // MCP23017 I2C GPIO expander
#define ENABLE_MAX7219        0  // MAX7219 SPI LED driver
#define ENABLE_PCA9685        0  // PCA9685 I2C PWM driver
#define ENABLE_PCA9548A       0  // PCA9548A I2C multiplexer
#define ENABLE_STEPPER        0  // Stepper motor support
#define ENABLE_RS485          0  // RS-485 bus support

// RS-485 configuration
#define RS485_BAUD            250000
#define RS485_SLAVE_ADDR      0x01  // This device's address (slave mode)

// MCP23017 polling
#define MCP23017_POLL_MS      4  // Same as PCA9555
```

---

## Migration Strategy

### Phase 1: Single Panel Proof-of-Concept

1. Select one OH panel (recommend: IFEI or Caution)
2. Implement required drivers (WS2812B already done, add MAX7219 if needed)
3. Create Label Set matching OH panel layout
4. Test with standalone ESP32 + direct DCS-BIOS (no RS-485)

### Phase 2: RS-485 Slave Integration

1. Add RS-485 transceiver hardware
2. Implement RS-485 slave protocol
3. Test as drop-in replacement for one OH slave
4. Verify compatibility with existing OH master

### Phase 3: Multi-Panel Expansion

1. Create Label Sets for additional OH panels
2. Address any I2C address conflicts
3. Test full cockpit integration

### Phase 4: Optional Master Mode

1. Implement RS-485 master protocol
2. Slave discovery and enumeration
3. DCS-BIOS command routing
4. Full CockpitOS ecosystem deployment

---

## File Structure for OH Integration

```
CockpitOS/
├── lib/CUtils/src/
│   ├── MCP23017.h         (new)
│   ├── MCP23017.cpp       (new)
│   ├── MAX7219.h          (new)
│   ├── MAX7219.cpp        (new)
│   ├── PCA9685.h          (new)
│   ├── PCA9685.cpp        (new)
│   ├── PCA9548A.h         (new)
│   ├── PCA9548A.cpp       (new)
│   ├── RS485.h            (new)
│   ├── RS485.cpp          (new)
│   └── Stepper.h          (new)
│   └── Stepper.cpp        (new)
├── src/LABELS/
│   ├── LABEL_SET_OH_IFEI/
│   │   ├── InputMapping.h
│   │   ├── LEDMapping.h
│   │   ├── DisplayMapping.h
│   │   ├── DisplayMapping.cpp
│   │   ├── IFEI_SegmentMap.h
│   │   └── CustomPins.h
│   ├── LABEL_SET_OH_UFC/
│   ├── LABEL_SET_OH_CAUTION/
│   ├── LABEL_SET_OH_DDI/
│   ├── LABEL_SET_OH_AMPCD/
│   └── LABEL_SET_OH_ABSIS_MPC/
└── Docs/
    └── OPENHORNET_INTEGRATION.md (this file)
```

---

## Known Compatibility Issues

1. **I2C Address Conflicts:** OH uses common addresses (0x20-0x27). Multiple panels on same I2C bus need address planning or PCA9548A mux.

2. **3.3V vs 5V Logic:** OH MCP23017 boards may use 5V logic. ESP32 is 3.3V. Level shifters required.

3. **WS2812B Timing:** ESP32 RMT peripheral handles timing. OH's dense WS2812B layouts should work with standard FastLED.

4. **MAX7219 SPI Speed:** ESP32 SPI can run at 10MHz+. MAX7219 max is 10MHz. Match or reduce.

5. **Stepper Motor Current:** X40_8798 gauges draw significant current. ESP32 GPIO cannot drive directly. Need L298N or similar driver.

---

## Testing Checklist

- [ ] MCP23017 driver: Init, read all, write all, interrupts
- [ ] MAX7219 driver: Init, set digit, set row, brightness, cascade
- [ ] PCA9685 driver: Init, PWM, servo pulse, duty cycle
- [ ] PCA9548A driver: Channel select, disable
- [ ] Stepper driver: Init, target, tick, home
- [ ] RS-485 slave: Address response, command handling
- [ ] RS-485 master: Slave enumeration, broadcast
- [ ] Label Set OH_IFEI: Full display + button mapping
- [ ] Label Set OH_UFC: PCA9555 + MCP23017 mixed
- [ ] Label Set OH_CAUTION: MAX7219 LED array
- [ ] Integration: ESP32 + OH PCB physical fit
- [ ] DCS-BIOS: All OH panel controls mapped
