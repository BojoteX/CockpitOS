// RightAnnunciator.cpp
// Implementation for RIGHT annunciator button panel integration

#include "../Globals.h"
#include "../RightAnnunciator.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // Used for specific isCoverOpen logic from DCSBIOSBridge

// TM1637 device instances (must match externs)
TM1637Device RA_Device;

// Initialization routine for RIGHT annunciator buttons
void RightAnnunciator_init() {
    // delay(50);  // Small delay to ensure when init is called DCS has settled

    // If RIGHT_FIRE_BTN is already physically pressed
    if (!(tm1637_readKeys(RA_Device) & 0x01)) {
        const char* latched = isCoverOpen("RIGHT_FIRE_BTN_COVER")
            ? "RIGHT_FIRE_BTN"
            : "RIGHT_FIRE_BTN_COVER";
        HIDManager_setNamedButton(latched, true, true); // deferSend = true
    }

    // If APU_FIRE_BTN is physically pressed
    if (!(tm1637_readKeys(RA_Device) & 0x08)) {
        HIDManager_setToggleNamedButton("APU_FIRE_BTN", true); // deferSend = true
    }

    // HIDManager_commitDeferredReport("Right Annunciator");

    debugPrintln("✅ RIGHT Annunciator initialized for buttons");
}

void RightAnnunciator_loop() {
    static unsigned long lastRAPoll = 0;
    if (!shouldPollMs(lastRAPoll)) return;

    static uint16_t raSampleCounter = 0;
    static uint8_t prevFinalKeysRA = 0xFF;

    uint8_t finalKeys = 0;

    if (tm1637_handleSamplingWindow(RA_Device, raSampleCounter, finalKeys)) {
        if (finalKeys != prevFinalKeysRA) {
            // APU_FIRE_BTN (bit 3 = 0 when pressed) — NOT guarded
            bool currApu = !(finalKeys & 0x08);
            bool prevApu = !(prevFinalKeysRA & 0x08);
            if (currApu != prevApu) {
                HIDManager_toggleIfPressed(currApu, "APU_FIRE_BTN", false);
            }

            // RIGHT_FIRE_BTN (bit 0 = 0 when pressed) — 🔒 guarded
            bool currRightFire = !(finalKeys & 0x01);
            HIDManager_handleGuardedToggleIfPressed(currRightFire, "RIGHT_FIRE_BTN", "RIGHT_FIRE_BTN_COVER", false);

            prevFinalKeysRA = finalKeys;
        }
    }
}