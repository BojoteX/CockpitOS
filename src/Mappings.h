#pragma once

void initMappings();
void initializePanels(bool force);
void panelLoop();
void initializeLEDs();
void initializeDisplays();

// ************************************************************************************************
//
// YOU NEED TO AUTO-GENERATE THE LABEL SET YOU'LL BE USING, TO DO SO, SIMPLY GO TO THE LABELS
// DIRECTORY, COPY ANY OF THE PRE-SETS WITH A UNIQUE DIRECTORY NAME (e.g COPY README_FIRST TO
// IFEI OR CUSTOM) AND INSIDE IFEI OR CUSTOM JUST UPDATE "selected_panels.txt" AND RUN THE PYTHON
// SCRIPT "generate_data.py". THE LABELS LABEL_SET_XXXX ARE DEFINED BY YOU, YOU SIMPLY NEED TO 
// MAKE SURE THAT WHEN CREATING A CUSTOM SET YOU ADD IT TO THE CONDITIONALS BELOW AND ALSO SELECT
// IT VIA "#define LABEL_SET_XXXX" IN "Config.h". PLEASE, STUDY BOTH "Config.h" AND "Mappings.h"
// TO UNDERSTAND HOW THIS WORKS, INCLUDING THE AUTO-GENERATOR INSIDE THE LABELS DIRECTORY.  
//
// ************************************************************************************************
//
// Then, just use the correct LABEL_SET_XXXXX on Config.h to pick which firmware to build.

#if defined(LABEL_SET_ALR67)

   #define LABEL_SET "RWR ALR-67, ECM Panel + Cockpit Altimeter Gauge"

   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33 // Mode Selection Pin (DCS-BIOS/HID)  
   #define MODE_DEFAULT_IS_HID 0

   #include "LABELS/LABEL_SET_ALR67/CT_Display.h"
   #include "LABELS/LABEL_SET_ALR67/DCSBIOSBridgeData.h"
   #include "LABELS/LABEL_SET_ALR67/InputMapping.h"
   #include "LABELS/LABEL_SET_ALR67/LEDMapping.h"

   // —— Single‐source panel table —— 
   struct PanelDef { uint8_t addr; PanelID id; const char* label; };
   static constexpr PanelDef kPanels[] = {
     { 0x22, PanelID::ECM,   "ECM Panel"            },
     // { 0x26, PanelID::BRAIN, "Brain / IRCool Panel" },
     // { 0x5B, PanelID::ARM,   "Master Arm Panel"     },
   };

   constexpr bool hasBrain     = false;
   constexpr bool hasECM       = true;
   constexpr bool hasMasterARM = false;
   constexpr bool hasIFEI      = false;
   constexpr bool hasALR67     = true;
   constexpr bool hasCA        = false;
   constexpr bool hasLA        = false;
   constexpr bool hasRA        = false;
   constexpr bool hasIR        = false;
   constexpr bool hasLockShoot = false;

#elif defined(LABEL_SET_ALTIMETER)

   #define LABEL_SET "Cockpit Altimeter"

   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33 // Mode Selection Pin (DCS-BIOS/HID)  
   #define MODE_DEFAULT_IS_HID 0

   #include "LABELS/LABEL_SET_ALTIMETER/CT_Display.h"
   #include "LABELS/LABEL_SET_ALTIMETER/DCSBIOSBridgeData.h"
   #include "LABELS/LABEL_SET_ALTIMETER/InputMapping.h"
   #include "LABELS/LABEL_SET_ALTIMETER/LEDMapping.h"

   // —— Single‐source panel table —— 
   struct PanelDef { uint8_t addr; PanelID id; const char* label; };
   static constexpr PanelDef kPanels[] = {
     // { 0x22, PanelID::ECM,   "ECM Panel"            },
     // { 0x26, PanelID::BRAIN, "Brain / IRCool Panel" },
     // { 0x5B, PanelID::ARM,   "Master Arm Panel"     },
   };

   constexpr bool hasBrain     = false;
   constexpr bool hasECM       = false;
   constexpr bool hasMasterARM = false;
   constexpr bool hasIFEI      = false;
   constexpr bool hasALR67     = false;
   constexpr bool hasCA        = false;
   constexpr bool hasLA        = false;
   constexpr bool hasRA        = false;
   constexpr bool hasIR        = false;
   constexpr bool hasLockShoot = false;

#elif defined(LABEL_SET_ALL)

   #define LABEL_SET "ALL (Debug Only)"

   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33 // Mode Selection Pin (DCS-BIOS/HID)  
   #define MODE_DEFAULT_IS_HID 0

   #include "LABELS/LABEL_SET_ALL/CT_Display.h"
   #include "LABELS/LABEL_SET_ALL/DCSBIOSBridgeData.h"
   #include "LABELS/LABEL_SET_ALL/InputMapping.h"
   #include "LABELS/LABEL_SET_ALL/LEDMapping.h"

   // —— Single‐source panel table —— 
   struct PanelDef { uint8_t addr; PanelID id; const char* label; };
   static constexpr PanelDef kPanels[] = {
     // { 0x22, PanelID::ECM,   "ECM Panel"            },
     // { 0x26, PanelID::BRAIN, "Brain / IRCool Panel" },
     // { 0x5B, PanelID::ARM,   "Master Arm Panel"     },
   };

   constexpr bool hasBrain     = false;
   constexpr bool hasECM       = false;
   constexpr bool hasMasterARM = false;
   constexpr bool hasIFEI      = false;
   constexpr bool hasALR67     = false;
   constexpr bool hasCA        = false;
   constexpr bool hasLA        = false;
   constexpr bool hasRA        = false;
   constexpr bool hasIR        = false;
   constexpr bool hasLockShoot = false;

#elif defined(LABEL_SET_IFEI_NO_VIDEO)

   #define LABEL_SET "IFEI (No Video)"

   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_SWITCH_PIN 33 // Mode Selection Pin (DCS-BIOS/HID)  
   #define MODE_DEFAULT_IS_HID 0

   #include "LABELS/LABEL_SET_IFEI_NO_VIDEO/CT_Display.h"
   #include "LABELS/LABEL_SET_IFEI_NO_VIDEO/DCSBIOSBridgeData.h"
   #include "LABELS/LABEL_SET_IFEI_NO_VIDEO/InputMapping.h"
   #include "LABELS/LABEL_SET_IFEI_NO_VIDEO/LEDMapping.h"

   // —— Single‐source panel table —— 
   struct PanelDef { uint8_t addr; PanelID id; const char* label; };
   static constexpr PanelDef kPanels[] = {
     // { 0x22, PanelID::ECM,   "ECM Panel"            },
     // { 0x26, PanelID::BRAIN, "Brain / IRCool Panel" },
     // { 0x5B, PanelID::ARM,   "Master Arm Panel"     },
   };

   constexpr bool hasBrain     = false;
   constexpr bool hasECM       = false;
   constexpr bool hasMasterARM = false;
   constexpr bool hasIFEI      = true;
   constexpr bool hasALR67     = false;
   constexpr bool hasCA        = false;
   constexpr bool hasLA        = false;
   constexpr bool hasRA        = false;
   constexpr bool hasIR        = false;
   constexpr bool hasLockShoot = false;

#elif defined(LABEL_SET_MAIN)

   #define LABEL_SET "Main Instruments"

   #define HAS_HID_MODE_SELECTOR 1
   #define MODE_SWITCH_PIN 33 // Mode Selection Pin (DCS-BIOS/HID)  
   #define MODE_DEFAULT_IS_HID 0

   #include "LABELS/LABEL_SET_MAIN/CT_Display.h"
   #include "LABELS/LABEL_SET_MAIN/DCSBIOSBridgeData.h"
   #include "LABELS/LABEL_SET_MAIN/InputMapping.h"
   #include "LABELS/LABEL_SET_MAIN/LEDMapping.h"

   // —— Single‐source panel table —— 
   struct PanelDef { uint8_t addr; PanelID id; const char* label; };
   static constexpr PanelDef kPanels[] = {
     // { 0x22, PanelID::ECM,   "ECM Panel"            },
     { 0x26, PanelID::BRAIN, "Brain / IRCool Panel" },
     { 0x5B, PanelID::ARM,   "Master Arm Panel"     },
   };

   constexpr bool hasBrain     = true;
   constexpr bool hasECM       = false;
   constexpr bool hasMasterARM = true;
   constexpr bool hasIFEI      = false;
   constexpr bool hasALR67     = false;
   constexpr bool hasCA        = true;
   constexpr bool hasLA        = true;
   constexpr bool hasRA        = true;
   constexpr bool hasIR        = true;
   constexpr bool hasLockShoot = true;

#endif