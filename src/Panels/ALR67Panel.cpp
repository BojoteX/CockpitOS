// ALR67Panel.cpp - ALR-67 Controller Panel

// The 4 Pins for the Cabin Altitude add-on future expansion are (from left to right): 18, 36, 39, 40

//  This implementation reads a 5-position rotary selector using a simple strobe-and-read matrix method.
//  - Four GPIOs are used as strobes: GPIO 16, 17, 21, 37
//  - One shared GPIO (GPIO 38) acts as the common return line

//  The rotary switch internally routes only one strobe to the shared return line based on its position.
//  We scan each strobe in sequence, pull it LOW, and check if GPIO 38 goes LOW — if it does,
//  we set the corresponding bit in a pattern. This pattern maps directly to a rotary position.

//  This method requires no external components and supports multiple rotaries as long as they are polled separately.

#include "../Globals.h"
#include "../ALR67Panel.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // Needed for #defines in Mappings.h 

const int ALR67_Strobes[] = { ALR67_STROBE_1, ALR67_STROBE_2, ALR67_STROBE_3, ALR67_STROBE_4 };
const int ALR67_StrobeCount = sizeof(ALR67_Strobes) / sizeof(ALR67_Strobes[0]);

// Rotary Matrix Cache
static uint8_t prevPattern = 0xFF;

// HC165 Cache
static uint64_t prevButtonBits = 0xFF;
static uint64_t buttonBits = 0xFF;

void dispatchRotaryPosition(uint8_t pattern) {
  if (pattern == 0x08) {
    HIDManager_setNamedButton("RWR_DIS_TYPE_SW_F", false, true); // _N
  } else if (pattern == 0x04) {
    HIDManager_setNamedButton("RWR_DIS_TYPE_SW_U", false, true); // _I
  } else if (pattern == 0x02) {
    HIDManager_setNamedButton("RWR_DIS_TYPE_SW_A", false, true); // _A
  } else if (pattern == 0x01) {
    HIDManager_setNamedButton("RWR_DIS_TYPE_SW_I", false, true); // _U
  } else if (pattern == 0x0F) {
    HIDManager_setNamedButton("RWR_DIS_TYPE_SW_N", false, true); // _F
  } else {
    // debugPrintf("⚠️ ALR67 unknown rotary pattern: 0x%02X\n", pattern);
  }
}

void ALR67_init() {
  // delay(50);  // Small delay to ensure when init is called DCS has settled

  // "impossible" value to force controller reset and cache invalidation
  prevPattern = 0xAA;        // Matrix rotary cache
  prevButtonBits = 0xAA;     // HC165 cache (for that panel's buttons)

  // Init HC165 Pins
  HC165_init(ALR67_HC165_PL, ALR67_HC165_CP, ALR67_HC165_QH, 8);  // PL, CP, QH

  pinMode(ALR67_DataPin, INPUT_PULLUP);
  for (int i = 0; i < ALR67_StrobeCount; i++) {
    pinMode(ALR67_Strobes[i], OUTPUT);
    digitalWrite(ALR67_Strobes[i], HIGH);
  }

  prevPattern = MatrixRotary_readPattern(ALR67_Strobes, ALR67_StrobeCount, ALR67_DataPin);
  dispatchRotaryPosition(prevPattern);

  HIDManager_moveAxis("RWR_AUDIO_CTRL", RWR_AUDIO_PIN, AXIS_SLIDER1,true);
  HIDManager_moveAxis("RWR_DMR_CTRL", RWR_DMR_PIN, AXIS_SLIDER2, true);

  // 74HC165 button inputs
  buttonBits = HC165_read();
 
  if ((prevButtonBits ^ buttonBits) & (1 << 4)) {
    HIDManager_setToggleNamedButton("RWR_POWER_BTN", true);
  }

  debugPrintln("✅ Initialized ALR-67 Panel");
}

void ALR67_loop() {
  static unsigned long lastALR67Poll = 0;
  if (!shouldPollMs(lastALR67Poll)) return;

  uint8_t pattern = MatrixRotary_readPattern(ALR67_Strobes, ALR67_StrobeCount, ALR67_DataPin);
  if (pattern != prevPattern) {
    prevPattern = pattern;
    dispatchRotaryPosition(pattern);
  }

  HIDManager_moveAxis("RWR_AUDIO_CTRL", RWR_AUDIO_PIN, AXIS_SLIDER1);
  HIDManager_moveAxis("RWR_DMR_CTRL", RWR_DMR_PIN, AXIS_SLIDER2);  

  // 74HC165 button inputs
  buttonBits = HC165_read();

  if ((prevButtonBits ^ buttonBits) & (1 << 0)) {
    HIDManager_setNamedButton("RWR_BIT_BTN", false, !(buttonBits & (1 << 0)));
  }
  if ((prevButtonBits ^ buttonBits) & (1 << 1)) {
    HIDManager_setNamedButton("RWR_OFFSET_BTN", false, !(buttonBits & (1 << 1)));
  }
  if ((prevButtonBits ^ buttonBits) & (1 << 2)) {
    HIDManager_setNamedButton("RWR_SPECIAL_BTN", false, !(buttonBits & (1 << 2)));
  }
  if ((prevButtonBits ^ buttonBits) & (1 << 3)) {
    HIDManager_setNamedButton("RWR_DISPLAY_BTN", false, !(buttonBits & (1 << 3)));
  }
  if ((prevButtonBits ^ buttonBits) & (1 << 4)) {
    HIDManager_toggleIfPressed(!(buttonBits & (1 << 4)), "RWR_POWER_BTN");    
  }

  prevButtonBits = buttonBits;
}