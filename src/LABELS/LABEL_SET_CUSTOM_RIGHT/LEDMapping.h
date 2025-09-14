// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_GPIO,
  DEVICE_TM1637,
  DEVICE_WS2812,
  DEVICE_GN1640T,
  DEVICE_NONE,
  DEVICE_GAUGE,
  DEVICE_PCA9555,
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
  { "CLIP_APU_ACC_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 1, 0}}, false, false }, // GN1640 Addr 0 Col 1 Row 0,
  { "CLIP_BATT_SW_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 2, 0}}, false, false }, // GN1640 Addr 0 Col 2 Row 0,
  { "CLIP_CK_SEAT_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 0, 0}}, false, false }, // GN1640 Addr 0 Col 0 Row 0,
  { "CLIP_FCES_LT"         , DEVICE_GN1640T , {.gn1640Info = {0, 1, 2}}, false, false }, // GN1640 Addr 0 Col 1 Row 2,
  { "CLIP_FCS_HOT_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 0, 1}}, false, false }, // GN1640 Addr 0 Col 0 Row 1,
  { "CLIP_FUEL_LO_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 0, 2}}, false, false }, // GN1640 Addr 0 Col 0 Row 2,
  { "CLIP_GEN_TIE_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 1, 1}}, false, false }, // GN1640 Addr 0 Col 1 Row 1,
  { "CLIP_L_GEN_LT"        , DEVICE_GN1640T , {.gn1640Info = {0, 0, 3}}, false, false }, // GN1640 Addr 0 Col 0 Row 3,
  { "CLIP_R_GEN_LT"        , DEVICE_GN1640T , {.gn1640Info = {0, 1, 3}}, false, false }, // GN1640 Addr 0 Col 1 Row 3,
  { "CLIP_SPARE_CTN1_LT"   , DEVICE_GN1640T , {.gn1640Info = {0, 2, 1}}, false, false }, // GN1640 Addr 0 Col 2 Row 1,
  { "CLIP_SPARE_CTN2_LT"   , DEVICE_GN1640T , {.gn1640Info = {0, 2, 2}}, false, false }, // GN1640 Addr 0 Col 2 Row 2,
  { "CLIP_SPARE_CTN3_LT"   , DEVICE_GN1640T , {.gn1640Info = {0, 2, 3}}, false, false }, // GN1640 Addr 0 Col 2 Row 3,
  { "HYD_IND_LEFT"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "HYD_IND_RIGHT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LOW_ALT_WARN_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_ALT_PTR"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_GREEN_LAMP"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_HEIGHT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_MIN_HEIGHT_PTR", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_OFF_FLAG"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADALT_GREEN_LAMP", &panelLEDs[21]},
  {"CLIP_FCES_LT", &panelLEDs[3]},
  {"CLIP_SPARE_CTN1_LT", &panelLEDs[9]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[18]},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN2_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {"HYD_IND_RIGHT", &panelLEDs[13]},
  {"CLIP_SPARE_CTN3_LT", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[15]},
  {"INST_PNL_DIMMER", &panelLEDs[17]},
  {"CLIP_APU_ACC_LT", &panelLEDs[0]},
  {"CLIP_FUEL_LO_LT", &panelLEDs[5]},
  {nullptr, nullptr},
  {"RADALT_MIN_HEIGHT_PTR", &panelLEDs[23]},
  {nullptr, nullptr},
  {"CLIP_R_GEN_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_GEN_TIE_LT", &panelLEDs[6]},
  {"CLIP_CK_SEAT_LT", &panelLEDs[2]},
  {"CLIP_FCS_HOT_LT", &panelLEDs[4]},
  {"RADALT_HEIGHT", &panelLEDs[22]},
  {"RADALT_ALT_PTR", &panelLEDs[20]},
  {"RADALT_OFF_FLAG", &panelLEDs[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HYD_IND_LEFT", &panelLEDs[12]},
  {"CHART_DIMMER", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_BATT_SW_LT", &panelLEDs[1]},
  {nullptr, nullptr},
  {"CLIP_L_GEN_LT", &panelLEDs[7]},
  {"LOW_ALT_WARN_LT", &panelLEDs[19]},
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
