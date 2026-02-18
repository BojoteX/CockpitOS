// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_NONE,
  DEVICE_TM1637,
  DEVICE_GPIO,
  DEVICE_PCA9555,
  DEVICE_GAUGE,
  DEVICE_GN1640T,
  DEVICE_WS2812,
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
  { "IFEI_DWN_BTN" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_ET_BTN"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_MODE_BTN", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_QTY_BTN" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_UP_BTN"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_ZONE_BTN", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
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
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_QTY_BTN", &panelLEDs[3]},
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
  {"IFEI_UP_BTN", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_ZONE_BTN", &panelLEDs[5]},
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
