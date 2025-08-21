// Mappings.h

#pragma once

// -- Panel Modules -- Not needed
/*
#include "src/LeftAnnunciator.h"
#include "src/RightAnnunciator.h"
#include "src/IRCoolPanel.h"
#include "src/MasterARMPanel.h"
#include "src/ALR67Panel.h"
#include "src/ECMPanel.h"
#include "src/IFEIPanel.h"
#include "src/TFT_Gauges_BrakePress.h"
#include "src/TFT_Gauges_CabPress.h"
#include "src/TFT_Gauges_RadarAlt.h"
#include "src/TFT_Gauges_HydPress.h"
#include "src/FrontLeftPanel.h"
#include "src/CustomFrontRightPanel.h"
#include "src/RightPanelController.h"
#include "src/LeftPanelController.h"
*/

enum class PanelKind : uint8_t {
  Brain, 
  ECM, 
  MasterARM, 
  IFEI, 
  ALR67, 
  CA, 
  LA, 
  RA, 
  IR, 
  LockShoot,
  TFTBatt, 
  TFTCabPress, 
  TFTBrake, 
  TFTHyd, 
  TFTRadarAlt,
  RightPanelCtl, 
  LeftPanelCtl, 
  FrontLeft, 
  CustomFrontRight,
  TEST_ONLY,
  AnalogGauge,
  COUNT
};
static_assert(static_cast<uint8_t>(PanelKind::COUNT) <= 32,
			  "Switch mask to 64-bit if you add more kinds");


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
extern bool hasTFTBattGauge;
extern bool hasTFTCabPressGauge;
extern bool hasTFTBrakePressGauge;
extern bool hasTFTHydPressGauge;
extern bool hasTFTRadarAltGauge;
extern bool hasRightPanelController;
extern bool hasLeftPanelController;
extern bool hasFrontLeftPanel;
extern bool hasCustomFrontRightPanel;
extern bool hasTEST_ONLY;

// General Pins (canonical S2 → mapped by PIN())
#define SDA_PIN                               PIN(8)
#define SCL_PIN                               PIN(9)
#define GLOBAL_CLK_PIN                        PIN(37)
#define CA_DIO_PIN                            PIN(36)
#define LA_DIO_PIN                            PIN(39)
#define LA_CLK_PIN                            GLOBAL_CLK_PIN
#define RA_DIO_PIN                            PIN(40)
#define RA_CLK_PIN                            GLOBAL_CLK_PIN
#define LOCKSHOOT_DIO_PIN                     PIN(35)
#define WS2812B_PIN                           LOCKSHOOT_DIO_PIN

// Misc Pins
#define MODE_SWITCH_PIN                       PIN(33)
#define HMD_KNOB_PIN                          PIN(18)
#define SPIN_LED_PIN                          PIN(34)
#define INST_BACKLIGHT_PIN                    PIN(6)

// Right Panel Controller
#define HC165_RIGHT_PANEL_CONTROLLER_QH       PIN(33)  // QH
#define HC165_RIGHT_PANEL_CONTROLLER_CP       PIN(34)  // CP
#define HC165_RIGHT_PANEL_CONTROLLER_PL       PIN(35)  // PL
#define FLOOD_DIMMER_KNOB_PIN                 PIN(1)
#define INST_PNL_DIMMER_KNOB_PIN              PIN(2)
#define CABIN_TEMP_KNOB_PIN                   PIN(3)
#define SUIT_TEMP_KNOB_PIN                    PIN(4)
#define CONSOLES_DIMMER_KNOB_PIN              PIN(5)
#define WARN_CAUTION_DIMMER_KNOB_PIN          PIN(6)
#define CHART_DIMMER_KNOB_PIN                 PIN(7)
#define LED_CONSOLE_BACKLIGHT_RIGHT_PANEL     PIN(14)

// Left Panel Controller
#define HC165_LEFT_PANEL_CONTROLLER_QH        PIN(36)  // QH
#define HC165_LEFT_PANEL_CONTROLLER_CP        PIN(38)  // CP
#define HC165_LEFT_PANEL_CONTROLLER_PL        PIN(39)  // PL
#define LED_APU_READY                         PIN(17)
#define LED_CONSOLE_BACKLIGHT_LEFT_PANEL      PIN(12)

#define COM_ICS_KNOB_PIN                      PIN(1)
#define COM_WPN_KNOB_PIN                      PIN(2)
#define COM_MIDS_A_KNOB_PIN                   PIN(3)
#define COM_AUX_KNOB_PIN                      PIN(4)
#define COM_VOX_KNOB_PIN                      PIN(5)
#define COM_RWR_KNOB_PIN                      PIN(6)
#define COM_MIDS_B_KNOB_PIN                   PIN(7)
#define OXYFLOW_KNOB_PIN                      PIN(14)
#define COM_TACAN_KNOB_PIN                    PIN(16)

// Front Left Panel Controller
#define FORMATION_LTS_KNOB_PIN                PIN(1)
#define POSITION_LTS_KNOB_PIN                 PIN(2)
#define LED_CONSOLE_BACKLIGHT_FRONT_LEFT_PANEL PIN(12)

// ALR-67 Pins
#define ALR67_HC165_PL                        PIN(35)
#define ALR67_HC165_CP                        PIN(34)
#define ALR67_HC165_QH                        PIN(33)
#define ALR67_STROBE_1                        PIN(16)
#define ALR67_STROBE_2                        PIN(17)
#define ALR67_STROBE_3                        PIN(21)
#define ALR67_STROBE_4                        PIN(37)
#define ALR67_DataPin                         PIN(38)
#define RWR_AUDIO_PIN                         PIN(1)
#define RWR_DMR_PIN                           PIN(2)

// TFT chip-selects
#define BATTERY_CS_PIN                        PIN(38)
#define CABIN_PRESSURE_CS_PIN                 PIN(38)
#define BRAKE_PRESSURE_CS_PIN                 PIN(38)
#define HYD_PRESSURE_CS_PIN                   PIN(38)
#define RADARALT_CS_PIN                       PIN(38)

// 74HC165 IFEI pins
#define HC165_QH                              PIN(38)
#define HC165_PL                              PIN(39)
#define HC165_CP                              PIN(40)

// Left LCD
#define DATA0_PIN                             PIN(34)
#define WR0_PIN                               PIN(35)
#define CS0_PIN                               PIN(36)

// Right LCD
#define DATA1_PIN                             PIN(18)
#define WR1_PIN                               PIN(21)
#define CS1_PIN                               PIN(33)

// Backlight PINs
#define BL_GREEN_PIN                          PIN(1)
#define BL_WHITE_PIN                          PIN(2)
#define BL_NVG_PIN                            PIN(4)

// BRT Axis
#define IFEI_BRIGHTNESS_PIN                   PIN(3)

// ALR-67 extra pins
#define RWR_SPECIAL_LT_PIN                    PIN(10)
#define RWR_SPECIAL_EN_LT_PIN                 PIN(7)
#define RWR_OFFSET_LT_PIN                     PIN(6)
#define RWR_LOWER_LT_PIN                      PIN(12)
#define RWR_LIMIT_LT_PIN                      PIN(11)
#define RWR_FAIL_LT_PIN                       PIN(3)
#define RWR_ENABLE_LT_PIN                     PIN(5)
#define RWR_DISPLAY_LT_PIN                    PIN(13)
#define RWR_BIT_LT_PIN                        PIN(4)
#define PRESSURE_ALT_GAUGE_PIN                PIN(18)
#define INST_BACKLIGHT_PIN_ALR67              PIN(14)

// Radar Altimeter GPIOs
#define RA_TEST_GPIO                          PIN(2)
#define RA_DEC_HEIGHT_GPIO                    PIN(3)
#define RA_INC_HEIGHT_GPIO                    PIN(4)

void initMappings();
void initializePanels(bool force);
void panelLoop();
void initializeLEDs();
void initializeDisplays();

// Give your PCA custom panel a basic short name and address where it is located (use the I2C scanner in Tools) 
enum class PanelID : uint8_t { 
   ECM      = 0x22, // ECM Panel bundled with ALR-67
   BRAIN    = 0x26, // TEK Brain Controller (IR Cool uses the PCA on the brain, IRCOOL itself its a dumb panel)
   CUSTOM   = 0x27, // Used for testing simulating a panel (for the Tutorial)
   ARM      = 0x5B, // Master ARM, 0x5B is non-standard (PCA clone? hack?) don't know, don't care
   UNKNOWN  = 0x00  // Placeholder
};

// —— Source panel table —— (don't forget to update the ENUM above, here you give it a friendly name) 
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

#include "src/LabelSetSelect.h"

/*
       S2 to S3 GPIO conversion table
-----------------------------------------------
| S2 Mini GPIO | S3 Mini GPIO (same position) |
| ------------ | ---------------------------- |
| 3            | 2                            |
| 5            | 4                            |
| 7            | 12                           |
| 9            | 13                           |
| 12           | 10                           |
| 2            | 3                            |
| 4            | 5                            |
| 8            | 7                            |
| 10           | 8                            |
| 13           | 9                            |
| 40           | 33                           |
| 38           | 37                           |
| 36           | 38                           |
| 39           | 43                           |
| 37           | 44                           |
| 35           | 36                           |
| 33           | 35                           |
-----------------_-----------------------------

*/