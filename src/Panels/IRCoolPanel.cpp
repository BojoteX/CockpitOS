// HEADER  ESP32S2
// PIN 1   GND/GND
// PIN 2   3V3
// PIN 3   GPIO 6  (Panel Backlight)
// PIN 4   GPIO 18 (HMD Knob)
// PIN 5   N/A
// PIN 6   N/A
// PIN 7   N/A
// PIN 8   GPIO 34 (SPIN RCVY LED)
//
// Shared across all panels GPIO 8 (SDA) and GPIO 9 (SCL)

// IRCoolPanel.cpp

#include "../Globals.h"
#include "../IRCoolPanel.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // Used for specific isCoverOpen logic from DCSBIOSBridge

// Previous state cache for change detection
static byte prevIRCPort0 = 0xFF;
static byte prevIRCPort1 = 0xFF;

// Analog pin connected to HMD Knob (Rx axis)
constexpr int HMD_KNOB_PIN = 18;

// Correct Enums
enum Port1Bits {
  IR_COOL_ORIDE = 0,   // LOW = ORIDE
  IR_COOL_OFF   = 1,   // LOW = OFF
  SPIN_RCVY     = 2    // LOW = RCVY (confirmed by trusted loop)
};

// Called each loop to monitor GPIO0 (SPIN button)
void updateSpinRecovery(bool deferredSend, bool pressed) {
    static bool prevPressed = false;

    if (pressed != prevPressed) {
        if (pressed) {
            // Button pressed: always open cover, then set RCVY
            HIDManager_setNamedButton("SPIN_RECOVERY_COVER", deferredSend, pressed);
            HIDManager_setNamedButton("SPIN_RECOVERY_SW_RCVY", deferredSend, pressed);
        } else {
            // Button released: set NORM, then close cover
            HIDManager_setNamedButton("SPIN_RECOVERY_SW_NORM", deferredSend, pressed);
            // Optional: flush HID report here if required for strict order
            HIDManager_setNamedButton("SPIN_RECOVERY_COVER", deferredSend, pressed);
        }
        prevPressed = pressed;
    }
}

void IRCool_init() {
  delay(50);  // Small delay to ensure when init is called DCS has settled

  // "impossible" value to force controller reset and cache invalidation
  prevIRCPort0 = 0xAA;
  prevIRCPort1 = 0xAA;

  byte port0, port1;
  if (readPCA9555(IRCOOL_PCA_ADDR, port0, port1)) {
    prevIRCPort0 = port0;
    prevIRCPort1 = port1;

    // SPIN switch and cosmetic cover sync on init
    bool currSpin = !bitRead(port1, SPIN_RCVY);
    // bool currSpin = (digitalRead(0) == LOW);
    updateSpinRecovery(true, currSpin);

    // IR COOL 3-position logic (PORT1 bits 0 & 1)
    if (!bitRead(port1, IR_COOL_OFF))
      HIDManager_setNamedButton("IR_COOL_SW_OFF", true);
    else if (!bitRead(port1, IR_COOL_ORIDE))
      HIDManager_setNamedButton("IR_COOL_SW_ORIDE", true);
    else
      HIDManager_setNamedButton("IR_COOL_SW_NORM", true);

    // Analog input for HMD knob
    HIDManager_moveAxis("HMD_OFF_BRT", HMD_KNOB_PIN, AXIS_RX);

    // Send deferred HID report
    // HIDManager_commitDeferredReport("IR Cool Panel");

    debugPrintf("✅ Initialized IR Cool Panel\n", IRCOOL_PCA_ADDR);
  } else {
    debugPrintf("❌ Could not initialize IR Cool Panel\n", IRCOOL_PCA_ADDR);
  }
}

void IRCool_loop() {
  static unsigned long lastIRCoolPoll = 0;
  if (!shouldPollMs(lastIRCoolPoll)) return;

  // Always read the HMD knob
  HIDManager_moveAxis("HMD_OFF_BRT", HMD_KNOB_PIN, AXIS_RX);

  byte port0, port1;
  if (!readPCA9555(IRCOOL_PCA_ADDR, port0, port1)) return;

  // Spin Recovery Handler
  bool currSpin = !bitRead(port1, SPIN_RCVY);
  // bool currSpin = (digitalRead(0) == LOW);
  updateSpinRecovery(false, currSpin);

  // IR COOL 3-position logic
  if ((bitRead(prevIRCPort1, IR_COOL_OFF) != bitRead(port1, IR_COOL_OFF)) ||
      (bitRead(prevIRCPort1, IR_COOL_ORIDE) != bitRead(port1, IR_COOL_ORIDE))) {
    if (!bitRead(port1, IR_COOL_OFF))
      HIDManager_setNamedButton("IR_COOL_SW_OFF");
    else if (!bitRead(port1, IR_COOL_ORIDE))
      HIDManager_setNamedButton("IR_COOL_SW_ORIDE");
    else
      HIDManager_setNamedButton("IR_COOL_SW_NORM");
  }

  prevIRCPort0 = port0;
  prevIRCPort1 = port1;
}