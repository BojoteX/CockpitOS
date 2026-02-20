// Mappings.cpp – Panel Configuration and Runtime Presence Logic

// ==============================================================================================================
// This file manages panel registration, dynamic presence detection, and initialization of all major cockpit panels.
// PCA9555 devices are auto-detected from InputMapping.h and LEDMapping.h — no manual table needed.
// - Button and selector logic is mostly automated; only extend this file when adding new panels, covered controls,
//   or custom panel hardware.
// ==============================================================================================================

#include "src/Globals.h"
#include "Mappings.h"
#include "src/HIDManager.h"
#include "src/LEDControl.h"

// Recently added
#include "src/InputControl.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "src/WiFiDebug.h"
#endif

// kLatchedButtons[] and kCoverGates[] are now per-label-set.
// See src/LABELS/LABEL_SET_*/LatchedButtons.h and CoverGates.h
// Included automatically via LabelSetSelect.h

const char* getPanelName(uint8_t addr) {
    const char* n = (addr < I2C_ADDR_SPACE) ? panelNameByAddr[addr] : nullptr;
    return n ? n : "Unknown";
}

// ================================================================
// Collect unique PCA9555 addresses from InputMapping + LEDMapping
// ================================================================
static uint8_t _pcaAddrs[MAX_DEVICES];
static uint8_t _pcaAddrCount = 0;

static void _collectPcaAddresses() {
    _pcaAddrCount = 0;

    auto addUnique = [](uint8_t addr) {
        if (addr == 0x00) return;  // skip PCA_0x00 placeholder
        for (uint8_t j = 0; j < _pcaAddrCount; ++j)
            if (_pcaAddrs[j] == addr) return;
        if (_pcaAddrCount < MAX_DEVICES)
            _pcaAddrs[_pcaAddrCount++] = addr;
    };

    // From InputMapping: source strings like "PCA_0x22"
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.source || !startsWith(m.source, "PCA_0x")) continue;
        addUnique(parseHexByte(m.source + 4));
    }

    // From LEDMapping: DEVICE_PCA9555 entries
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        if (panelLEDs[i].deviceType == DEVICE_PCA9555)
            addUnique(panelLEDs[i].info.pcaInfo.address);
    }
}

void initMappings() {
    // ================================================================
    // Mapping sanity checks (fail fast on invalid or out-of-range data)
    // ================================================================
    bool ok = true;

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source) continue;

        if (m.group >= MAX_GROUPS) {
            debugPrintf("❌ [MAPPING] %s has group=%u >= MAX_GROUPS (%u)\n",
                m.label, (unsigned)m.group, (unsigned)MAX_GROUPS);
            ok = false;
        }

        if ((strcmp(m.controlType, "selector") == 0) && m.group >= MAX_SELECTOR_GROUPS) {
            debugPrintf("❌ [MAPPING] %s selector group=%u >= MAX_SELECTOR_GROUPS (%u)\n",
                m.label, (unsigned)m.group, (unsigned)MAX_SELECTOR_GROUPS);
            ok = false;
        }

        if (strcmp(m.source, "GPIO") == 0 && m.port >= 48) {
            debugPrintf("❌ [MAPPING] %s uses GPIO port=%d >= 48 (unsupported)\n",
                m.label, (int)m.port);
            ok = false;
        }
    }

    // TM1637 mapping caps
    {
        uint8_t tmKeyCount = 0;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (!m.source) continue;
            if (strcmp(m.source, "TM1637") != 0) continue;
            if (m.port < 0 || m.bit < 0) continue;
            ++tmKeyCount;
        }
        if (tmKeyCount > MAX_TM1637_KEYS) {
            debugPrintf("❌ [MAPPING] TM1637 keys=%u exceeds MAX_TM1637_KEYS=%u\n",
                (unsigned)tmKeyCount, (unsigned)MAX_TM1637_KEYS);
            ok = false;
        }
    }

    // WS2812 mapping caps (per-strip LED count and strip count)
    {
        uint8_t stripPins[WS2812_MAX_STRIPS] = { 0 };
        uint8_t stripCount = 0;
        for (size_t i = 0; i < panelLEDsCount; ++i) {
            const auto& led = panelLEDs[i];
            if (led.deviceType != DEVICE_WS2812) continue;

            if (led.info.ws2812Info.index >= WS2812_MAX_LEDS) {
                debugPrintf("❌ [MAPPING] WS2812 %s index=%u >= WS2812_MAX_LEDS=%u\n",
                    led.label,
                    (unsigned)led.info.ws2812Info.index,
                    (unsigned)WS2812_MAX_LEDS);
                ok = false;
            }

            const uint8_t pin = led.info.ws2812Info.pin;
            bool seen = false;
            for (uint8_t s = 0; s < stripCount; ++s) {
                if (stripPins[s] == pin) { seen = true; break; }
            }
            if (!seen) {
                if (stripCount >= WS2812_MAX_STRIPS) {
                    debugPrintf("❌ [MAPPING] WS2812 pin=%u exceeds WS2812_MAX_STRIPS=%u\n",
                        (unsigned)pin, (unsigned)WS2812_MAX_STRIPS);
                    ok = false;
                }
                else {
                    stripPins[stripCount++] = pin;
                }
            }
        }
    }

    if (!ok) {
        debugPrintln("❌ [MAPPING] Invalid configuration detected. Halting.");
        while (true) { delay(1000); }
    }

    // ================================================================
    // TM1637 sanity check: TM1637 inputs require at least ONE LEDMapping
    // ================================================================

    bool hasTm1637Inputs = false;
    bool hasTm1637LEDs   = false;

    // Check InputMappings for "TM1637"
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (m.source && strcmp(m.source, "TM1637") == 0) {
            hasTm1637Inputs = true;
            break;
        }
    }

    // Check LEDMapping table for DEVICE_TM1637
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        if (panelLEDs[i].deviceType == DEVICE_TM1637) {
            hasTm1637LEDs = true;
            break;
        }
    }

    if (hasTm1637Inputs && !hasTm1637LEDs) {
        debugPrintln("⚠️ WARNING: TM1637 inputs detected but NO TM1637 LEDs found!");
        debugPrintln("⚠️ At least ONE TM1637 LEDMapping entry must exist for each TM1637 device.");
        debugPrintln("⚠️ Add a dummy LED entry so the framework can instantiate the TM1637 device.");
    }

#if ENABLE_PCA9555
    // Auto-detect PCA9555 addresses from InputMapping + LEDMapping
    debugPrintf("Using SDA %d and SCL %d for I2C\n", SDA_PIN, SCL_PIN);

    _collectPcaAddresses();
    PCA9555_scanConnectedPanels(_pcaAddrs, _pcaAddrCount);

    // Show what PCA panels were discovered
    printDiscoveredPanels();
#endif

    // Print registered panels
    for (int i=0; i<PanelRegistry_count(); ++i)
      debugPrintf("Registered Panel: %s\n", PanelRegistry_labelAt(i));

}

// Runs only once when device starts, never again
void initializeDisplays() {
  PanelRegistry_forEachDisplayInit();
}

// Local helper: initialize all TM1637 devices referenced in LEDMapping
static void TM1637_initFromLEDMap_Local() {
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        const auto& led = panelLEDs[i];
        if (led.deviceType != DEVICE_TM1637) continue;

        uint8_t clk = led.info.tm1637Info.clkPin;
        uint8_t dio = led.info.tm1637Info.dioPin;

        TM1637_getOrCreate(clk, dio);
    }
}

void initializeLEDs() {

    // ========================================================================
    // STEP 1: Scan LEDMapping to detect which output drivers are needed
    // This replaces panel-based detection with data-driven detection
    // ========================================================================
    scanOutputDevicePresence();    

    // ========================================================================
    // STEP 2: Initialize PCA9555 devices (data-driven from discovery)
    // ========================================================================
    #if ENABLE_PCA9555
        for (uint8_t i = 0; i < discoveredDeviceCount; ++i)
            PCA9555_autoInitFromLEDMap(discoveredDevices[i].address);
    #endif

    // ========================================================================
    // STEP 3: Initialize drivers based on LEDMapping presence (data-driven)
    // ========================================================================
    
    // GN1640 (Caution Advisory matrix) - now data-driven
    if (hasOutputDevice(DEVICE_GN1640T)) {
        debugPrintln("✅ GN1640 detected in LEDMapping, initializing...");
        GN1640_init(CA_CLK_PIN, CA_DIO_PIN);
    } else {
        debugPrintln("⚠️ GN1640 not present in LEDMapping");
    }

    // WS2812 - now data-driven (was already scanning LEDMapping)
    if (hasOutputDevice(DEVICE_WS2812)) {
        debugPrintln("✅ WS2812 detected in LEDMapping, initializing...");
    } else {
        debugPrintln("⚠️ WS2812 not present in LEDMapping");
    }

    // TM1637: generic init from LEDMapping
    TM1637_initFromLEDMap_Local();

    // Generic TM1637 flash: flash all TM1637 devices once
    bool hasTM = false;
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        if (panelLEDs[i].deviceType == DEVICE_TM1637) {
            hasTM = true;
            break;
        }
    }
    if (hasTM) {
        tm1637_allOn();
        delay(1000);
        tm1637_allOff();
    }

    if (hasOutputDevice(DEVICE_GN1640T)) {
        GN1640_allOn(); delay(1000); GN1640_allOff(); 
    }

    if (hasOutputDevice(DEVICE_WS2812)) {

        // Load ALL WS2812 defined in our file
        initWS2812FromMap(); 

        // Test ALL WS2812 LEDs;
        // WS2812Mini::WS2812_allOnFromMap();
        // delay(1000);
        // WS2812Mini::WS2812_allOffAll();

        WS2812_allOn(Green); delay(1000); WS2812_allOff();
    }

    // PCA9555 devices — flash all discovered PCA LEDs
    #if ENABLE_PCA9555
        for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
            PCA9555_allOn(discoveredDevices[i].address);
            delay(1000);
            PCA9555_allOff(discoveredDevices[i].address);
        }
    #endif

    // GPIO LEDs
    preconfigureGPIO();
    GPIO_setAllLEDs(true); delay(1000); GPIO_setAllLEDs(false);

    // Always init GPIO 0 (ESP32S2) for testing and debugging
    // pinMode(0, INPUT_PULLUP);

    // Any custom outputs such as LEDs, Analog gauges or mechanical levers should go here.
}

// Runs on mission start (sim)
void initializePanels(bool force) {
    if(!mainLoopStarted && !force) return;
    debugPrintln("Syncronizing Panel state....");

    PanelRegistry_forEachInit();

    debugPrintln("Finished Syncronizing Panel state....");
    if (!isModeSelectorDCS()) HIDManager_commitDeferredReport("All devices");
}

void panelLoop() {

    PanelRegistry_forEachLoop();
    PanelRegistry_forEachDisplayLoop();
    PanelRegistry_forEachTick();
    
    // ========================================================================
    // AUTO-TICK: Flush output driver buffers based on LEDMapping presence
    // This makes panel tick registration optional for LED-only panels
    // ========================================================================
    tickOutputDrivers();

    #if DEBUG_USE_WIFI && WIFI_DEBUG_USE_RINGBUFFER
    wifiDebugDrainSendBuffer();
    #endif

    #if SERIAL_DEBUG_USE_RINGBUFFER
    sendPendingSerial();
    #endif

#if ENABLE_PCA9555 
    if (isPCA9555LoggingEnabled()) {
        for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
            uint8_t addr = discoveredDevices[i].address;
            byte p0, p1;
            readPCA9555(addr, p0, p1);
        }
    }
#endif
    
}

bool isLatchedButton(const char* label) {
    for (unsigned i = 0; i < kLatchedButtonCount; ++i)
        if (strcmp(label, kLatchedButtons[i]) == 0)
            return true;
    return false;
}