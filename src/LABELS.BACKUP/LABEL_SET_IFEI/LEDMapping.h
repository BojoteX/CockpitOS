// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_GN1640T,
  DEVICE_NONE,
  DEVICE_PCA9555,
  DEVICE_WS2812,
  DEVICE_TM1637,
  DEVICE_GAUGE,
  DEVICE_GPIO,
};

struct LEDMapping {
  const char* label;
  LEDDeviceType deviceType;
  union {
    struct { uint8_t gpio; } gpioInfo;
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
  { "FLP_LG_FLAPS_LT"     , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 5, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 5 Bit 0,
  { "FLP_LG_FULL_FLAPS_LT", DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 4, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 4 Bit 1,
  { "FLP_LG_HALF_FLAPS_LT", DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 4, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 4 Bit 0,
  { "FLP_LG_LEFT_GEAR_LT" , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 3, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 3 Bit 0,
  { "FLP_LG_NOSE_GEAR_LT" , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 5, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 5 Bit 1,
  { "FLP_LG_RIGHT_GEAR_LT", DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 3, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 3 Bit 1,
  { "IFEI"                , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"     , DEVICE_GPIO    , {.gpioInfo = {PIN(6)}}, true, false }, // GPIO PIN(6),
  { "WARN_CAUTION_DIMMER" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_INT_LT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLE_INT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "EMERG_INSTR_INT_LT"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENG_INSTR_INT_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_INT_LT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_BTN_INT_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_DISP_INT_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INSTR_INT_LT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "NVG_FLOOD_INT_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, true, false }, // No Info,
  { "STBY_COMPASS_INT_LT" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_CTR_LT"           , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 0, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 0 Bit 0,
  { "SJ_LI_LT"            , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 1, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 1 Bit 0,
  { "SJ_LO_LT"            , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 1, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 1 Bit 1,
  { "SJ_RI_LT"            , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 2, 0}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 2 Bit 0,
  { "SJ_RO_LT"            , DEVICE_TM1637  , {.tm1637Info = {PIN(9), PIN(8), 2, 1}}, false, false }, // TM1637 CLK PIN(9) DIO PIN(8) Seg 2 Bit 1
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[59] = {
  {nullptr, nullptr},
  {"FLP_LG_NOSE_GEAR_LT", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLP_LG_RIGHT_GEAR_LT", &panelLEDs[5]},
  {"INST_PNL_DIMMER", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EMERG_INSTR_INT_LT", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DISP_INT_LT", &panelLEDs[18]},
  {"CHART_INT_LT", &panelLEDs[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_BTN_INT_LT", &panelLEDs[17]},
  {"NVG_FLOOD_INT_LT", &panelLEDs[20]},
  {"SJ_LO_LT", &panelLEDs[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI", &panelLEDs[6]},
  {"STBY_COMPASS_INT_LT", &panelLEDs[21]},
  {"SJ_RO_LT", &panelLEDs[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENG_INSTR_INT_LT", &panelLEDs[15]},
  {"FLP_LG_FULL_FLAPS_LT", &panelLEDs[1]},
  {"FLP_LG_HALF_FLAPS_LT", &panelLEDs[2]},
  {"FLP_LG_LEFT_GEAR_LT", &panelLEDs[3]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[11]},
  {nullptr, nullptr},
  {"FLP_LG_FLAPS_LT", &panelLEDs[0]},
  {"CONSOLES_DIMMER", &panelLEDs[8]},
  {"CONSOLE_INT_LT", &panelLEDs[13]},
  {"FLOOD_INT_LT", &panelLEDs[16]},
  {"SJ_LI_LT", &panelLEDs[23]},
  {nullptr, nullptr},
  {"INSTR_INT_LT", &panelLEDs[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[7]},
  {"SJ_CTR_LT", &panelLEDs[22]},
  {"SJ_RI_LT", &panelLEDs[25]},
  {nullptr, nullptr},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 59;
  for (uint16_t i = 0; i < 59; ++i) {
    uint16_t idx = (startH + i >= 59) ? (startH + i - 59) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
