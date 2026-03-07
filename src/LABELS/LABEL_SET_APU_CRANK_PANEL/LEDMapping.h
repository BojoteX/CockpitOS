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
  { "APU_CONTROL_SW"        , DEVICE_MAGNETIC, {.magneticInfo = {37, 255}}, false, false }, // MAGNETIC A=37 B=255,
  { "APU_READY_LT"          , DEVICE_GPIO    , {.gpioInfo = {PIN(17)}}, false, false }, // GPIO PIN(17),
  { "ENGINE_CRANK_SW"       , DEVICE_MAGNETIC, {.magneticInfo = {10, 11}}, false, false }, // MAGNETIC A=10 B=11,
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
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_CONTROL_SW", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW", &panelLEDs[2]},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[4]},
  {"FLOOD_DIMMER", &panelLEDs[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_READY_LT", &panelLEDs[1]},
  {"LIGHTS_TEST_SW", &panelLEDs[8]},
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
