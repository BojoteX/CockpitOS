// Mappings.cpp - Panel configuration and runtime presence logic

#include "src/Globals.h"
#include "Mappings.h"
#include "src/HIDManager.h"

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "src/WiFiDebug.h"
#endif

// TM1637 device instances (must match externs)
TM1637Device RA_Device;
TM1637Device LA_Device;

// ---- Panel presence flags (all initialized to false, set in initMappings) ----
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
bool hasRightPanelController  = false;
// Add more runtime panel conditionals here when adding custom panels/other aircraft

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

// -- Panel Modules --
#if defined(LABEL_SET_MAIN) || defined(LABEL_SET_ALL) 
  #include "src/LeftAnnunciator.h"
  #include "src/RightAnnunciator.h"
  #include "src/IRCoolPanel.h"
  #include "src/MasterARMPanel.h"
#endif
#if defined(LABEL_SET_ALR67) || defined(LABEL_SET_ALL)
  #include "src/ALR67Panel.h"
  #include "src/ECMPanel.h"
#endif
#if defined(LABEL_SET_IFEI_NO_VIDEO) || defined(LABEL_SET_ALL)
  #include "src/IFEIPanel.h"
#endif
#if defined(LABEL_SET_BATTERY_GAUGE) || defined(LABEL_SET_ALL)
  #include "src/TFT_Gauges.h"
#endif
#if defined(LABEL_SET_RIGHT_PANEL_CONTROLLER) || defined(LABEL_SET_ALL)
  #include "src/TFT_Gauges.h"
  #include "src/RightPanelController.h"
#endif

// Don't forget to include headers for any custom panels added

void initMappings() {

    // Enable flags for panels present in this label set
    #if defined(LABEL_SET_ALR67) || defined(LABEL_SET_ALL)
      hasALR67   = true;
    #endif
    #if defined(LABEL_SET_MAIN) || defined(LABEL_SET_ALL)
      hasCA        = true;
      hasLA        = true;
      hasRA        = true;
      hasIR        = true;
      hasLockShoot = true;
    #endif
    #if defined(LABEL_SET_IFEI_NO_VIDEO) || defined(LABEL_SET_ALL)
      hasIFEI = true;
    #endif
    #if defined(LABEL_SET_BATTERY_GAUGE) || defined(LABEL_SET_ALL)
      hasTFTBattGauge = true;
    #endif
    #if defined(LABEL_SET_RIGHT_PANEL_CONTROLLER) || defined(LABEL_SET_ALL)
      hasRightPanelController = true;
      hasTFTBattGauge = true;
    #endif
    #if defined(LABEL_SET_ALTIMETER) || defined(LABEL_SET_ALL)
      // If needed, set specific panels
    #endif
    // Any conditional logic for custom panels should be nested above

    // Runs a discovery routine to check for PCA panels automatically, but they still need to be added manually in kPanels[] (see Mappings.h)
    debugPrintf("Using SDA %d and SCL %d for I2C\n", SDA_PIN, SCL_PIN);
    PCA9555_scanConnectedPanels();

    // ---- Runtime detection ---- (This is used so that outputs to PCA panels only work when they are present)
    // We check if (!panelExists(addr)) for PCA writes 
    hasECM        = panelExists(0x22);
    hasBrain      = panelExists(0x26);
    hasMasterARM  = panelExists(0x5B);
    
    // Show what PCA panels were discovered
    printDiscoveredPanels();
}

void initializeDisplays() {
  #if defined(LABEL_SET_IFEI_NO_VIDEO) || defined(LABEL_SET_ALL)
    if (hasIFEI) IFEIDisplay_init();
  #endif

    // Why init it here and not panels? because Display's should be INITIALIZED once! not on every mission start
  #if defined(LABEL_SET_BATTERY_GAUGE)  || defined(LABEL_SET_RIGHT_PANEL_CONTROLLER) || defined(LABEL_SET_ALL)
    if (hasTFTBattGauge) BatteryGauge_init();
  #endif
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

  // This is not required, but you can wrap this calls with pre-processors if you plan to "disable" panels by moving them out of the Panels directory. 
  // This one is just as an example. this is needed so the linker wont give you an error for functions that are not present anymore (even if not used)
  #if defined(LABEL_SET_IFEI_NO_VIDEO) || defined(LABEL_SET_ALL)
    if (hasIFEI) IFEI_init();
  #endif

  #if defined(LABEL_SET_ALR67) || defined(LABEL_SET_ALL)
    if (hasALR67) ALR67_init();
    if (hasECM) ECM_init();
  #endif

  #if defined(LABEL_SET_MAIN) || defined(LABEL_SET_ALL)
    if (hasRA) RightAnnunciator_init();
    if (hasLA) LeftAnnunciator_init();
    if (hasIR) IRCool_init();
    if (hasMasterARM) MasterARM_init();
  #endif

  #if defined(LABEL_SET_RIGHT_PANEL_CONTROLLER) || defined(LABEL_SET_ALL)
    if (hasRightPanelController) RightPanelButtons_init();
  #endif  

  // Your custom panels init routine should go here

    if (!isModeSelectorDCS()) HIDManager_commitDeferredReport("All devices");
}

void panelLoop() {

  #if defined(LABEL_SET_ALR67) || defined(LABEL_SET_ALL)
    if (hasALR67) ALR67_loop();
    if (hasECM) ECM_loop();
  #endif

  #if defined(LABEL_SET_MAIN) || defined(LABEL_SET_ALL)
    if (hasLA) LeftAnnunciator_loop();
    if (hasRA) RightAnnunciator_loop();
    if (hasIR) IRCool_loop();
    if (hasMasterARM) MasterARM_loop();
  #endif

  #if defined(LABEL_SET_IFEI_NO_VIDEO) || defined(LABEL_SET_ALL)
    if (hasIFEI) IFEI_loop();
    if (hasIFEI) IFEIDisplay_loop();
  #endif

  #if defined(LABEL_SET_BATTERY_GAUGE) || defined(LABEL_SET_ALL)
    if (hasTFTBattGauge) BatteryGauge_loop();
  #endif

  #if defined(LABEL_SET_RIGHT_PANEL_CONTROLLER) || defined(LABEL_SET_ALL)
    if (hasTFTBattGauge) BatteryGauge_loop();
    if (hasRightPanelController) RightPanelButtons_loop();
  #endif

  #if defined(LABEL_SET_MAIN) || defined(LABEL_SET_ALL)
    if (hasCA) GN1640_tick();
    if (hasLA || hasRA) tm1637_tick();
    if (hasLockShoot) WS2812_tick();
  #endif

  #if defined(LABEL_SET_ALR67) || defined(LABEL_SET_ALL)
    if (hasGauge) AnalogG_tick();
  #endif
  // Your custom panels loop routine should go here

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