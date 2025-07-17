// HEADER ESP32S2
// PIN 1	3V3
// PIN 2	GND
// PIN 3	GPIOs 14, 17, 18 (ALIVE BUT NOT USED) 
// PIN 4	GPIOs 14, 17, 18 (ALIVE BUT NOT USED)
// PIN 5	GPIO 6
//
// Shared across all panels GPIO 8 (SDA) and GPIO 9 (SCL)

// ECMPanel.cpp

#include "../Globals.h"
#include "../ECMPanel.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // Needed for #defines in Mappings.h 

// Track previous PCA9555 state for ECM panel only
static byte prevECMPort0 = 0xFF;
static byte prevECMPort1 = 0xFF;

// Port 0 Bit Assignments (from PCA9555 @ 0x22)
enum Port0Bits {
  JETT_SEL = 0,             // Momentary button
  AUX_REL = 1,              // 2-position switch
  DISPENSER_BYPASS = 2,     // 3-position group
  DISPENSER_OFF = 3,
  ECM_OFF = 4,              // EMC rotary selector (4-pos group)
  ECM_STBY = 5,
  ECM_BIT = 6,
  ECM_REC = 7
};

// Initializes panel by reading its current state and deferring USB report
void ECM_init() {
  // delay(50);  // Small delay to ensure when init is called DCS has settled

  // "impossible" value to force controller reset and cache invalidation
  prevECMPort0 = 0xAA;
  prevECMPort1 = 0xAA;

  byte port0, port1;
  if (readPCA9555(ECM_PCA_ADDR, port0, port1)) {
    prevECMPort0 = port0;
    prevECMPort1 = port1;

    if (!bitRead(port0, JETT_SEL)) {
      HIDManager_setToggleNamedButton("CMSD_JET_SEL_BTN", true); // deferSend = true
    }
    
    // DISPENSER 3-pos group: OFF / BYPASS / ON (default)
    if (!bitRead(port0, DISPENSER_OFF))
      HIDManager_setNamedButton("CMSD_DISPENSE_SW_OFF", true);
    else if (!bitRead(port0, DISPENSER_BYPASS))
      HIDManager_setNamedButton("CMSD_DISPENSE_SW_BYPASS", true);
    else
      HIDManager_setNamedButton("CMSD_DISPENSE_SW_ON", true);

    // AUX_REL 2-pos switch: ENABLE / NORM
    HIDManager_setNamedButton(
      bitRead(port0, AUX_REL) ? "AUX_REL_SW_ENABLE" : "AUX_REL_SW_NORM", true
    );

    // EMC Selector (4-pos rotary via 4 bits)
    if (!bitRead(port0, ECM_OFF))
      HIDManager_setNamedButton("ECM_MODE_SW_OFF", true);
    else if (!bitRead(port0, ECM_STBY))
      HIDManager_setNamedButton("ECM_MODE_SW_STBY", true);
    else if (!bitRead(port0, ECM_BIT))
      HIDManager_setNamedButton("ECM_MODE_SW_BIT", true);
    else if (!bitRead(port0, ECM_REC))
      HIDManager_setNamedButton("ECM_MODE_SW_REC", true);
    else if (!bitRead(port1, 0))
      HIDManager_setNamedButton("ECM_MODE_SW_XMIT", true);

    // Commit all deferred button reports at once
    // HIDManager_commitDeferredReport("ECM Panel");

    debugPrintf("✅ Initialized ECM Panel\n",ECM_PCA_ADDR);
  } else {
    debugPrintf("❌ Could not initialize ECM Panel\n",ECM_PCA_ADDR);
  }
}

// Runtime loop for polling and reacting to state changes
void ECM_loop() {
  // Adjust polling rate
  static unsigned long lastECMPoll = 0;
  if (!shouldPollMs(lastECMPoll)) return;

  byte port0, port1;
  if (!readPCA9555(ECM_PCA_ADDR, port0, port1)) return;

  // JETT_SEL - Momentary press detection
  bool currJettsel = !bitRead(port0, JETT_SEL); // Inverted logic: pressed = true
  HIDManager_toggleIfPressed(currJettsel, "CMSD_JET_SEL_BTN");

  // DISPENSER 3-pos switch
  bool currDispBypass = bitRead(port0, DISPENSER_BYPASS);
  bool currDispOff    = bitRead(port0, DISPENSER_OFF);
  if ((bitRead(prevECMPort0, DISPENSER_BYPASS) != currDispBypass) ||
      (bitRead(prevECMPort0, DISPENSER_OFF) != currDispOff)) {
    if (!currDispOff)
      HIDManager_setNamedButton("CMSD_DISPENSE_SW_OFF");
    else if (!currDispBypass)
      HIDManager_setNamedButton("CMSD_DISPENSE_SW_BYPASS");
    else
      HIDManager_setNamedButton("CMSD_DISPENSE_SW_ON");
  }

  // AUX_REL 2-pos switch
  bool currAuxRel = bitRead(port0, AUX_REL);
  if (bitRead(prevECMPort0, AUX_REL) != currAuxRel) {
    HIDManager_setNamedButton(currAuxRel ? "AUX_REL_SW_ENABLE" : "AUX_REL_SW_NORM");
  }

  // EMC rotary selector (OFF / STBY / BIT / REC)
  for (int bit = ECM_OFF; bit <= ECM_REC; bit++) {
    bool currSelectorBit = bitRead(port0, bit);
    if (bitRead(prevECMPort0, bit) != currSelectorBit) {
      if (!currSelectorBit) {
        switch (bit) {
          case ECM_OFF:  HIDManager_setNamedButton("ECM_MODE_SW_OFF"); break;
          case ECM_STBY: HIDManager_setNamedButton("ECM_MODE_SW_STBY"); break;
          case ECM_BIT:  HIDManager_setNamedButton("ECM_MODE_SW_BIT"); break;
          case ECM_REC:  HIDManager_setNamedButton("ECM_MODE_SW_REC"); break;
        }
      }
    }
  }

  // EMC XMIT (bit 0 of PORT1)
  bool currXmit = bitRead(port1, 0);
  if (bitRead(prevECMPort1, 0) != currXmit) {
    if (!currXmit) {
      HIDManager_setNamedButton("ECM_MODE_SW_XMIT");
    }
  }

  // Update previous states
  prevECMPort0 = port0;
  prevECMPort1 = port1;
}