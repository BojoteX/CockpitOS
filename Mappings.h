// Mappings.h

#pragma once

// Global GPIO centralized PIN assignments. This ones should be carefully MANAGED. Use your board ARDUINO_ name for custom PINs
#if defined(ARDUINO_LOLIN_S3_MINI)

   // General Pins
   #define SDA_PIN                                    7 // SDA for S3
   #define SCL_PIN                                   13 // SCL for S3
   #define GLOBAL_CLK_PIN                            44 // Global CLK for S3
   #define CA_DIO_PIN                                38 // Caution Advisory DIO Pin
   #define LA_DIO_PIN                                43 // Left Annunciator DIO Pin
   #define LA_CLK_PIN                                GLOBAL_CLK_PIN // Left Annunciator Clock Pin
   #define RA_DIO_PIN                                33 // Right Annunciator DIO Pin
   #define RA_CLK_PIN                                GLOBAL_CLK_PIN // right Annunciator Clock Pin
   #define LOCKSHOOT_DIO_PIN                         36 // Lock-Shoot Indicator DIO Pin
   #define WS2812B_PIN                               LOCKSHOOT_DIO_PIN // Lock-Shoot Indicator DIO Pin

   // Misc Pins
   #define MODE_SWITCH_PIN                           35 // Mode Selector Pin
   #define HMD_KNOB_PIN                              18 // Axis for HMD Knob
   #define SPIN_LED_PIN                              34 // GPIO for SPIN Led   
   #define INST_BACKLIGHT_PIN                         6 // GPIO for Instrument Backlight    

   // Right Panel Controller
   #define HC165_RIGHT_PANEL_CONTROLLER_QH            35  // Serial data output (QH)
   #define HC165_RIGHT_PANEL_CONTROLLER_CP            34  // Clock input
   #define HC165_RIGHT_PANEL_CONTROLLER_PL            36  // Latch input (active-low)     
   #define FLOOD_DIMMER_KNOB_PIN                      1     
   #define INST_PNL_DIMMER_KNOB_PIN                   3     
   #define CABIN_TEMP_KNOB_PIN                        2
   #define SUIT_TEMP_KNOB_PIN                         5 
   #define CONSOLES_DIMMER_KNOB_PIN                   4 
   #define WARN_CAUTION_DIMMER_KNOB_PIN               6   
   #define CHART_DIMMER_KNOB_PIN                      12 
   #define LED_CONSOLE_BACKLIGHT_RIGHT_PANEL          14

   // Left Panel Controller
   #define HC165_LEFT_PANEL_CONTROLLER_QH            38  // Serial data output (QH)
   #define HC165_LEFT_PANEL_CONTROLLER_CP            37  // Clock input
   #define HC165_LEFT_PANEL_CONTROLLER_PL            43  // Latch input (active-low)   
   #define LED_APU_READY                             17
   #define LED_CONSOLE_BACKLIGHT                     10

   #define COM_AUX_KNOB_PIN                           1
   #define COM_ICS_KNOB_PIN                           2
   #define COM_MIDS_A_KNOB_PIN                        3
   #define COM_MIDS_B_KNOB_PIN                        4
   #define COM_RWR_KNOB_PIN                           5
   #define COM_TACAN_KNOB_PIN                         6
   #define COM_VOX_KNOB_PIN                           7
   #define COM_WPN_KNOB_PIN                          14
   #define OXYFLOW_KNOB_PIN                          16

   // Front Left Panel Controller
   #define FORMATION_LTS_KNOB_PIN                     1
   #define POSITION_LTS_KNOB_PIN                      3
   #define LED_CONSOLE_BACKLIGHT_FRONT_LEFT_PANEL    10
   
   // ALR-67 Pins
   #define ALR67_HC165_PL                            36
   #define ALR67_HC165_CP                            34
   #define ALR67_HC165_QH                            35
   #define ALR67_STROBE_1                            16
   #define ALR67_STROBE_2                            17
   #define ALR67_STROBE_3                            21
   #define ALR67_STROBE_4                            44
   #define ALR67_DataPin                             37
   #define RWR_AUDIO_PIN                              1
   #define RWR_DMR_PIN                                3    

   // Chip Select pin for battery gauge (TFT Battery CS pin)
   #define BATTERY_CS_PIN                            10    

   // Chip Select pin for Cabin Pressure gauge (TFT Cabin Pressure CS pin)
   #define CABIN_PRESSURE_CS_PIN                     10  

   // Chip Select pin for Brake Pressure gauge (TFT Brake Pressure CS pin)
   #define BRAKE_PRESSURE_CS_PIN                     10  

   // --- 74HC165 IFEI pins ---
   #define HC165_QH                                  37
   #define HC165_PL                                  43
   #define HC165_CP                                  33

   // Left LCD
   #define DATA0_PIN                                 34
   #define WR0_PIN                                   36
   #define CS0_PIN                                   38

   // Right LCD
   #define DATA1_PIN                                 18
   #define WR1_PIN                                   21
   #define CS1_PIN                                   35

   // Backlight PINs
   #define BL_GREEN_PIN                               1
   #define BL_WHITE_PIN                               3
   #define BL_NVG_PIN                                 5      

   // BRT Axis
   #define IFEI_BRIGHTNESS_PIN                        2   

   // ALR-67 Pins (S3)
   #define RWR_SPECIAL_LT_PIN                         8
   #define RWR_SPECIAL_EN_LT_PIN                     12
   #define RWR_OFFSET_LT_PIN                          6
   #define RWR_LOWER_LT_PIN                          10
   #define RWR_LIMIT_LT_PIN                          11
   #define RWR_FAIL_LT_PIN                            2
   #define RWR_ENABLE_LT_PIN                          4
   #define RWR_DISPLAY_LT_PIN                         9
   #define RWR_BIT_LT_PIN                             5
   #define PRESSURE_ALT_GAUGE_PIN                    18
   #define INST_BACKLIGHT_PIN_ALR67                  14     

#else // Below are PINs used with the LOLIN S2 Mini board. Create elif branch if adding different boards

   // General Pins
   #define SDA_PIN                                    8 // This could be overriden from a panel file if needed
   #define SCL_PIN                                    9 // This could be overriden from a panel file if needed
   #define GLOBAL_CLK_PIN                            37 // Clock Pin
   #define CA_DIO_PIN                                36 // Caution Advisory DIO Pin
   #define LA_DIO_PIN                                39 // Left Annunciator DIO Pin
   #define LA_CLK_PIN                                GLOBAL_CLK_PIN // Left Annunciator Clock Pin
   #define RA_DIO_PIN                                40 // Right Annunciator DIO Pin
   #define RA_CLK_PIN                                GLOBAL_CLK_PIN // right Annunciator Clock Pin
   #define LOCKSHOOT_DIO_PIN                         35 // Lock-Shoot Indicator DIO Pin
   #define WS2812B_PIN                               LOCKSHOOT_DIO_PIN // Lock-Shoot Indicator DIO Pin

   // Misc Pins
   #define MODE_SWITCH_PIN                           33 // Mode Selector Pin 
   #define HMD_KNOB_PIN                              18 // Acis for HMD Knob
   #define SPIN_LED_PIN                              34 // GPIO for SPIN Led     
   #define INST_BACKLIGHT_PIN                         6 // GPIO for Instrument Backlight   

   // Right Panel Controller (S2)
   #define HC165_RIGHT_PANEL_CONTROLLER_QH            33  // Serial data output (QH)
   #define HC165_RIGHT_PANEL_CONTROLLER_CP            34  // Clock input
   #define HC165_RIGHT_PANEL_CONTROLLER_PL            35  // Latch input (active-low)   
   #define FLOOD_DIMMER_KNOB_PIN                      1     
   #define INST_PNL_DIMMER_KNOB_PIN                   2     
   #define CABIN_TEMP_KNOB_PIN                        3
   #define SUIT_TEMP_KNOB_PIN                         4 
   #define CONSOLES_DIMMER_KNOB_PIN                   5 
   #define WARN_CAUTION_DIMMER_KNOB_PIN               6   
   #define CHART_DIMMER_KNOB_PIN                      7 
   #define LED_CONSOLE_BACKLIGHT_RIGHT_PANEL         14
   
// Left Panel Controller (S2)
   #define HC165_LEFT_PANEL_CONTROLLER_QH            36  // Serial data output (QH)
   #define HC165_LEFT_PANEL_CONTROLLER_CP            38  // Clock input
   #define HC165_LEFT_PANEL_CONTROLLER_PL            39  // Latch input (active-low)   
   #define LED_APU_READY                             17
   #define LED_CONSOLE_BACKLIGHT_LEFT_PANEL          12

   #define COM_ICS_KNOB_PIN                           1
   #define COM_WPN_KNOB_PIN                           2
   #define COM_MIDS_A_KNOB_PIN                        3
   #define COM_AUX_KNOB_PIN                           4   
   #define COM_VOX_KNOB_PIN                           5
   #define COM_RWR_KNOB_PIN                           6
   #define COM_MIDS_B_KNOB_PIN                        7
   #define OXYFLOW_KNOB_PIN                          14
   #define COM_TACAN_KNOB_PIN                        16

   // Front Left Panel Controller
   #define FORMATION_LTS_KNOB_PIN                    1
   #define POSITION_LTS_KNOB_PIN                     2
   #define LED_CONSOLE_BACKLIGHT_FRONT_LEFT_PANEL   12

   // ALR-67 Pins
   #define ALR67_HC165_PL                            35
   #define ALR67_HC165_CP                            34
   #define ALR67_HC165_QH                            33
   #define ALR67_STROBE_1                            16
   #define ALR67_STROBE_2                            17
   #define ALR67_STROBE_3                            21
   #define ALR67_STROBE_4                            37
   #define ALR67_DataPin                             38
   #define RWR_AUDIO_PIN                              1
   #define RWR_DMR_PIN                                2

   // Chip Select pin for battery gauge (TFT CS pin)
   #define BATTERY_CS_PIN                            36   

   // Chip Select pin for Cabin Pressure gauge (TFT Cabin Pressure CS pin)
   #define CABIN_PRESSURE_CS_PIN                     36  

   // Chip Select pin for Brake Pressure gauge (TFT Brake Pressure CS pin)
   #define BRAKE_PRESSURE_CS_PIN                     36  

   // --- 74HC165 IFEI pins ---
   #define HC165_QH                                  38
   #define HC165_PL                                  39
   #define HC165_CP                                  40

   // Left LCD
   #define DATA0_PIN                                 34
   #define WR0_PIN                                   35
   #define CS0_PIN                                   36

   // Right LCD
   #define DATA1_PIN                                 18
   #define WR1_PIN                                   21
   #define CS1_PIN                                   33

   // Backlight PINs
   #define BL_GREEN_PIN                               1
   #define BL_WHITE_PIN                               2
   #define BL_NVG_PIN                                 4   

   // BRT Axis
   #define IFEI_BRIGHTNESS_PIN                        3

   // ALR-67 Pins (S2)
   #define RWR_SPECIAL_LT_PIN                        10
   #define RWR_SPECIAL_EN_LT_PIN                      7
   #define RWR_OFFSET_LT_PIN                          6
   #define RWR_LOWER_LT_PIN                          12
   #define RWR_LIMIT_LT_PIN                          11
   #define RWR_FAIL_LT_PIN                            3
   #define RWR_ENABLE_LT_PIN                          5
   #define RWR_DISPLAY_LT_PIN                        13
   #define RWR_BIT_LT_PIN                             4
   #define PRESSURE_ALT_GAUGE_PIN                    18
   #define INST_BACKLIGHT_PIN_ALR67                  14   

#endif

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
extern bool hasTFTBattGauge;
extern bool hasTFTCabPressGauge;
extern bool hasTFTBrakePressGauge;
extern bool hasRightPanelController;
extern bool hasLeftPanelController;
extern bool hasFrontLeftPanel;

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
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_ALR67/CT_Display.h"
   #include "src/LABELS/LABEL_SET_ALR67/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_ALR67/InputMapping.h"
   #include "src/LABELS/LABEL_SET_ALR67/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_ALR67/DisplayMapping.h"

#elif defined(LABEL_SET_ALTIMETER)

   #define LABEL_SET "Analog Cockpit Altimeter"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_ALTIMETER/CT_Display.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/InputMapping.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_ALTIMETER/DisplayMapping.h"

#elif defined(LABEL_SET_ALL)

   #define LABEL_SET "ALL (Debug Only)"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_ALL/CT_Display.h"
   #include "src/LABELS/LABEL_SET_ALL/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_ALL/InputMapping.h"
   #include "src/LABELS/LABEL_SET_ALL/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_ALL/DisplayMapping.h"

#elif defined(LABEL_SET_IFEI_NO_VIDEO)

   #define LABEL_SET "IFEI (No Video)"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/CT_Display.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/InputMapping.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_IFEI_NO_VIDEO/DisplayMapping.h"

#elif defined(LABEL_SET_MAIN)

   #define LABEL_SET "Main Instruments"
   #define HAS_HID_MODE_SELECTOR 1
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_MAIN/CT_Display.h"
   #include "src/LABELS/LABEL_SET_MAIN/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_MAIN/InputMapping.h"
   #include "src/LABELS/LABEL_SET_MAIN/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_MAIN/DisplayMapping.h"

#elif defined(LABEL_SET_RIGHT_PANEL_CONTROLLER)

   #define LABEL_SET "Right Panel Controller"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_RIGHT_PANEL_CONTROLLER/CT_Display.h"
   #include "src/LABELS/LABEL_SET_RIGHT_PANEL_CONTROLLER/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_RIGHT_PANEL_CONTROLLER/InputMapping.h"
   #include "src/LABELS/LABEL_SET_RIGHT_PANEL_CONTROLLER/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_RIGHT_PANEL_CONTROLLER/DisplayMapping.h"

#elif defined(LABEL_SET_LEFT_PANEL_CONTROLLER)

   #define LABEL_SET "Left Panel Controller"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_LEFT_PANEL_CONTROLLER/CT_Display.h"
   #include "src/LABELS/LABEL_SET_LEFT_PANEL_CONTROLLER/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_LEFT_PANEL_CONTROLLER/InputMapping.h"
   #include "src/LABELS/LABEL_SET_LEFT_PANEL_CONTROLLER/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_LEFT_PANEL_CONTROLLER/DisplayMapping.h"

#elif defined(LABEL_SET_FRONT_LEFT_PANEL)

   #define LABEL_SET "Front Left Panel"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_FRONT_LEFT_PANEL/CT_Display.h"
   #include "src/LABELS/LABEL_SET_FRONT_LEFT_PANEL/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_FRONT_LEFT_PANEL/InputMapping.h"
   #include "src/LABELS/LABEL_SET_FRONT_LEFT_PANEL/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_FRONT_LEFT_PANEL/DisplayMapping.h"

#elif defined(LABEL_SET_BATTERY_GAUGE)

#if defined(ARDUINO_LOLIN_S3_MINI)
   #define SDA_PIN 35
   #define SCL_PIN 36
#else
   #define SDA_PIN 33
   #define SCL_PIN 35
#endif

   #define LABEL_SET "Battery Gauge"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/CT_Display.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/InputMapping.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_BATTERY_GAUGE/DisplayMapping.h"

#elif defined(LABEL_SET_BRAKE_PRESSURE_GAUGE)

#if defined(ARDUINO_LOLIN_S3_MINI)
   #define SDA_PIN 35
   #define SCL_PIN 36
#else
   #define SDA_PIN 33
   #define SCL_PIN 35
#endif

   #define LABEL_SET "Brake Pressure Gauge"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_BRAKE_PRESSURE_GAUGE/CT_Display.h"
   #include "src/LABELS/LABEL_SET_BRAKE_PRESSURE_GAUGE/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_BRAKE_PRESSURE_GAUGE/InputMapping.h"
   #include "src/LABELS/LABEL_SET_BRAKE_PRESSURE_GAUGE/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_BRAKE_PRESSURE_GAUGE/DisplayMapping.h"

#elif defined(LABEL_SET_CABIN_PRESSURE_GAUGE)

#if defined(ARDUINO_LOLIN_S3_MINI)
   #define SDA_PIN 35
   #define SCL_PIN 36
#else
   #define SDA_PIN 33
   #define SCL_PIN 35
#endif

   #define LABEL_SET "Cabin Pressure Gauge"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_CABIN_PRESSURE_GAUGE/CT_Display.h"
   #include "src/LABELS/LABEL_SET_CABIN_PRESSURE_GAUGE/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_CABIN_PRESSURE_GAUGE/InputMapping.h"
   #include "src/LABELS/LABEL_SET_CABIN_PRESSURE_GAUGE/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_CABIN_PRESSURE_GAUGE/DisplayMapping.h"

#elif defined(LABEL_SET_F16_TEST)

   #define LABEL_SET "F-16 test"
   #define HAS_HID_MODE_SELECTOR 0
   #define MODE_DEFAULT_IS_HID 0

   #include "src/LABELS/LABEL_SET_F16_TEST/CT_Display.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/DCSBIOSBridgeData.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/InputMapping.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/LEDMapping.h"
   #include "src/LABELS/LABEL_SET_F16_TEST/DisplayMapping.h"

#endif

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