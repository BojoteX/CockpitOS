// Mappings.cpp - Specific to your aircraft of choice

#include "../Globals.h"
#include "../Mappings.h"
#include "../HIDManager.h" // Needed for _init and _loop for HIDManager

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "../WiFiDebug.h" // Only WiFi
#endif

/*
// PCA Only panels will be autodetected, we init to false
bool hasBrain     = false; // Brain Controller PCB (e.g TEK Brain uses PCA9555 0x26 for IRCool) autodetected by our program.
bool hasECM       = false; // Panel uses PCA9555 0x22, it is autodetected by our program
bool hasMasterARM = false; // Panel uses 0x5B, it is autodetected by our program
*/

// -- Panel Modules --
#include "../LeftAnnunciator.h"
#include "../RightAnnunciator.h"
#include "../ECMPanel.h"
#include "../IRCoolPanel.h"
#include "../MasterARMPanel.h"
#include "../ALR67Panel.h"

#include "../IFEIPanel.h"

bool hasGauge = false;

// Initialize Displays if present
void initializeDisplays() {
    if (hasIFEI) IFEIDisplay_init();
}

// Initialize all LEDs and their associated devices
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

    // Annunciators
    if (hasLA && hasRA) { tm1637_allOn(RA_Device); tm1637_allOn(LA_Device); delay(1000); tm1637_allOff(RA_Device); tm1637_allOff(LA_Device); }
    else if (hasLA) { tm1637_allOn(LA_Device); delay(1000); tm1637_allOff(LA_Device); }
    else if (hasRA) { tm1637_allOn(RA_Device); delay(1000); tm1637_allOff(RA_Device); }

    // Flash sequence
    if (hasCA) { GN1640_allOn(); delay(1000); GN1640_allOff(); }
    if (hasLockShoot) { WS2812_allOn(Green); delay(1000); WS2812_allOff(); }

	// PCA9555 devices
    if (hasMasterARM) { PCA9555_allOn(0x5B); delay(1000); PCA9555_allOff(0x5B); }
    if (hasECM) { PCA9555_allOn(0x22); delay(1000); PCA9555_allOff(0x22); }

    // Init GPIO LEDs and turn them all ON then OFF
    preconfigureGPIO();
    GPIO_setAllLEDs(true); delay(1000); GPIO_setAllLEDs(false);

    // Always init GPIO 0 (ESP32S2) for testing and debugging. Comment if it conflicts
    pinMode(0, INPUT_PULLUP); // Configure GPIO 0 as input and use it like this: bool pressed = (digitalRead(0) == LOW);
}

void initMappings() {

    // Runs a discovery routine to check for PCA panels
    PCA9555_scanConnectedPanels();

    // Set flags if panels were discovered.
    // hasBrain     = panelExists(0x26);
    // hasECM       = panelExists(0x22);
    // hasMasterARM = panelExists(0x5B);

    // Just for reference, measures PCA9555 bus speed for brain controller (if present)
    // if(hasBrain) measureI2Cspeed(0x26);

    // Show what PCA Panels were discovered
    printDiscoveredPanels();
}

// Reads panel current state during init
void initializePanels(bool force) {

  if(!mainLoopStarted && !force) return;

  // Syncronize your active panels states
  debugPrintln("Syncronizing Panel state....");

  if (hasIFEI) IFEI_init();    
  if (hasALR67) ALR67_init();    
  if (hasRA) RightAnnunciator_init();
  if (hasLA) LeftAnnunciator_init();
  if (hasIR) IRCool_init();
  if (hasECM) ECM_init();
  if (hasMasterARM) MasterARM_init();  

  // Consolidated report for all devices
  if (!isModeSelectorDCS()) HIDManager_commitDeferredReport("All devices");
}

void panelLoop() {
    // Regular Panels
    if (hasALR67) ALR67_loop();      
    if (hasLA) LeftAnnunciator_loop();
    if (hasRA) RightAnnunciator_loop();
    if (hasIR) IRCool_loop();  
    if (hasECM) ECM_loop();
    if (hasMasterARM) MasterARM_loop();    

	// Displays (IFEI)
    if (hasIFEI) IFEI_loop(); 
    if (hasIFEI) IFEIDisplay_loop(); // IFEI Display loop (if present)

    // Ticks
    if (hasCA) GN1640_tick(); // Shadow buffer implementation for Caution Advisory
    if (hasLA || hasRA) tm1637_tick(); 
    if (hasLockShoot) WS2812_tick();    
    if (hasGauge) AnalogG_tick();

    // Misc tasks
    
    // Drains the SEND buffer for pending outgoing UDP messages (mostly used for debugging and VERBOSE output)
    #if DEBUG_USE_WIFI && WIFI_DEBUG_USE_RINGBUFFER
    wifiDebugDrainSendBuffer();
    #endif

    #if SERIAL_DEBUG_USE_RINGBUFFER
    // Drains the SEND buffer for pending outgoing Serial messages (mostly used for debugging and VERBOSE output)
    sendPendingSerial();
    #endif    

    // Only when DEBUG is enabled we scan port/bit/position when the user clicks/flips switches on a PCA9555 detected device 
    if (isPCA9555LoggingEnabled()) {
        if (hasBrain) {
            byte p0, p1;
            readPCA9555(0x26, p0, p1);
        }
        if (hasECM) {
            byte p0, p1;
            readPCA9555(0x22, p0, p1);
        }
        if (hasMasterARM) {
            byte p0, p1;
            readPCA9555(0x5B, p0, p1);
        }
    }
}