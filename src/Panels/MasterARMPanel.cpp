// MasterARMPanel.cpp

#include "../Globals.h"
#include "../MasterARMPanel.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // Needed for #defines in Mappings.h 

// Cache local del estado
static byte prevMasterPort0 = 0xFF;

enum Port0Bits {
  MASTER_ARM_AG     = 0,  // Momentary (LOW = pressed)
  MASTER_ARM_AA     = 1,  // Momentary
  MASTER_ARM_DISCH  = 2,  // Momentary
  MASTER_ARM_SWITCH = 3   // ON = HIGH, OFF = LOW
};

void MasterARM_init() {
  delay(50);  // Small delay to ensure when init is called DCS has settled

  // "impossible" value to force controller reset and cache invalidation
  prevMasterPort0 = 0xAA; 

  byte port0, port1;
  if (readPCA9555(MASTERARM_PCA_ADDR, port0, port1)) {
    prevMasterPort0 = port0;

    // 2-pos switch (OFF / ON)
    HIDManager_setNamedButton(
      bitRead(port0, MASTER_ARM_SWITCH) ? "MASTER_ARM_SW_ARM" : "MASTER_ARM_SW_SAFE",
      true
    );

    // Momentary buttons: detect state at startup (optional)
    HIDManager_setNamedButton("MASTER_MODE_AG",     true, !bitRead(port0, MASTER_ARM_AG));
    HIDManager_setNamedButton("MASTER_MODE_AA",     true, !bitRead(port0, MASTER_ARM_AA));
    HIDManager_setNamedButton("FIRE_EXT_BTN",       true, !bitRead(port0, MASTER_ARM_DISCH));

    // HIDManager_commitDeferredReport("Master ARM Panel");

    debugPrintf("✅ Initialized Master ARM Panel\n",MASTERARM_PCA_ADDR);
  } else {
    debugPrintf("❌ Could not initialize Master ARM Panel\n",MASTERARM_PCA_ADDR);
  }
}

void MasterARM_loop() {

  // Adjust polling rate
  static unsigned long lastMasterARMPoll = 0;
  if (!shouldPollMs(lastMasterARMPoll)) return;

  byte port0, port1;
  if (!readPCA9555(MASTERARM_PCA_ADDR, port0, port1)) return;

  // 2-position switch (OFF / ON)
  if (bitRead(prevMasterPort0, MASTER_ARM_SWITCH) != bitRead(port0, MASTER_ARM_SWITCH)) {
    HIDManager_setNamedButton(
      bitRead(port0, MASTER_ARM_SWITCH) ? "MASTER_ARM_SW_ARM" : "MASTER_ARM_SW_SAFE"
    );
  }

  // Momentary buttons
  if (bitRead(prevMasterPort0, MASTER_ARM_AG) != bitRead(port0, MASTER_ARM_AG)) {
    HIDManager_setNamedButton("MASTER_MODE_AG", false, !bitRead(port0, MASTER_ARM_AG));
  }

  if (bitRead(prevMasterPort0, MASTER_ARM_AA) != bitRead(port0, MASTER_ARM_AA)) {
    HIDManager_setNamedButton("MASTER_MODE_AA", false, !bitRead(port0, MASTER_ARM_AA));
  }

  if (bitRead(prevMasterPort0, MASTER_ARM_DISCH) != bitRead(port0, MASTER_ARM_DISCH)) {
    HIDManager_setNamedButton("FIRE_EXT_BTN", false, !bitRead(port0, MASTER_ARM_DISCH));
  }

  prevMasterPort0 = port0;
}