// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_PCA9555,
  DEVICE_WS2812,
  DEVICE_NONE,
  DEVICE_GN1640T,
  DEVICE_GPIO,
  DEVICE_TM1637,
  DEVICE_GAUGE,
};

struct LEDMapping {
  const char* label;
  LEDDeviceType deviceType;
  union {
    struct { int8_t gpio; } gpioInfo;
    struct { uint8_t gpio; uint16_t minPulse; uint16_t maxPulse; uint16_t period; } gaugeInfo;
    struct { uint8_t address; uint8_t port; uint8_t bit; } pcaInfo;
    struct { uint8_t clkPin; uint8_t dioPin; uint8_t segment; uint8_t bit; } tm1637Info;
    struct { uint8_t address; uint8_t column; uint8_t row; } gn1640Info;
    struct { uint8_t index; uint8_t pin; uint8_t defR; uint8_t defG; uint8_t defB; uint8_t defBright; } ws2812Info;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "FLP_LG_FLAPS_LT"       , DEVICE_TM1637  , {.tm1637Info = {JETT_CLK_PIN, JETT_DIO_PIN, 5, 0}}, false, false }, // TM1637 CLK JETT_CLK_PIN DIO JETT_DIO_PIN Seg 5 Bit 0,
  { "FLP_LG_FULL_FLAPS_LT"  , DEVICE_TM1637  , {.tm1637Info = {JETT_CLK_PIN, JETT_DIO_PIN, 4, 1}}, false, false }, // TM1637 CLK JETT_CLK_PIN DIO JETT_DIO_PIN Seg 4 Bit 1,
  { "FLP_LG_HALF_FLAPS_LT"  , DEVICE_TM1637  , {.tm1637Info = {JETT_CLK_PIN, JETT_DIO_PIN, 4, 0}}, false, false }, // TM1637 CLK JETT_CLK_PIN DIO JETT_DIO_PIN Seg 4 Bit 0,
  { "FLP_LG_LEFT_GEAR_LT"   , DEVICE_TM1637  , {.tm1637Info = {JETT_CLK_PIN, JETT_DIO_PIN, 3, 0}}, false, false }, // TM1637 CLK JETT_CLK_PIN DIO JETT_DIO_PIN Seg 3 Bit 0,
  { "FLP_LG_NOSE_GEAR_LT"   , DEVICE_TM1637  , {.tm1637Info = {JETT_CLK_PIN, JETT_DIO_PIN, 5, 1}}, false, false }, // TM1637 CLK JETT_CLK_PIN DIO JETT_DIO_PIN Seg 5 Bit 1,
  { "FLP_LG_RIGHT_GEAR_LT"  , DEVICE_TM1637  , {.tm1637Info = {JETT_CLK_PIN, JETT_DIO_PIN, 3, 1}}, false, false }, // TM1637 CLK JETT_CLK_PIN DIO JETT_DIO_PIN Seg 3 Bit 1,
  { "IFEI"                  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MODE_SELECTOR_SW"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SELECT_HMD_LDDI_RDDI"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SELECT_HUD_LDDI_RDDI"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_DWN_BTN"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_ET_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_MODE_BTN"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_QTY_BTN"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_UP_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_ZONE_BTN"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"       , DEVICE_GPIO    , {.gpioInfo = {PIN(6)}}, true, false }, // GPIO PIN(6),
  { "LIGHTS_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_INT_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLE_INT_LT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "EMERG_INSTR_INT_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENG_INSTR_INT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_INT_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_BTN_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_DISP_INT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INSTR_INT_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "NVG_FLOOD_INT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, true, false }, // No Info,
  { "STBY_COMPASS_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_CTR"                , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_CTR_LT"             , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 0, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 0 Bit 0,
  { "SJ_LI"                 , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_LI_LT"              , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 1, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 1 Bit 0,
  { "SJ_LO"                 , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_LO_LT"              , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 1, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 1 Bit 1,
  { "SJ_RI"                 , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_RI_LT"              , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 2, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 2 Bit 0,
  { "SJ_RO"                 , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_RO_LT"              , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 2, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 2 Bit 1
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[89] = {
  {"FLP_LG_NOSE_GEAR_LT", &panelLEDs[4]},
  {"NVG_FLOOD_INT_LT", &panelLEDs[31]},
  {"SJ_RO_LT", &panelLEDs[42]},
  {"INSTR_INT_LT", &panelLEDs[30]},
  {"FLOOD_INT_LT", &panelLEDs[27]},
  {nullptr, nullptr},
  {"SJ_LI_LT", &panelLEDs[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLP_LG_FLAPS_LT", &panelLEDs[0]},
  {"FLP_LG_RIGHT_GEAR_LT", &panelLEDs[5]},
  {"SJ_RO", &panelLEDs[41]},
  {"IFEI_QTY_BTN", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLE_INT_LT", &panelLEDs[24]},
  {"EMERG_INSTR_INT_LT", &panelLEDs[25]},
  {"SJ_LI", &panelLEDs[35]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &panelLEDs[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI", &panelLEDs[6]},
  {"SJ_CTR_LT", &panelLEDs[34]},
  {nullptr, nullptr},
  {"SJ_RI_LT", &panelLEDs[40]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SELECT_HMD_LDDI_RDDI", &panelLEDs[8]},
  {"SJ_LO", &panelLEDs[37]},
  {"CHART_DIMMER", &panelLEDs[16]},
  {"SJ_RI", &panelLEDs[39]},
  {"FLP_LG_LEFT_GEAR_LT", &panelLEDs[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW", &panelLEDs[21]},
  {nullptr, nullptr},
  {"FLP_LG_HALF_FLAPS_LT", &panelLEDs[2]},
  {"IFEI_MODE_BTN", &panelLEDs[12]},
  {"STBY_COMPASS_INT_LT", &panelLEDs[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &panelLEDs[22]},
  {"CHART_INT_LT", &panelLEDs[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_BTN_INT_LT", &panelLEDs[28]},
  {nullptr, nullptr},
  {"ENG_INSTR_INT_LT", &panelLEDs[26]},
  {"SJ_LO_LT", &panelLEDs[38]},
  {"SELECT_HUD_LDDI_RDDI", &panelLEDs[9]},
  {"IFEI_ET_BTN", &panelLEDs[11]},
  {"IFEI_UP_BTN", &panelLEDs[14]},
  {"IFEI_ZONE_BTN", &panelLEDs[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW", &panelLEDs[7]},
  {"IFEI_DWN_BTN", &panelLEDs[10]},
  {nullptr, nullptr},
  {"IFEI_DISP_INT_LT", &panelLEDs[29]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[17]},
  {"SJ_CTR", &panelLEDs[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLP_LG_FULL_FLAPS_LT", &panelLEDs[1]},
  {"CONSOLES_DIMMER", &panelLEDs[18]},
  {nullptr, nullptr},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 89;
  for (uint16_t i = 0; i < 89; ++i) {
    uint16_t idx = (startH + i >= 89) ? (startH + i - 89) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
