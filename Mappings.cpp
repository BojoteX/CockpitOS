// Mappings.cpp - Panel configuration and runtime presence logic

#include "src/Globals.h"
#include "Mappings.h"
#include "src/HIDManager.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "src/WiFiDebug.h"
#endif

// -- Panel Modules --
/*
#include "src/LeftAnnunciator.h"
#include "src/RightAnnunciator.h"
#include "src/IRCoolPanel.h"
#include "src/MasterARMPanel.h"
#include "src/ALR67Panel.h"
#include "src/ECMPanel.h"
#include "src/IFEIPanel.h"
#include "src/TFT_Gauges_BrakePress.h"
#include "src/TFT_Gauges_CabPress.h"
#include "src/TFT_Gauges_RadarAlt.h"
#include "src/TFT_Gauges_HydPress.h"
#include "src/TFT_Gauges_Batt.h"
#include "src/FrontLeftPanel.h"
#include "src/CustomFrontRightPanel.h"
#include "src/RightPanelController.h"
#include "src/LeftPanelController.h"
*/

#include "src/Panels/includes/TEST_ONLY.h"
// Don't forget to include headers for any custom panels added

// Panel presence flags (runtime, set in initMappings() via registry or panelExists())
// Do NOT set manually or via preprocessor!
bool hasBrain                 = false;
bool hasECM                   = false;
bool hasMasterARM             = false;
bool hasIFEI                  = false;
bool hasALR67                 = false;
bool hasCA                    = false;
bool hasLA                    = false;
bool hasRA                    = false;
bool hasIR                    = false;
bool hasLockShoot             = false;
bool hasGauge                 = false;
bool hasTFTBattGauge          = false;
bool hasTFTCabPressGauge      = false;
bool hasTFTBrakePressGauge    = false;
bool hasTFTHydPressGauge      = false;
bool hasTFTRadarAltGauge      = false;
bool hasRightPanelController  = false;
bool hasLeftPanelController   = false;
bool hasFrontLeftPanel        = false;
bool hasCustomFrontRightPanel = false;
bool hasTEST_ONLY             = false;
// Add more runtime panel conditionals here when adding custom panels/other aircraft

// TM1637 device instances (must match externs)
TM1637Device LA_Device; // Left Annunciator, should always be present at file scope
TM1637Device RA_Device; // Right Annunciator, should always be present at file scope

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

    // Runs a discovery routine to check for PCA panels automatically, but they still need to be added manually in kPanels[] (see Mappings.h)
    debugPrintf("Using SDA %d and SCL %d for I2C\n", SDA_PIN, SCL_PIN);

#if defined(ARDUINO_LOLIN_S3_MINI)
    debugPrintln("Device is LOLIN S3 Mini by WEMOS");
#elif defined(ARDUINO_LOLIN_S2_MINI)
    debugPrintln("Device is LOLIN S2 Mini by WEMOS");
#elif defined(ARDUINO_LOLIN_C3_MINI)
    debugPrintln("Device is LOLIN C3 Mini by WEMOS");
#else
    debugPrintln("Unknown device type");
#endif

    // Presence via registry
    hasBrain                 = PanelRegistry_has(PanelKind::Brain);
    hasIFEI                  = PanelRegistry_has(PanelKind::IFEI);
    hasALR67                 = PanelRegistry_has(PanelKind::ALR67);
    hasCA                    = PanelRegistry_has(PanelKind::CA);
    hasLA                    = PanelRegistry_has(PanelKind::LA);
    hasRA                    = PanelRegistry_has(PanelKind::RA);
    hasIR                    = PanelRegistry_has(PanelKind::IR);
    hasLockShoot             = PanelRegistry_has(PanelKind::LockShoot);
    hasTFTBattGauge          = PanelRegistry_has(PanelKind::TFTBatt);
    hasTFTCabPressGauge      = PanelRegistry_has(PanelKind::TFTCabPress);
    hasTFTBrakePressGauge    = PanelRegistry_has(PanelKind::TFTBrake);
    hasTFTHydPressGauge      = PanelRegistry_has(PanelKind::TFTHyd);
    hasTFTRadarAltGauge      = PanelRegistry_has(PanelKind::TFTRadarAlt);
    hasRightPanelController  = PanelRegistry_has(PanelKind::RightPanelCtl);
    hasLeftPanelController   = PanelRegistry_has(PanelKind::LeftPanelCtl);
    hasFrontLeftPanel        = PanelRegistry_has(PanelKind::FrontLeft);
    hasCustomFrontRightPanel = PanelRegistry_has(PanelKind::CustomFrontRight);
    hasTEST_ONLY             = PanelRegistry_has(PanelKind::TEST_ONLY);

    // Print registered panels
    int n = PanelRegistry_count();
    debugPrintf("Registry count=%d\n", PanelRegistry_count());
    for (int i=0; i<PanelRegistry_count(); ++i)
      debugPrintf("Registered Panel: %s\n", PanelRegistry_labelAt(i));

    PCA9555_scanConnectedPanels();

    // Auto-Detection is used for outputs so that we don't drive what is not present.
    // ---- Runtime detection overrides ---- (This is used so that outputs to PCA panels only work when they are present)
    // We check if (!panelExists(addr)) for PCA writes 
    hasECM        = panelExists(0x22);
    hasMasterARM  = panelExists(0x5B);
    hasBrain      = panelExists(0x26);

    // After PCA9555_scanConnectedPanels() and hasECM/hasMasterARM/hasBrain assignments
    PanelRegistry_setActive(PanelKind::ECM,       hasECM);
    PanelRegistry_setActive(PanelKind::MasterARM, hasMasterARM);
    PanelRegistry_setActive(PanelKind::Brain,     hasBrain);
    // We already PanelRegistry_setActive for AnalogGauge in GPIO.cpp 
  
    // Show what PCA panels were discovered
    printDiscoveredPanels();
}

void initializeDisplays() {

  PanelRegistry_forEachDisplayInit();

  // Any custom displays should be initialized here
}

void initializeLEDs() {
    if (hasMasterARM) PCA9555_autoInitFromLEDMap(0x5B);
    if (hasECM) PCA9555_autoInitFromLEDMap(0x22);
    if (hasBrain) PCA9555_autoInitFromLEDMap(0x26);

    if (hasLockShoot) {
        debugPrintln("✅ Lock/Shoot detected, initializing...");
        WS2812_init();
    } else {
        debugPrintln("⚠️ Lock/Shoot NOT detected!");
    }

    if (hasCA) {
        debugPrintln("✅ Caution Advisory detected, initializing...");
        GN1640_init(GLOBAL_CLK_PIN, CA_DIO_PIN);
    } else {
        debugPrintln("⚠️ Caution Advisory NOT detected!");
    }

    if (hasLA && hasRA) {
        debugPrintln("✅ Left & Right Annunciators detected, initializing...");
        tm1637_init(LA_Device, GLOBAL_CLK_PIN, LA_DIO_PIN);
        tm1637_init(RA_Device, GLOBAL_CLK_PIN, RA_DIO_PIN);
    }
    else if (hasLA) {
        debugPrintln("✅ Left Annunciator detected, initializing...");
        tm1637_init(LA_Device, GLOBAL_CLK_PIN, LA_DIO_PIN);
    }
    else if (hasRA) {
        debugPrintln("✅ Right Annunciator detected, initializing...");
        tm1637_init(RA_Device, GLOBAL_CLK_PIN, RA_DIO_PIN);
    } else {
        debugPrintln("⚠️ No Annunciators detected!");
    }

    // Annunciators flash
    if (hasLA && hasRA) { tm1637_allOn(RA_Device); tm1637_allOn(LA_Device); delay(1000); tm1637_allOff(RA_Device); tm1637_allOff(LA_Device); }
    else if (hasLA) { tm1637_allOn(LA_Device); delay(1000); tm1637_allOff(LA_Device); }
    else if (hasRA) { tm1637_allOn(RA_Device); delay(1000); tm1637_allOff(RA_Device); }

    // Flash sequence
    if (hasCA) { GN1640_allOn(); delay(1000); GN1640_allOff(); }
    if (hasLockShoot) { WS2812_allOn(Green); delay(1000); WS2812_allOff(); }

    // PCA9555 devices
    if (hasMasterARM) { PCA9555_allOn(0x5B); delay(1000); PCA9555_allOff(0x5B); }
    if (hasECM) { PCA9555_allOn(0x22); delay(1000); PCA9555_allOff(0x22); }

    // GPIO LEDs
    preconfigureGPIO();
    GPIO_setAllLEDs(true); delay(1000); GPIO_setAllLEDs(false);

    // Always init GPIO 0 (ESP32S2) for testing and debugging
    // pinMode(0, INPUT_PULLUP);

    // Any custom outputs such as LEDs, Analog gauges or mechanical levers should go here.
}

void initializePanels(bool force) {
    if(!mainLoopStarted && !force) return;
    debugPrintln("Syncronizing Panel state....");

    PanelRegistry_forEachInit();
    // Your custom panels init routine should go here

    if (!isModeSelectorDCS()) HIDManager_commitDeferredReport("All devices");
}

void panelLoop() {

    PanelRegistry_forEachLoop();
    PanelRegistry_forEachDisplayLoop();
    PanelRegistry_forEachTick();

    // Your custom panels loop/tick routines should go here

    #if DEBUG_USE_WIFI && WIFI_DEBUG_USE_RINGBUFFER
    wifiDebugDrainSendBuffer();
    #endif

    #if SERIAL_DEBUG_USE_RINGBUFFER
    sendPendingSerial();
    #endif

    if (isPCA9555LoggingEnabled()) {
        for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
            uint8_t addr = discoveredDevices[i].address;
            byte p0, p1;
            readPCA9555(addr, p0, p1);
        }
    }
}