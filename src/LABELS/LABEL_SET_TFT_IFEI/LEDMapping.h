// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_GPIO,
  DEVICE_GAUGE,
  DEVICE_PCA9555,
  DEVICE_TM1637,
  DEVICE_GN1640T,
  DEVICE_WS2812,
  DEVICE_MAGNETIC,
  DEVICE_NONE,
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
    struct { uint8_t gpioA; uint8_t gpioB; } magneticInfo;  // gpioB=255 → single solenoid (2-pos)
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
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
  { "INST_PNL_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
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
  { "NVG_FLOOD_INT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "STBY_COMPASS_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_ET_BTN", &panelLEDs[1]},
  {"EMERG_INSTR_INT_LT", &panelLEDs[15]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_QTY_BTN", &panelLEDs[3]},
  {"IFEI_BTN_INT_LT", &panelLEDs[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLE_INT_LT", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[6]},
  {"FLOOD_INT_LT", &panelLEDs[17]},
  {nullptr, nullptr},
  {"INSTR_INT_LT", &panelLEDs[20]},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[7]},
  {"FLOOD_DIMMER", &panelLEDs[9]},
  {"IFEI_UP_BTN", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[8]},
  {"IFEI_ZONE_BTN", &panelLEDs[5]},
  {"IFEI_DISP_INT_LT", &panelLEDs[19]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENG_INSTR_INT_LT", &panelLEDs[16]},
  {"NVG_FLOOD_INT_LT", &panelLEDs[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_INT_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {"STBY_COMPASS_INT_LT", &panelLEDs[22]},
  {"IFEI_DWN_BTN", &panelLEDs[0]},
  {"IFEI_MODE_BTN", &panelLEDs[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 53;
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
