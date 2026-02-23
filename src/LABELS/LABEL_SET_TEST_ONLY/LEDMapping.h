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
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "APU_CONTROL_SW"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "APU_READY_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENGINE_CRANK_SW"  , DEVICE_GPIO    , {.gpioInfo = {15}}, true, false }, // GPIO 15,
  { "MASTER_ARM_SW"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AG"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AG_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MC_DISCH"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MC_READY"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
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
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_CONTROL_SW", &panelLEDs[0]},
  {"MASTER_ARM_SW", &panelLEDs[3]},
  {"MASTER_MODE_AA", &panelLEDs[4]},
  {"MASTER_MODE_AA_LT", &panelLEDs[5]},
  {nullptr, nullptr},
  {"MC_DISCH", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW", &panelLEDs[2]},
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
  {"MC_READY", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_READY_LT", &panelLEDs[1]},
  {"MASTER_MODE_AG", &panelLEDs[6]},
  {"MASTER_MODE_AG_LT", &panelLEDs[7]},
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
