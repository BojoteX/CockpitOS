// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_TM1637,
  DEVICE_WS2812,
  DEVICE_GPIO,
  DEVICE_GAUGE,
  DEVICE_NONE,
  DEVICE_PCA9555,
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
    struct { uint8_t index; uint8_t pin; uint8_t defR; uint8_t defG; uint8_t defB; uint8_t defBright; } ws2812Info;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  // { "PRESSURE_ALT"     , DEVICE_GAUGE    , {.gaugeInfo = {36,725,2125,20000}}, true, false },
  { "PRESSURE_ALT"     , DEVICE_GAUGE    , {.gaugeInfo = {36,800,2200,20000}}, true, false },
  { "COM_AUX"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_ICS"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_MIDS_A"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_MIDS_B"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_RWR"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_TACAN"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_VOX"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_WPN"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AG_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MC_DISCH"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MC_READY"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_CAUTION_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_WPN", &panelLEDs[8]},
  {"COM_VOX", &panelLEDs[7]},
  {nullptr, nullptr},
  {"COM_TACAN", &panelLEDs[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AA_LT", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_AUX", &panelLEDs[1]},
  {"MC_DISCH", &panelLEDs[11]},
  {"MC_READY", &panelLEDs[12]},
  {nullptr, nullptr},
  {"COM_ICS", &panelLEDs[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AG_LT", &panelLEDs[10]},
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
  {"COM_RWR", &panelLEDs[5]},
  {"MASTER_CAUTION_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_MIDS_A", &panelLEDs[3]},
  {"COM_MIDS_B", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
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
