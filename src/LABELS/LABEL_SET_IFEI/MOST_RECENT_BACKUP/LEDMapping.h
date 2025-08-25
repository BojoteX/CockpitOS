// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_GAUGE,
  DEVICE_PCA9555,
  DEVICE_WS2812,
  DEVICE_TM1637,
  DEVICE_NONE,
  DEVICE_GN1640T,
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
    struct { uint8_t index; } ws2812Info;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "CHART_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"    , DEVICE_GPIO    , {.gpioInfo = {6}}, true, false }, // GPIO 6,
  { "WARN_CAUTION_DIMMER", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLE_INT_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "EMERG_INSTR_INT_LT" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENG_INSTR_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_BTN_INT_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_DISP_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INSTR_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "NVG_FLOOD_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, true, false }, // No Info,
  { "STBY_COMPASS_INT_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &panelLEDs[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"NVG_FLOOD_INT_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_INT_LT", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DISP_INT_LT", &panelLEDs[11]},
  {"INSTR_INT_LT", &panelLEDs[12]},
  {"ENG_INSTR_INT_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[1]},
  {"INST_PNL_DIMMER", &panelLEDs[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[0]},
  {nullptr, nullptr},
  {"STBY_COMPASS_INT_LT", &panelLEDs[14]},
  {"IFEI_BTN_INT_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLE_INT_LT", &panelLEDs[6]},
  {"EMERG_INSTR_INT_LT", &panelLEDs[7]},
  {"CHART_INT_LT", &panelLEDs[5]},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 53;
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
