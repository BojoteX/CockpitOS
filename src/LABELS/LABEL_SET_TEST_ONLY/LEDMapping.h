// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_NONE,
  DEVICE_GPIO,
  DEVICE_WS2812,
  DEVICE_GAUGE,
  DEVICE_TM1637,
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
    struct { uint8_t index; } ws2812Info;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "COM_AUX"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_ICS"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_MIDS_A"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_MIDS_B"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_RWR"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_TACAN"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_VOX"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_WPN"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CABIN_TEMP"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SUIT_TEMP"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RUD_TRIM"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LOW_ALT_WARN_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_ALT_PTR"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_GREEN_LAMP"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_MIN_HEIGHT_PTR", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_OFF_FLAG"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {"SUIT_TEMP", &panelLEDs[9]},
  {nullptr, nullptr},
  {"COM_WPN", &panelLEDs[7]},
  {"COM_VOX", &panelLEDs[6]},
  {nullptr, nullptr},
  {"COM_TACAN", &panelLEDs[5]},
  {"RADALT_GREEN_LAMP", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_AUX", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_ICS", &panelLEDs[1]},
  {nullptr, nullptr},
  {"RUD_TRIM", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADALT_MIN_HEIGHT_PTR", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CABIN_TEMP", &panelLEDs[8]},
  {nullptr, nullptr},
  {"RADALT_ALT_PTR", &panelLEDs[12]},
  {"RADALT_OFF_FLAG", &panelLEDs[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_RWR", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_MIDS_A", &panelLEDs[2]},
  {"COM_MIDS_B", &panelLEDs[3]},
  {"LOW_ALT_WARN_LT", &panelLEDs[11]},
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
