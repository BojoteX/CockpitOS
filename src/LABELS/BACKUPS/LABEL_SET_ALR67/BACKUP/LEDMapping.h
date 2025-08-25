// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_PCA9555,
  DEVICE_GPIO,
  DEVICE_GN1640T,
  DEVICE_NONE,
  DEVICE_WS2812,
  DEVICE_GAUGE,
  DEVICE_TM1637,
};

struct LEDMapping {
  const char* label;
  LEDDeviceType deviceType;
  union {
    struct { uint8_t gpio; } gpioInfo;
    struct { uint8_t gauge; } gaugeInfo;
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
  { "PRESSURE_ALT"       , DEVICE_GAUGE   , {.gpioInfo = {18}}, false, false }, // GAUGE GPIO 18,
  { "CMSD_JET_SEL_L"     , DEVICE_PCA9555 , {.pcaInfo = {0x22, 1, 1}}, false, true }, // PCA 0x22 Port 1 Bit 1,
  { "CHART_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {14}}, true, false }, // No Info,
  { "WARN_CAUTION_DIMMER", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_AUDIO_CTRL"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_BIT_LT"         , DEVICE_GPIO    , {.gpioInfo = {4}}, false, true }, // GPIO 4,
  { "RWR_DISPLAY_LT"     , DEVICE_GPIO    , {.gpioInfo = {13}}, false, true }, // GPIO 13,
  { "RWR_DMR_CTRL"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_ENABLE_LT"      , DEVICE_GPIO    , {.gpioInfo = {5}}, false, true }, // GPIO 5,
  { "RWR_FAIL_LT"        , DEVICE_GPIO    , {.gpioInfo = {3}}, false, true }, // GPIO 3,
  { "RWR_LIMIT_LT"       , DEVICE_GPIO    , {.gpioInfo = {11}}, false, true }, // GPIO 11,
  { "RWR_LOWER_LT"       , DEVICE_GPIO    , {.gpioInfo = {12}}, false, true }, // GPIO 12,
  { "RWR_LT_BRIGHT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_OFFSET_LT"      , DEVICE_GPIO    , {.gpioInfo = {6}}, false, true }, // GPIO 6,
  { "RWR_RWR_INTESITY"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_SPECIAL_EN_LT"  , DEVICE_GPIO    , {.gpioInfo = {7}}, false, true }, // GPIO 7,
  { "RWR_SPECIAL_LT"     , DEVICE_GPIO    , {.gpioInfo = {10}}, false, true }, // GPIO 10
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &panelLEDs[4]},
  {nullptr, nullptr},
  {"RWR_LIMIT_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &panelLEDs[6]},
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
  {"CONSOLES_DIMMER", &panelLEDs[3]},
  {"INST_PNL_DIMMER", &panelLEDs[5]},
  {"RWR_RWR_INTESITY", &panelLEDs[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_AUDIO_CTRL", &panelLEDs[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_OFFSET_LT", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_ENABLE_LT", &panelLEDs[11]},
  {"RWR_LT_BRIGHT", &panelLEDs[15]},
  {"RWR_SPECIAL_EN_LT", &panelLEDs[18]},
  {"RWR_SPECIAL_LT", &panelLEDs[19]},
  {"CHART_DIMMER", &panelLEDs[2]},
  {"RWR_FAIL_LT", &panelLEDs[12]},
  {nullptr, nullptr},
  {"RWR_DMR_CTRL", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_L", &panelLEDs[1]},
  {"RWR_BIT_LT", &panelLEDs[8]},
  {"RWR_LOWER_LT", &panelLEDs[14]},
  {"RWR_DISPLAY_LT", &panelLEDs[9]},
  {"PRESSURE_ALT", &panelLEDs[0]},
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
