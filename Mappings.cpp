// Mappings.cpp – Panel Configuration and Runtime Presence Logic

// ==============================================================================================================
// This file manages panel registration, dynamic presence detection, and initialization of all major cockpit panels.
// - Add physical "cover gate" logic or custom guarded button/selector definitions here (see kCoverGates).
// - All PCA9555 panel devices, annunciators, and custom LEDs/gauges should be configured/initialized here.
// - Button and selector logic is mostly automated; only extend this file when adding new panels, covered controls,
//   or custom panel hardware.
// ==============================================================================================================

#include "src/Globals.h"
#include "Mappings.h"
#include "src/HIDManager.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "src/WiFiDebug.h"
#endif

// This table is where you add selectors or buttons that are physically guarded by a "cover"
const CoverGateDef kCoverGates[] = {
    // --- 2-Pos SELECTORS that are physically guarded --- ON_POSITION OFF_POSITION COVER_NAME ACTION_TYPE DELAY(OPEN) DELAY(CLOSE) 
    { "GAIN_SWITCH_POS1",      "GAIN_SWITCH_POS0",     "GAIN_SWITCH_COVER",         CoverGateKind::Selector,        500,  500 },
    { "GEN_TIE_SW_RESET",      "GEN_TIE_SW_NORM",      "GEN_TIE_COVER",             CoverGateKind::Selector,        500,  500 },
    { "SPIN_RECOVERY_SW_RCVY", "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_COVER",       CoverGateKind::Selector,        500,  500 },

    // --- Momentary (latching) BUTTONS that are behind a cover --- TOGGLE N/A COVER_NAME ACTION_TYPE DELAY(OPEN) DELAY(CLOSE)
    { "LEFT_FIRE_BTN",         nullptr,                "LEFT_FIRE_BTN_COVER",       CoverGateKind::ButtonMomentary, 350,  300 },
    { "RIGHT_FIRE_BTN",        nullptr,                "RIGHT_FIRE_BTN_COVER",      CoverGateKind::ButtonMomentary, 350,  300 },
    // --- Add other covered buttons as your cockpit expands ---
    // { "SOME_OTHER_BTN",    nullptr, "SOME_COVER", CoverGateKind::ButtonMomentary, 400, 200 },

    // TODO: Momentary (NON latching) BUTTONS behind a cover. Is it really required?
};

// This table is where you add LABELS for buttons that require "latching"
const char* kLatchedButtons[] = {
    "APU_FIRE_BTN",
    "CMSD_JET_SEL_BTN",
    "RWR_POWER_BTN",
    "SJ_CTR",
    "SJ_LI",
    "SJ_LO",
    "SJ_RI",
    "SJ_RO",
    // ...add more as needed
};
const unsigned kLatchedButtonCount = sizeof(kLatchedButtons)/sizeof(kLatchedButtons[0]);
const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);

PanelID getPanelID(uint8_t address) {
  for (auto &p : kPanels)
    if (p.addr == address) return p.id;
  return PanelID::UNKNOWN;
}

const char* panelIDToString(PanelID id) {
  for (auto &p : kPanels)
    if (p.id == id) return p.label;
  return "Unknown Panel";
}

const char* getPanelName(uint8_t addr) {
    const char* n = (addr < I2C_ADDR_SPACE) ? panelNameByAddr[addr] : nullptr;
    return n ? n : panelIDToString(getPanelID(addr));
}

void initMappings() {

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
    // Runs a discovery routine to check for PCA panels automatically, but they still need to be added manually in kPanels[] (see Mappings.h)
    debugPrintf("Using SDA %d and SCL %d for I2C\n", SDA_PIN, SCL_PIN);

    PCA9555_scanConnectedPanels();

    PanelRegistry_setActive(PanelKind::ECM, panelExists(0x22));
    PanelRegistry_setActive(PanelKind::MasterARM, panelExists(0x5B));
    PanelRegistry_setActive(PanelKind::Brain, panelExists(0x26));
  
    // Show what PCA panels were discovered
    printDiscoveredPanels();

#else   
    // hard-off at runtime when PCA is disabled at compile time
    PanelRegistry_setActive(PanelKind::ECM,       false);
    PanelRegistry_setActive(PanelKind::MasterARM, false);
    PanelRegistry_setActive(PanelKind::Brain,     false);    
#endif

    // Print registered panels
    int n = PanelRegistry_count();
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

    if (PanelRegistry_has(PanelKind::MasterARM)) PCA9555_autoInitFromLEDMap(0x5B);
    if (PanelRegistry_has(PanelKind::ECM)) PCA9555_autoInitFromLEDMap(0x22);
    if (PanelRegistry_has(PanelKind::Brain)) PCA9555_autoInitFromLEDMap(0x26);

    if (PanelRegistry_has(PanelKind::LockShoot)) {
        debugPrintln("✅ Lock/Shoot detected, initializing...");
    } else {
        debugPrintln("⚠️ Lock/Shoot NOT detected!");
    }

    if (PanelRegistry_has(PanelKind::CA)) {
        debugPrintln("✅ Caution Advisory detected, initializing...");
        GN1640_init(CA_CLK_PIN, CA_DIO_PIN);
    } else {
        debugPrintln("⚠️ Caution Advisory NOT detected!");
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

    // Flash sequence
    if (PanelRegistry_has(PanelKind::CA)) { GN1640_allOn(); delay(1000); GN1640_allOff(); }
    if (PanelRegistry_has(PanelKind::LockShoot)) { WS2812_allOn(Green); delay(1000); WS2812_allOff(); }

    // PCA9555 devices
    if (PanelRegistry_has(PanelKind::MasterARM)) { PCA9555_allOn(0x5B); delay(1000); PCA9555_allOff(0x5B); }
    if (PanelRegistry_has(PanelKind::ECM)) { PCA9555_allOn(0x22); delay(1000); PCA9555_allOff(0x22); }

    // GPIO LEDs
    preconfigureGPIO();
    GPIO_setAllLEDs(true); delay(1000); GPIO_setAllLEDs(false);

    // Load ALL WS2812 defined in our file
    initWS2812FromMap(); 

    // Test ALL WS2812 LEDs;
    // WS2812_allOnFromMap();
    WS2812Mini::WS2812_allOnFromMap();
    delay(1000);
    WS2812Mini::WS2812_allOffAll();

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