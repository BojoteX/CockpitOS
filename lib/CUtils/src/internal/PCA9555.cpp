// Max 8 PCA9555 devices
static byte prevPort0Cache[8];
static byte prevPort1Cache[8];
static uint8_t addrCache[8];
static uint8_t cacheSize = 0;
bool loggingEnabled = false;

/////////////////////////////////////
// Actual i2c functions
/////////////////////////////////////

#if USE_WIRE_FOR_I2C
#include <Wire.h>

void PCA9555_init() {
    Wire.setTimeOut(100);
    #if PCA_FAST_MODE
    Wire.begin(SDA_PIN, SCL_PIN, 400000); // FAST PCA Mode
    debugPrint("I²C Initialized using the Arduino Wire Library (Fast Mode)\n");
    #else
    Wire.begin(SDA_PIN, SCL_PIN, 100000); // Standard mode
    debugPrint("I²C Initialized using the Arduino Wire Library (Normal mode)\n");
    #endif
}

// For reference only
void measureI2Cspeed(uint8_t deviceAddr) {
    uint32_t t0 = micros();
    Wire.requestFrom((uint8_t)deviceAddr, (uint8_t)2);

    while (Wire.available()) {
        Wire.read();
    }
    uint32_t t1 = micros();
    debugPrintf("I²C at 0x%02X Read Time: %u us\n", deviceAddr, t1 - t0);
}

void PCA9555_scanConnectedPanels() {
    delay(500); // Give time for I2C bus to stabilize

    // Step 2: Proceed as before: scan and record expected devices only
    discoveredDeviceCount = 0;
    memset(panelNameByAddr, 0, sizeof(panelNameByAddr));

    for (const auto& p : kPanels) {
        bool present = false;
        int lastWireError = -1;
        int lastReqFrom = -1;

        for (uint8_t attempt = 1; attempt <= 3; ++attempt) {
            Wire.beginTransmission(p.addr);
            Wire.write((uint8_t)0x00);
            int error = Wire.endTransmission(false);
            int req = 0;
            if (error == 0) {
                req = Wire.requestFrom(p.addr, (uint8_t)1);
                if (req == 1) {
                    int value = Wire.read();
                    present = true;
                    break;
                }
            }
            lastWireError = error;
            lastReqFrom = req;
            delay(20);
        }

        if (present && discoveredDeviceCount < MAX_DEVICES) {
            discoveredDevices[discoveredDeviceCount].address = p.addr;
            discoveredDevices[discoveredDeviceCount].label = p.label;
            ++discoveredDeviceCount;
            panelNameByAddr[p.addr] = p.label;
        }
        else {
			// Log the error if the device was not found
        }
    }
}

/*
void PCA9555_scanConnectedPanels() {
    discoveredDeviceCount = 0;
    memset(panelNameByAddr, 0, sizeof(panelNameByAddr));
    for (const auto& p : kPanels) {
        if (discoveredDeviceCount >= MAX_DEVICES) break;
        discoveredDevices[discoveredDeviceCount].address = p.addr;
        discoveredDevices[discoveredDeviceCount].label = p.label;
        ++discoveredDeviceCount;
        panelNameByAddr[p.addr] = p.label;
    }
}
*/

void PCA9555_initCache() {
    for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
        uint8_t addr = discoveredDevices[i].address;
        debugPrintf("Initializing PCA_0x%02X inputs and cached port states\n", addr);

        initPCA9555AsInput(addr);

        PCA9555_cachedPortStates[addr][0] = 0xFF;
        PCA9555_cachedPortStates[addr][1] = 0xFF;

        Wire.beginTransmission(addr);
        Wire.write((uint8_t)0x02);
        Wire.write((uint8_t)0xFF);
        Wire.write((uint8_t)0xFF);
        Wire.endTransmission();
    }
}

// Reads both config and output registers for the PCA9555 at 'addr' using Wire
void PCA9555_readConfigOutput(uint8_t addr, uint8_t& config0, uint8_t& config1, uint8_t& out0, uint8_t& out1) {
    // Config
    Wire.beginTransmission(addr);
    Wire.write(0x06);
    Wire.endTransmission(false); // No stop
    Wire.requestFrom(addr, (uint8_t)2);
    config0 = Wire.read();
    config1 = Wire.read();

    // Output
    Wire.beginTransmission(addr);
    Wire.write(0x02);
    Wire.endTransmission(false); // No stop
    Wire.requestFrom(addr, (uint8_t)2);
    out0 = Wire.read();
    out1 = Wire.read();
}

void PCA9555_autoInitFromLEDMap(uint8_t addr) {
    uint8_t configPort0, configPort1, outputPort0, outputPort1;
    PCA9555_readConfigOutput(addr, configPort0, configPort1, outputPort0, outputPort1);

    int nChanged = 0;
    for (int i = 0; i < panelLEDsCount; i++) {
        if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
            uint8_t port = panelLEDs[i].info.pcaInfo.port;
            uint8_t bit = panelLEDs[i].info.pcaInfo.bit;
            const char* label = panelLEDs[i].label;
            if (port == 0) {
                if (configPort0 & (1 << bit)) {
                    debugPrintf("PCA9555 0x%02X: Pin P0.%u (label=%s) set as OUTPUT (LED)\n", addr, bit, label);
                    configPort0 &= ~(1 << bit);
                    nChanged++;
                }
                outputPort0 |= (1 << bit); // LED OFF (HIGH)
            }
            else {
                if (configPort1 & (1 << bit)) {
                    debugPrintf("PCA9555 0x%02X: Pin P1.%u (label=%s) set as OUTPUT (LED)\n", addr, bit, label);
                    configPort1 &= ~(1 << bit);
                    nChanged++;
                }
                outputPort1 |= (1 << bit); // LED OFF (HIGH)
            }
        }
    }

    // Write outputs first, then config
    Wire.beginTransmission(addr);
    Wire.write(0x02);           // Output port register PORT0
    Wire.write(outputPort0);    // Port0
    Wire.write(outputPort1);    // Port1
    Wire.endTransmission();

    Wire.beginTransmission(addr);
    Wire.write(0x06);           // Configuration register PORT0
    Wire.write(configPort0);    // Port0
    Wire.write(configPort1);    // Port1
    Wire.endTransmission();

    debugPrintf("PCA9555 0x%02X: %d pins set as OUTPUT (LED)\n", addr, nChanged);
}

void PCA9555_write(uint8_t addr, uint8_t port, uint8_t bit, bool state) {
    uint8_t data0, data1;

    if (!panelExists(addr)) {
        if(DEBUG) debugPrintf("[PCA] ❌ Write / LED skipped. %s (0x%02X) not present\n", getPanelName(addr), addr);
        return;
    }

    // update the cache
    if (state)
        PCA9555_cachedPortStates[addr][port] |=  (1 << bit);
    else
        PCA9555_cachedPortStates[addr][port] &= ~(1 << bit);

    data0 = PCA9555_cachedPortStates[addr][0];
    data1 = PCA9555_cachedPortStates[addr][1];

    // one-shot I²C write both ports
    uint32_t t0 = micros();
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x02);
    Wire.write(data0);
    Wire.write(data1);
    Wire.endTransmission();
    uint32_t dt = micros() - t0;
    if(DEBUG) debugPrintf("[INFO] PCA 0x%2x raw I2C write: %u µs\n", addr, dt);
}

void initPCA9555AsInput(uint8_t addr) {
    uint8_t configPort0 = 0xFF; // All inputs initially
    uint8_t configPort1 = 0xFF; // All inputs initially

    // EXCLUDE any LED pin clearly (set as outputs later)
    for (int i = 0; i < panelLEDsCount; i++) {
        if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
            if (panelLEDs[i].info.pcaInfo.port == 0) {
                configPort0 &= ~(1 << panelLEDs[i].info.pcaInfo.bit); // exclude LED pins (output)
            } else {
                configPort1 &= ~(1 << panelLEDs[i].info.pcaInfo.bit); // exclude LED pins (output)
            }
        }
    }

    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x06); // Configuration register PORT0
    Wire.write(configPort0);   // Set directions for PORT0
    Wire.write(configPort1);   // Set directions for PORT1
    Wire.endTransmission();
}

// Corrected readPCA9555 function (Wire-style)
bool readPCA9555(uint8_t address, byte &port0, byte &port1) {
    byte tmpPort0, tmpPort1;

    Wire.beginTransmission(address);
    Wire.write((uint8_t)0x00);  // Port 0 input register
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom((uint8_t)address, (uint8_t)2) == 2) {
        tmpPort0 = Wire.read();
        tmpPort1 = Wire.read();

        port0 = tmpPort0;
        port1 = tmpPort1;

        // NOW CALL LOGGER IF CHANGED
        if (isPCA9555LoggingEnabled() && shouldLogChange(address, port0, port1)) {
            logPCA9555State(address, port0, port1);
        }

        return true;
    }
    return false;
}

#else

#include <driver/i2c.h>
#define PCA_I2C_PORT    I2C_NUM_0
#define I2C_TIMEOUT_MS  10

void PCA9555_init() {
    i2c_driver_delete(PCA_I2C_PORT); // In case already installed
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = SDA_PIN;
    conf.scl_io_num = SCL_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    #if PCA_FAST_MODE
    conf.master.clk_speed = 400000;
    debugPrintln("I²C Initialized using the ESP-IDF Library (Fast mode)");
    #else
    conf.master.clk_speed = 100000;
    debugPrintln("I²C Initialized using the ESP-IDF Library (Normal mode");
    #endif
    i2c_param_config(PCA_I2C_PORT, &conf);
    i2c_driver_install(PCA_I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
}

// For reference only
void measureI2Cspeed(uint8_t deviceAddr) {
    uint32_t t0 = micros();
    uint8_t dummy[2] = {0};
    i2c_master_read_from_device(PCA_I2C_PORT, deviceAddr, dummy, 2, 100);
    uint32_t t1 = micros();
    debugPrintf("I²C at 0x%02X Read Time: %u us\n", deviceAddr, t1 - t0);
}

void PCA9555_scanConnectedPanels() {
    delay(500); // Give time for I2C bus to stabilize

    // Step 2: Proceed as before: scan and record expected devices only
    discoveredDeviceCount = 0;
    memset(panelNameByAddr, 0, sizeof(panelNameByAddr));

    for (const auto& p : kPanels) {
        bool present = false;
        esp_err_t lastRet = ESP_FAIL;
        for (uint8_t attempt = 1; attempt <= 3; ++attempt) {
            uint8_t reg = 0x00;
            uint8_t val = 0xEE;
            esp_err_t ret = i2c_master_write_read_device(
                PCA_I2C_PORT, p.addr, &reg, 1, &val, 1, 100
            );
            lastRet = ret;
            if (ret == ESP_OK) {
                present = true;
                break;
            }
            delay(50);
        }
        if (present && discoveredDeviceCount < MAX_DEVICES) {
            discoveredDevices[discoveredDeviceCount].address = p.addr;
            discoveredDevices[discoveredDeviceCount].label = p.label;
            ++discoveredDeviceCount;
            panelNameByAddr[p.addr] = p.label;
        }
        else {
			// Log the last error if not found
        }
    }
}

/*
void PCA9555_scanConnectedPanels() {
    discoveredDeviceCount = 0;
    memset(panelNameByAddr, 0, sizeof(panelNameByAddr));
    for (const auto& p : kPanels) {
        if (discoveredDeviceCount >= MAX_DEVICES) break;
        discoveredDevices[discoveredDeviceCount].address = p.addr;
        discoveredDevices[discoveredDeviceCount].label = p.label;
        ++discoveredDeviceCount;
        panelNameByAddr[p.addr] = p.label;
    }
}
*/

// Atomic/fast multi-byte write (for cache/init/output, always 3 bytes: reg, val0, val1)
static inline esp_err_t PCA9555_writeReg2(uint8_t addr, uint8_t reg, uint8_t val0, uint8_t val1) {
    uint8_t buf[3] = {reg, val0, val1};
    return i2c_master_write_to_device(PCA_I2C_PORT, addr, buf, 3, 100);
}

// For LED cache/init, always best to use atomic multi-byte write
void PCA9555_initCache() {
    for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
        uint8_t addr = discoveredDevices[i].address;
        debugPrintf("Initializing PCA_0x%02X inputs and cached port states\n", addr);
        initPCA9555AsInput(addr);
        PCA9555_cachedPortStates[addr][0] = 0xFF;
        PCA9555_cachedPortStates[addr][1] = 0xFF;
        PCA9555_writeReg2(addr, 0x02, 0xFF, 0xFF);
    }
}

// Single 8-bit register read (pure ESP-IDF)
uint8_t PCA9555_readReg(uint8_t addr, uint8_t reg) {
    uint8_t val = 0xFF;
    esp_err_t ret = i2c_master_write_read_device(
        PCA_I2C_PORT, addr, &reg, 1, &val, 1, I2C_TIMEOUT_MS / portTICK_PERIOD_MS
    );
    if (ret != ESP_OK) {
        // Optionally: debugPrintf("PCA9555 0x%02X: read reg 0x%02X failed (%d)\n", addr, reg, ret);
        return 0xFF;
    }
    return val;
}

// Reads both config and output registers for the PCA9555 at 'addr'
void PCA9555_readConfigOutput(uint8_t addr, uint8_t& config0, uint8_t& config1, uint8_t& out0, uint8_t& out1) {
    config0 = PCA9555_readReg(addr, 0x06); // Config Port 0
    config1 = PCA9555_readReg(addr, 0x07); // Config Port 1
    out0 = PCA9555_readReg(addr, 0x02); // Output Port 0
    out1 = PCA9555_readReg(addr, 0x03); // Output Port 1
}

void PCA9555_autoInitFromLEDMap(uint8_t addr) {
    uint8_t configPort0, configPort1, outputPort0, outputPort1;
    PCA9555_readConfigOutput(addr, configPort0, configPort1, outputPort0, outputPort1);

    int nChanged = 0;
    for (int i = 0; i < panelLEDsCount; i++) {
        if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
            uint8_t port = panelLEDs[i].info.pcaInfo.port;
            uint8_t bit = panelLEDs[i].info.pcaInfo.bit;
            const char* label = panelLEDs[i].label;
            if (port == 0) {
                if (configPort0 & (1 << bit)) {
                    debugPrintf("PCA9555 0x%02X: Pin P0.%u (label=%s) set as OUTPUT (LED)\n", addr, bit, label);
                    configPort0 &= ~(1 << bit); // 0 = output
                    nChanged++;
                }
                outputPort0 |= (1 << bit); // (active-low: HIGH = off)
            }
            else {
                if (configPort1 & (1 << bit)) {
                    debugPrintf("PCA9555 0x%02X: Pin P1.%u (label=%s) set as OUTPUT (LED)\n", addr, bit, label);
                    configPort1 &= ~(1 << bit);
                    nChanged++;
                }
                outputPort1 |= (1 << bit);
            }
        }
    }
    // Always write outputs first, then config, regardless of nChanged
    PCA9555_writeReg2(addr, 0x02, outputPort0, outputPort1); // Set output latches
    PCA9555_writeReg2(addr, 0x06, configPort0, configPort1); // Set config

    debugPrintf("PCA9555 0x%02X: %d pins set as OUTPUT (LED)\n", addr, nChanged);
}

void PCA9555_write(uint8_t addr, uint8_t port, uint8_t bit, bool state) {
    if (!panelExists(addr)) {
        if(DEBUG) debugPrintf("[PCA] ❌ Write / LED skipped. %s (0x%02X) not present\n", getPanelName(addr), addr);
        return;
    }
    if (state)
        PCA9555_cachedPortStates[addr][port] |= (1 << bit);
    else
        PCA9555_cachedPortStates[addr][port] &= ~(1 << bit);
    uint8_t data0 = PCA9555_cachedPortStates[addr][0];
    uint8_t data1 = PCA9555_cachedPortStates[addr][1];
    uint32_t t0 = micros();
    PCA9555_writeReg2(addr, 0x02, data0, data1);
    uint32_t dt = micros() - t0;
    if(DEBUG) debugPrintf("[INFO] PCA 0x%2x raw I2C write: %u µs\n", addr, dt);
}

void initPCA9555AsInput(uint8_t addr) {
    uint8_t configPort0 = 0xFF, configPort1 = 0xFF;
    for (int i = 0; i < panelLEDsCount; i++) {
        if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
            if (panelLEDs[i].info.pcaInfo.port == 0)
                configPort0 &= ~(1 << panelLEDs[i].info.pcaInfo.bit);
            else
                configPort1 &= ~(1 << panelLEDs[i].info.pcaInfo.bit);
        }
    }
    PCA9555_writeReg2(addr, 0x06, configPort0, configPort1);
}

// Fast/atomic input read
bool readPCA9555(uint8_t address, byte &port0, byte &port1) {
    uint8_t reg = 0x00;
    uint8_t buf[2] = {0, 0};
    esp_err_t ret = i2c_master_write_read_device(
        PCA_I2C_PORT,
        address,
        &reg, 1,
        buf, 2,
        100
    );
    if (ret == ESP_OK) {
        port0 = buf[0];
        port1 = buf[1];
        if (isPCA9555LoggingEnabled() && shouldLogChange(address, port0, port1)) {
            logPCA9555State(address, port0, port1);
        }
        return true;
    }
    return false;
}

#endif


//////////////////////////////////////////////
// Internal Expander functions
//////////////////////////////////////////////

// static const char* resolveInputLabel(uint8_t addr, uint8_t port, uint8_t bit);
static const char* resolveInputLabel(uint8_t addr, uint8_t port, uint8_t bit) {
  char deviceName[20];
  snprintf(deviceName, sizeof(deviceName), "PCA_0x%02X", addr);
  for (size_t i = 0; i < InputMappingSize; ++i) {
    const InputMapping& m = InputMappings[i];
    if (!m.source) continue;
    if (strcmp(m.source, deviceName) == 0 && m.port == port && m.bit == bit) {
      return m.label;
    }
  }
  return nullptr;
}

// static const InputMapping* resolveInputMapping(uint8_t addr, uint8_t port, uint8_t bit);
static const InputMapping* resolveInputMapping(uint8_t addr, uint8_t port, uint8_t bit) {
  char deviceName[20];
  snprintf(deviceName, sizeof(deviceName), "PCA_0x%02X", addr);
  for (size_t i = 0; i < InputMappingSize; ++i) {
    const InputMapping& m = InputMappings[i];
    if (!m.source) continue;
    if (strcmp(m.source, deviceName) == 0 && m.port == port && m.bit == bit) {
      return &m;
    }
  }
  return nullptr;
}

static bool isInputBitMapped(uint8_t addr, uint8_t port, uint8_t bit) {
  return resolveInputLabel(addr, port, bit) != nullptr;
}

void PCA9555_setAllLEDs(bool state) {
  for (int i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType == DEVICE_PCA9555) {
      bool writeState = panelLEDs[i].activeLow ? !state : state;
      PCA9555_write(panelLEDs[i].info.pcaInfo.address,
                    panelLEDs[i].info.pcaInfo.port,
                    panelLEDs[i].info.pcaInfo.bit,
                    writeState);
    }
  }
}

// PCA9555 Helper Implementation
void PCA9555_allLEDsByAddress(uint8_t addr, bool state) {
    for (int i = 0; i < panelLEDsCount; i++) {
        if (panelLEDs[i].deviceType == DEVICE_PCA9555 &&
            panelLEDs[i].info.pcaInfo.address == addr) {
            bool writeState = panelLEDs[i].activeLow ? !state : state;
            PCA9555_write(addr,
                          panelLEDs[i].info.pcaInfo.port,
                          panelLEDs[i].info.pcaInfo.bit,
                          writeState);
        }
    }
}

// Turn ON all PCA9555 LEDs at a specific address
void PCA9555_allOn(uint8_t addr) {
  debugPrintf("🔆 PCA9555 (0x%02X) Turning ALL LEDs ON\n", addr);
  for (int i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
      // setLED(panelLEDs[i].label, true, 100);
      setLED(panelLEDs[i].label, true);
    }
  }
}

// Turn OFF all PCA9555 LEDs at a specific address
void PCA9555_allOff(uint8_t addr) {
  debugPrintf("⚫ PCA9555 (0x%02X) Turning ALL LEDs OFF\n", addr);
  for (int i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
      // setLED(panelLEDs[i].label, false, 0);
      setLED(panelLEDs[i].label, false);
    }
  }
}

// Sweep through all PCA9555 LEDs at a specific address
void PCA9555_sweep(uint8_t addr) {
  debugPrintf("🔍 PCA9555 (0x%02X) LED Sweep Start\n", addr);
  for (int i = 0; i < panelLEDsCount; i++) {
    if (panelLEDs[i].deviceType == DEVICE_PCA9555 && panelLEDs[i].info.pcaInfo.address == addr) {
      debugPrint("🟢 Sweeping LED: ");
      debugPrintln(panelLEDs[i].label);
      setLED(panelLEDs[i].label, true, 100);
      delay(500);
      setLED(panelLEDs[i].label, false, 0);
    }
  }
  debugPrintf("✅ PCA9555 (0x%02X) LED Sweep Complete\n", addr);
}

// Run full PCA9555 test pattern for a single device (All OFF → Sweep → All ON → OFF)
void PCA9555_patternTesting(uint8_t addr) {
  debugPrintf("🧪 PCA9555 (0x%02X) Test Pattern Start\n", addr);
  PCA9555_allOff(addr);
  PCA9555_allOn(addr);
  delay(3000);
  PCA9555_allOff(addr);
  debugPrintf("✅ PCA9555 (0x%02X) Test Pattern Complete\n", addr);
}

int getCacheIndex(uint8_t address) {
  for (uint8_t i = 0; i < cacheSize; i++) {
    if (addrCache[i] == address) return i;
  }
  return -1;
}

bool shouldLogChange(uint8_t address, byte port0, byte port1) {
  int idx = getCacheIndex(address);
  if (idx >= 0) {
    return (prevPort0Cache[idx] != port0 || prevPort1Cache[idx] != port1);
  }

  // Nuevo PCA detectado
  if (cacheSize < 8) {
    addrCache[cacheSize] = address;
    prevPort0Cache[cacheSize] = port0;
    prevPort1Cache[cacheSize] = port1;
    cacheSize++;
    return true;
  }

  return false;
}

void enablePCA9555Logging(bool enable) {
  loggingEnabled = enable;
}

bool isPCA9555LoggingEnabled() {
  return loggingEnabled;
}

void logExpanderState(uint8_t p0, uint8_t p1) {
  char buffer[32];

  // Formatting p0
  int idx = 0;
  idx += snprintf(buffer + idx, sizeof(buffer) - idx, " → [ p0:");
  for (int8_t b = 6; b >= 0; --b) {
    idx += snprintf(buffer + idx, sizeof(buffer) - idx, "%u", (p0 >> b) & 1);
  }

  // Formatting p1
  idx += snprintf(buffer + idx, sizeof(buffer) - idx, " | p1:");
  for (int8_t b = 6; b >= 0; --b) {
    idx += snprintf(buffer + idx, sizeof(buffer) - idx, "%u", (p1 >> b) & 1);
  }

  idx += snprintf(buffer + idx, sizeof(buffer) - idx, " ]");

  debugPrintln(buffer);  // Send the complete buffer to your debug handler
}

void logPCA9555State(uint8_t address, byte port0, byte port1) {
  int idx = getCacheIndex(address);
  if (idx < 0) return;

  byte prev0 = prevPort0Cache[idx];
  byte prev1 = prevPort1Cache[idx];

  bool printedNewLine = false;

  for (int port = 0; port <= 1; port++) {
    byte prev = (port == 0) ? prev0 : prev1;
    byte curr = (port == 0) ? port0 : port1;

    for (int b = 0; b < 8; b++) {
      if (bitRead(prev, b) != bitRead(curr, b)) {

        const InputMapping* mapping = resolveInputMapping(address, port, b);

        if (printedNewLine) debugPrintln("");

        debugPrintf("⚡PCA 0x%02X ", address);
        logExpanderState(port0, port1);
        debugPrint(" ");

        debugPrintf("→ Port%d Bit%d ", port, b);

        if (!mapping) {
          debugPrint("❌ No label mapped");
        } else {
          debugPrintf("✅ DCS Command = %s", mapping->oride_label ? mapping->oride_label : "(none)");
        }

        printedNewLine = true;
      }
    }
  }

  debugPrintln("");

  prevPort0Cache[idx] = port0;
  prevPort1Cache[idx] = port1;
}