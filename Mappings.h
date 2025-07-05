#pragma once

void initMappings();
void initializePanels(bool force);
void panelLoop();
void initializeLEDs();
void initializeDisplays();

// --- Panel presence flags (all runtime, set in initMappings) ---
extern bool hasBrain;
extern bool hasECM;
extern bool hasMasterARM;
extern bool hasIFEI;
extern bool hasALR67;
extern bool hasCA;
extern bool hasLA;
extern bool hasRA;
extern bool hasIR;
extern bool hasLockShoot;

// —— Single‐source panel table —— 
struct PanelDef { uint8_t addr; PanelID id; const char* label; };
static constexpr PanelDef kPanels[] = {
  { 0x22, PanelID::ECM,   "ECM Panel"            },
  { 0x26, PanelID::BRAIN, "Brain / IRCool Panel" },
  { 0x5B, PanelID::ARM,   "Master Arm Panel"     },
};

// --- Per-LABEL_SET logic and includes ---
// Just like before: include the right generated mapping files for this build
#if defined(LABEL_SET_ALR67)

   #define LABEL_SET "RWR ALR-67"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_ALR67/CT_Display.h"
   #include "src/LABELS/LABEL_SET_ALR67/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_ALR67/InputMapping.h"
   #include "src/LABELS/LABEL_SET_ALR67/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_ALR67/DisplayMapping.h"

#elif defined(LABEL_SET_ALTIMETER)

   #define LABEL_SET "Cockpit Altimeter"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_ALTIMETER/CT_Display.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/InputMapping.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/DisplayMapping.h"

#elif defined(LABEL_SET_ALL)

   #define LABEL_SET "ALL (Debug Only)"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_ALL/CT_Display.h"
   #include "src/LABELS/LABEL_SET_ALL/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_ALL/InputMapping.h"
   #include "src/LABELS/LABEL_SET_ALL/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_ALL/DisplayMapping.h"

#elif defined(LABEL_SET_IFEI_NO_VIDEO)

   #define LABEL_SET "IFEI (No Video)"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/CT_Display.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/InputMapping.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/DisplayMapping.h"

#elif defined(LABEL_SET_MAIN)

   #define LABEL_SET "Main Instruments"
   #define HAS_HID_MODE_SELECTOR 1
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_MAIN/CT_Display.h"
   #include "src/LABELS/LABEL_SET_MAIN/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_MAIN/InputMapping.h"
   #include "src/LABELS/LABEL_SET_MAIN/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_MAIN/DisplayMapping.h"

#elif defined(LABEL_SET_BATTERY_GAUGE)

   #define LABEL_SET "Battery Gauge"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/CT_Display.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/InputMapping.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/DisplayMapping.h"

#elif defined(LABEL_SET_F16_TEST)

   #define LABEL_SET "F-16 test"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_F16_TEST/CT_Display.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/InputMapping.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/DisplayMapping.h"

#endif
