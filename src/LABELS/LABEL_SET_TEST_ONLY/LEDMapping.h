// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_WS2812,
  DEVICE_PCA9555,
  DEVICE_NONE,
  DEVICE_TM1637,
  DEVICE_GPIO,
  DEVICE_GN1640T,
  DEVICE_GAUGE,
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
  { "ARRESTING_HOOK_LT"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_JET_SEL_L"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, true, false }, // No Info,
  { "WARN_CAUTION_DIMMER", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_CAUTION_LT"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_CTR_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_LI_LT"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_LO_LT"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_RI_LT"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SJ_RO_LT"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SJ_RO_LT", &panelLEDs[12]},
  {"FLOOD_DIMMER", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ARRESTING_HOOK_LT", &panelLEDs[0]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[6]},
  {nullptr, nullptr},
  {"SJ_CTR_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[3]},
  {"INST_PNL_DIMMER", &panelLEDs[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SJ_LI_LT", &panelLEDs[9]},
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
  {"SJ_RI_LT", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_LT", &panelLEDs[7]},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[2]},
  {nullptr, nullptr},
  {"SJ_LO_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_L", &panelLEDs[1]},
  {nullptr, nullptr},
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
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
