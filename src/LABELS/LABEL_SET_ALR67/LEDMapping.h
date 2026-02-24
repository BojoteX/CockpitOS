// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_PCA9555,
  DEVICE_GAUGE,
  DEVICE_WS2812,
  DEVICE_TM1637,
  DEVICE_GPIO,
  DEVICE_NONE,
  DEVICE_GN1640T,
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
    struct { uint8_t gpio; uint16_t restPosition; } magneticInfo;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "PRESSURE_ALT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AUX_REL_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_DISPENSE_SW"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_JET_SEL_BTN"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_JET_SEL_L"        , DEVICE_PCA9555 , {.pcaInfo = {0x22, 1, 1}}, false, true }, // PCA 0x22 Port 1 Bit 1,
  { "ECM_MODE_SW"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"       , DEVICE_GPIO    , {.gpioInfo = {ALR67_BACKLIGHT_PIN}}, true, false }, // GPIO ALR67_BACKLIGHT_PIN,
  { "LIGHTS_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_AUDIO_CTRL"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_BIT_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_BIT_LT"            , DEVICE_GPIO    , {.gpioInfo = {PIN(4)}}, false, true }, // GPIO PIN(4),
  { "RWR_DISPLAY_BTN"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_DISPLAY_LT"        , DEVICE_GPIO    , {.gpioInfo = {PIN(13)}}, false, true }, // GPIO PIN(13),
  { "RWR_DIS_TYPE_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_DMR_CTRL"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_ENABLE_LT"         , DEVICE_GPIO    , {.gpioInfo = {PIN(5)}}, false, true }, // GPIO PIN(5),
  { "RWR_FAIL_LT"           , DEVICE_GPIO    , {.gpioInfo = {PIN(3)}}, false, true }, // GPIO PIN(3),
  { "RWR_LIMIT_LT"          , DEVICE_GPIO    , {.gpioInfo = {PIN(11)}}, false, true }, // GPIO PIN(11),
  { "RWR_LOWER_LT"          , DEVICE_GPIO    , {.gpioInfo = {PIN(12)}}, false, true }, // GPIO PIN(12),
  { "RWR_LT_BRIGHT"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_OFFSET_BTN"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_OFFSET_LT"         , DEVICE_GPIO    , {.gpioInfo = {PIN(6)}}, false, true }, // GPIO PIN(6),
  { "RWR_POWER_BTN"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_RWR_INTESITY"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, true }, // No Info,
  { "RWR_SPECIAL_BTN"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RWR_SPECIAL_EN_LT"     , DEVICE_GPIO    , {.gpioInfo = {PIN(7)}}, false, true }, // GPIO PIN(7),
  { "RWR_SPECIAL_LT"        , DEVICE_GPIO    , {.gpioInfo = {PIN(10)}}, false, true }, // GPIO PIN(10)
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[67] = {
  {"WARN_CAUTION_DIMMER", &panelLEDs[12]},
  {"RWR_BIT_BTN", &panelLEDs[14]},
  {nullptr, nullptr},
  {"AUX_REL_SW", &panelLEDs[1]},
  {"RWR_DIS_TYPE_SW", &panelLEDs[18]},
  {"RWR_ENABLE_LT", &panelLEDs[20]},
  {"RWR_LOWER_LT", &panelLEDs[23]},
  {"RWR_RWR_INTESITY", &panelLEDs[28]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[10]},
  {"RWR_SPECIAL_BTN", &panelLEDs[29]},
  {nullptr, nullptr},
  {"RWR_SPECIAL_EN_LT", &panelLEDs[30]},
  {nullptr, nullptr},
  {"RWR_DMR_CTRL", &panelLEDs[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_OFFSET_LT", &panelLEDs[26]},
  {"RWR_DISPLAY_LT", &panelLEDs[17]},
  {"RWR_POWER_BTN", &panelLEDs[27]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_LT_BRIGHT", &panelLEDs[24]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[7]},
  {"CMSD_DISPENSE_SW", &panelLEDs[2]},
  {"CMSD_JET_SEL_L", &panelLEDs[4]},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_BTN", &panelLEDs[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_OFFSET_BTN", &panelLEDs[25]},
  {"CHART_DIMMER", &panelLEDs[6]},
  {"RWR_SPECIAL_LT", &panelLEDs[31]},
  {"PRESSURE_ALT", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_FAIL_LT", &panelLEDs[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_BIT_LT", &panelLEDs[15]},
  {"LIGHTS_TEST_SW", &panelLEDs[11]},
  {"RWR_DISPLAY_BTN", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_LIMIT_LT", &panelLEDs[22]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[8]},
  {"FLOOD_DIMMER", &panelLEDs[9]},
  {"RWR_AUDIO_CTRL", &panelLEDs[13]},
  {"ECM_MODE_SW", &panelLEDs[5]},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 67;
  for (uint16_t i = 0; i < 67; ++i) {
    uint16_t idx = (startH + i >= 67) ? (startH + i - 67) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
