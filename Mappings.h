#pragma once

// Global GPIO centralized PIN assignments. This ones should be carefully MANAGED. For the most part, they DO NOT need to change, even with potential conflicts (e.g strobe-and-read matrix method used by ALR67) which uses some of the same PINs this will NOT be an issue as ALR67 should be configured as a SINGLE firmware set and NOT mixed with other panels (except ECM or Analog Cockpit Altimiter) 

#ifndef SDA_PIN
#define SDA_PIN 8 // This could be overriden from a panel file if needed
#endif

#ifndef SCL_PIN
#define SCL_PIN 9 // This could be overriden from a panel file if needed
#endif

#define GLOBAL_CLK_PIN                            37 // Clock Pin
#define CA_DIO_PIN                                36 // Caution Advisory DIO Pin
#define LA_DIO_PIN                                39 // Left Annunciator DIO Pin
#define LA_CLK_PIN                                GLOBAL_CLK_PIN // Left Annunciator Clock Pin
#define RA_DIO_PIN                                40 // Right Annunciator DIO Pin
#define RA_CLK_PIN                                GLOBAL_CLK_PIN // right Annunciator Clock Pin
#define LOCKSHOOT_DIO_PIN                         35 // Lock-Shoot Indicator DIO Pin
#define WS2812B_PIN                               LOCKSHOOT_DIO_PIN // Lock-Shoot Indicator DIO Pin

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
extern bool hasTFTSwitch;

// Give your PCA custom panel a basic short name and address where it is located (use the I2C scanner in Tools) 
enum class PanelID : uint8_t { 
   ECM      = 0x22, // ECM Panel bundled with ALR-67
   BRAIN    = 0x26, // TEK Brain Controller (IR Cool uses the PCA on the brain, IRCOOL itself its a dumb panel)
   CUSTOM   = 0x27, // Used for testing simulating a panel (for the Tutorial)
   ARM      = 0x5B, // Master ARM, 0x5B is non-standard (PCA clone? hack?) don't know, don't care
   UNKNOWN  = 0x00  // Placeholder
};

// —— Source panel table —— (dodnt forget to update the ENUM above, here you give it a friendly name) 
struct PanelDef { uint8_t addr; PanelID id; const char* label; };
static constexpr PanelDef kPanels[] = {
  { 0x22, PanelID::ECM,    "ECM Panel"             },
  { 0x26, PanelID::BRAIN,  "Brain / IRCool Panel"  },
  { 0x27, PanelID::CUSTOM, "Custom Panel"          },
  { 0x5B, PanelID::ARM,    "Master Arm Panel"      },
};

PanelID     getPanelID(uint8_t address);
const char* panelIDToString(PanelID id);
const char* getPanelName(uint8_t addr);  // Declaration

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

   #undef SDA_PIN
   #undef SCL_PIN
   #define SDA_PIN 33
   #define SCL_PIN 35

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
