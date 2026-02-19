// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_NONE,
  DEVICE_TM1637,
  DEVICE_GN1640T,
  DEVICE_PCA9555,
  DEVICE_GAUGE,
  DEVICE_WS2812,
  DEVICE_GPIO,
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
  { "FIRE_TEST_SW"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "GEN_TIE_COVER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "GEN_TIE_SW"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CHART_DIMMER"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CONSOLES_DIMMER"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"        , DEVICE_GPIO    , {.gpioInfo = {6}}, true, false }, // GPIO 6,
  { "LIGHTS_TEST_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "WARN_CAUTION_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "HYD_ISOLATE_OVERRIDE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "MC_SW"                  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "ANTI_SKID_SW"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "FLAP_SW"                , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "HOOK_BYPASS_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "HYD_IND_BRAKE"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LAUNCH_BAR_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "LDG_TAXI_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "SEL_JETT_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "SEL_JETT_KNOB"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {"LDG_TAXI_SW", &panelLEDs[17]},
  {nullptr, nullptr},
  {"ANTI_SKID_SW", &panelLEDs[12]},
  {"HYD_IND_BRAKE", &panelLEDs[15]},
  {"HOOK_BYPASS_SW", &panelLEDs[14]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[7]},
  {"MC_SW", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SEL_JETT_BTN", &panelLEDs[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GEN_TIE_COVER", &panelLEDs[1]},
  {"GEN_TIE_SW", &panelLEDs[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[3]},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB", &panelLEDs[19]},
  {"LAUNCH_BAR_SW", &panelLEDs[16]},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[4]},
  {"FLOOD_DIMMER", &panelLEDs[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_TEST_SW", &panelLEDs[0]},
  {"CONSOLES_DIMMER", &panelLEDs[5]},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLAP_SW", &panelLEDs[13]},
  {"HYD_ISOLATE_OVERRIDE_SW", &panelLEDs[10]},
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
