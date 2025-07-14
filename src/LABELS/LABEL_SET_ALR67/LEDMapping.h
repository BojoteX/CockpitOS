// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_NONE,
  DEVICE_TM1637,
  DEVICE_PCA9555,
  DEVICE_GPIO,
  DEVICE_WS2812,
  DEVICE_GAUGE,
  DEVICE_GN1640T,
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
  { "PRESSURE_ALT"       , DEVICE_GAUGE   , {.gaugeInfo = {PRESSURE_ALT_GAUGE_PIN, 700, 2000, 20000}}, false, false }, // GAUGE GPIO PRESSURE_ALT_GAUGE_PIN,
  { "CMSD_JET_SEL_L"     , DEVICE_PCA9555 , {.pcaInfo = {0x22, 1, 1}}, false, true }, // PCA 0x22 Port 1 Bit 1,
  { "CHART_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"    , DEVICE_GPIO    , {.gpioInfo = {INST_BACKLIGHT_PIN_ALR67}}, true, false }, // GPIO INST_BACKLIGHT_PIN_ALR67,
  { "WARN_CAUTION_DIMMER", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_AUDIO_CTRL"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_BIT_LT"         , DEVICE_GPIO    , {.gpioInfo = {RWR_BIT_LT_PIN}}, false, true }, // GPIO RWR_BIT_LT_PIN,
  { "RWR_DISPLAY_LT"     , DEVICE_GPIO    , {.gpioInfo = {RWR_DISPLAY_LT_PIN}}, false, true }, // GPIO RWR_DISPLAY_LT_PIN,
  { "RWR_DMR_CTRL"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_ENABLE_LT"      , DEVICE_GPIO    , {.gpioInfo = {RWR_ENABLE_LT_PIN}}, false, true }, // GPIO RWR_ENABLE_LT_PIN,
  { "RWR_FAIL_LT"        , DEVICE_GPIO    , {.gpioInfo = {RWR_FAIL_LT_PIN}}, false, true }, // GPIO RWR_FAIL_LT_PIN,
  { "RWR_LIMIT_LT"       , DEVICE_GPIO    , {.gpioInfo = {RWR_LIMIT_LT_PIN}}, false, true }, // GPIO RWR_LIMIT_LT_PIN,
  { "RWR_LOWER_LT"       , DEVICE_GPIO    , {.gpioInfo = {RWR_LOWER_LT_PIN}}, false, true }, // GPIO RWR_LOWER_LT_PIN,
  { "RWR_LT_BRIGHT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_OFFSET_LT"      , DEVICE_GPIO    , {.gpioInfo = {RWR_OFFSET_LT_PIN}}, false, true }, // GPIO RWR_OFFSET_LT_PIN,
  { "RWR_RWR_INTESITY"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_SPECIAL_EN_LT"  , DEVICE_GPIO    , {.gpioInfo = {RWR_SPECIAL_EN_LT_PIN}}, false, true }, // GPIO RWR_SPECIAL_EN_LT_PIN,
  { "RWR_SPECIAL_LT"     , DEVICE_GPIO    , {.gpioInfo = {RWR_SPECIAL_LT_PIN}}, false, true }, // GPIO RWR_SPECIAL_LT_PIN
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
