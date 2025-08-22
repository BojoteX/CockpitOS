// Mappings.cpp - Panel configuration and runtime presence logic

#include "src/Globals.h"
#include "Mappings.h"
#include "src/HIDManager.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "src/WiFiDebug.h"
#endif

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

    // Print registered panels
    int n = PanelRegistry_count();
    for (int i=0; i<PanelRegistry_count(); ++i)
      debugPrintf("Registered Panel: %s\n", PanelRegistry_labelAt(i));

    PCA9555_scanConnectedPanels();

    PanelRegistry_setActive(PanelKind::ECM, panelExists(0x22));
    PanelRegistry_setActive(PanelKind::MasterARM, panelExists(0x5B));
    PanelRegistry_setActive(PanelKind::Brain, panelExists(0x26));
  
    // Show what PCA panels were discovered
    printDiscoveredPanels();
}

// Runs only once when device starts, never again
void initializeDisplays() {
  PanelRegistry_forEachDisplayInit();
}

void initializeLEDs() {
    if (PanelRegistry_has(PanelKind::MasterARM)) PCA9555_autoInitFromLEDMap(0x5B);
    if (PanelRegistry_has(PanelKind::ECM)) PCA9555_autoInitFromLEDMap(0x22);
    if (PanelRegistry_has(PanelKind::Brain)) PCA9555_autoInitFromLEDMap(0x26);

    if (PanelRegistry_has(PanelKind::LockShoot)) {
        debugPrintln("✅ Lock/Shoot detected, initializing...");
        WS2812_init();
    } else {
        debugPrintln("⚠️ Lock/Shoot NOT detected!");
    }

    if (PanelRegistry_has(PanelKind::CA)) {
        debugPrintln("✅ Caution Advisory detected, initializing...");
        GN1640_init(GLOBAL_CLK_PIN, CA_DIO_PIN);
    } else {
        debugPrintln("⚠️ Caution Advisory NOT detected!");
    }

    if (PanelRegistry_has(PanelKind::LA) && PanelRegistry_has(PanelKind::RA)) {
        debugPrintln("✅ Left & Right Annunciators detected, initializing...");
        tm1637_init(LA_Device, GLOBAL_CLK_PIN, LA_DIO_PIN);
        tm1637_init(RA_Device, GLOBAL_CLK_PIN, RA_DIO_PIN);
    }
    else if (PanelRegistry_has(PanelKind::LA)) {
        debugPrintln("✅ Left Annunciator detected, initializing...");
        tm1637_init(LA_Device, GLOBAL_CLK_PIN, LA_DIO_PIN);
    }
    else if (PanelRegistry_has(PanelKind::RA)) {
        debugPrintln("✅ Right Annunciator detected, initializing...");
        tm1637_init(RA_Device, GLOBAL_CLK_PIN, RA_DIO_PIN);
    } else {
        debugPrintln("⚠️ No Annunciators detected!");
    }

    // Annunciators flash
    if (PanelRegistry_has(PanelKind::LA) && PanelRegistry_has(PanelKind::RA)) { tm1637_allOn(RA_Device); tm1637_allOn(LA_Device); delay(1000); tm1637_allOff(RA_Device); tm1637_allOff(LA_Device); }
    else if (PanelRegistry_has(PanelKind::LA)) { tm1637_allOn(LA_Device); delay(1000); tm1637_allOff(LA_Device); }
    else if (PanelRegistry_has(PanelKind::RA)) { tm1637_allOn(RA_Device); delay(1000); tm1637_allOff(RA_Device); }

    // Flash sequence
    if (PanelRegistry_has(PanelKind::CA)) { GN1640_allOn(); delay(1000); GN1640_allOff(); }
    if (PanelRegistry_has(PanelKind::LockShoot)) { WS2812_allOn(Green); delay(1000); WS2812_allOff(); }

    // PCA9555 devices
    if (PanelRegistry_has(PanelKind::MasterARM)) { PCA9555_allOn(0x5B); delay(1000); PCA9555_allOff(0x5B); }
    if (PanelRegistry_has(PanelKind::ECM)) { PCA9555_allOn(0x22); delay(1000); PCA9555_allOff(0x22); }

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

    if (isPCA9555LoggingEnabled()) {
        for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
            uint8_t addr = discoveredDevices[i].address;
            byte p0, p1;
            readPCA9555(addr, p0, p1);
        }
    }
}