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
  { "FORMATION_DIMMER"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INT_WNG_TANK_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "POSITION_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "STROBE_SW"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "EXT_PWR_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GND_PWR_1_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GND_PWR_2_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GND_PWR_3_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GND_PWR_4_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"       , DEVICE_GPIO    , {.gpioInfo = {PIN(12)}}, true, false }, // GPIO PIN(12),
  { "FLOOD_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LIGHTS_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"STROBE_SW", &panelLEDs[3]},
  {nullptr, nullptr},
  {"GND_PWR_4_SW", &panelLEDs[8]},
  {"GND_PWR_1_SW", &panelLEDs[5]},
  {"INST_PNL_DIMMER", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"POSITION_DIMMER", &panelLEDs[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_2_SW", &panelLEDs[6]},
  {"EXT_PWR_SW", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[9]},
  {"FORMATION_DIMMER", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[10]},
  {"FLOOD_DIMMER", &panelLEDs[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &panelLEDs[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INT_WNG_TANK_SW", &panelLEDs[1]},
  {"LIGHTS_TEST_SW", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_3_SW", &panelLEDs[7]},
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
