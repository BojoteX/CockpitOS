// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_PCA9555,
  DEVICE_NONE,
  DEVICE_WS2812,
  DEVICE_TM1637,
  DEVICE_GPIO,
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
    struct { uint8_t index; uint8_t pin; uint8_t defR; uint8_t defG; uint8_t defB; uint8_t defBright; } ws2812Info;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "CLIP_APU_ACC_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_BATT_SW_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_CK_SEAT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_FCES_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_FCS_HOT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_FUEL_LO_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_GEN_TIE_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_L_GEN_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_R_GEN_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_SPARE_CTN1_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_SPARE_CTN2_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CLIP_SPARE_CTN3_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }
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
  {"CLIP_FCES_LT", &panelLEDs[3]},
  {"CLIP_SPARE_CTN1_LT", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN2_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN3_LT", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_APU_ACC_LT", &panelLEDs[0]},
  {"CLIP_FUEL_LO_LT", &panelLEDs[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_R_GEN_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_GEN_TIE_LT", &panelLEDs[6]},
  {"CLIP_CK_SEAT_LT", &panelLEDs[2]},
  {"CLIP_FCS_HOT_LT", &panelLEDs[4]},
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
  {"CLIP_BATT_SW_LT", &panelLEDs[1]},
  {nullptr, nullptr},
  {"CLIP_L_GEN_LT", &panelLEDs[7]},
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
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
