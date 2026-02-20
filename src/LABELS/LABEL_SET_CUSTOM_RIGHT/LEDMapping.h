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
  { "ARRESTING_HOOK_LT"     , DEVICE_GPIO    , {.gpioInfo = {PIN(2)}}, false, true }, // GPIO PIN(2),
  { "HOOK_LEVER"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_APU_ACC_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 1, 0}}, false, false }, // GN1640 Addr 0 Col 1 Row 0,
  { "CLIP_BATT_SW_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 2, 0}}, false, false }, // GN1640 Addr 0 Col 2 Row 0,
  { "CLIP_CK_SEAT_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 0, 0}}, false, false }, // GN1640 Addr 0 Col 0 Row 0,
  { "CLIP_FCES_LT"          , DEVICE_GN1640T , {.gn1640Info = {0, 1, 2}}, false, false }, // GN1640 Addr 0 Col 1 Row 2,
  { "CLIP_FCS_HOT_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 0, 1}}, false, false }, // GN1640 Addr 0 Col 0 Row 1,
  { "CLIP_FUEL_LO_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 0, 2}}, false, false }, // GN1640 Addr 0 Col 0 Row 2,
  { "CLIP_GEN_TIE_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 1, 1}}, false, false }, // GN1640 Addr 0 Col 1 Row 1,
  { "CLIP_L_GEN_LT"         , DEVICE_GN1640T , {.gn1640Info = {0, 0, 3}}, false, false }, // GN1640 Addr 0 Col 0 Row 3,
  { "CLIP_R_GEN_LT"         , DEVICE_GN1640T , {.gn1640Info = {0, 1, 3}}, false, false }, // GN1640 Addr 0 Col 1 Row 3,
  { "CLIP_SPARE_CTN1_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 2, 1}}, false, false }, // GN1640 Addr 0 Col 2 Row 1,
  { "CLIP_SPARE_CTN2_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 2, 2}}, false, false }, // GN1640 Addr 0 Col 2 Row 2,
  { "CLIP_SPARE_CTN3_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 2, 3}}, false, false }, // GN1640 Addr 0 Col 2 Row 3,
  { "AV_COOL_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "HYD_IND_LEFT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "HYD_IND_RIGHT"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"       , DEVICE_GPIO    , {.gpioInfo = {PIN(6)}}, true, false }, // GPIO PIN(6),
  { "LIGHTS_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LOW_ALT_WARN_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_ALT_PTR"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_GREEN_LAMP"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_HEIGHT"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_MIN_HEIGHT_PTR" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_OFF_FLAG"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WING_FOLD_PULL"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WING_FOLD_ROTATE"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WING_FOLD_PULL"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[71] = {
  {"LOW_ALT_WARN_LT", &panelLEDs[24]},
  {nullptr, nullptr},
  {"WING_FOLD_ROTATE", &panelLEDs[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_FCS_HOT_LT", &panelLEDs[6]},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN1_LT", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_GEN_TIE_LT", &panelLEDs[8]},
  {"ARRESTING_HOOK_LT", &panelLEDs[0]},
  {nullptr, nullptr},
  {"CHART_DIMMER", &panelLEDs[17]},
  {"CLIP_SPARE_CTN2_LT", &panelLEDs[12]},
  {nullptr, nullptr},
  {"AV_COOL_SW", &panelLEDs[14]},
  {"HOOK_LEVER", &panelLEDs[1]},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN3_LT", &panelLEDs[13]},
  {"RADALT_MIN_HEIGHT_PTR", &panelLEDs[28]},
  {"RADALT_OFF_FLAG", &panelLEDs[29]},
  {nullptr, nullptr},
  {"HYD_IND_LEFT", &panelLEDs[15]},
  {"CLIP_CK_SEAT_LT", &panelLEDs[4]},
  {"CLIP_FUEL_LO_LT", &panelLEDs[7]},
  {"CLIP_BATT_SW_LT", &panelLEDs[3]},
  {"CONSOLES_DIMMER", &panelLEDs[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HYD_IND_RIGHT", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[21]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_FCES_LT", &panelLEDs[5]},
  {"RADALT_GREEN_LAMP", &panelLEDs[26]},
  {"WING_FOLD_PULL", &panelLEDs[31]},
  {"WING_FOLD_PULL", &panelLEDs[33]},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW", &panelLEDs[22]},
  {"RADALT_HEIGHT", &panelLEDs[27]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADALT_TEST_SW", &panelLEDs[30]},
  {"CLIP_L_GEN_LT", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_APU_ACC_LT", &panelLEDs[2]},
  {"CLIP_R_GEN_LT", &panelLEDs[10]},
  {"RADALT_ALT_PTR", &panelLEDs[25]},
  {"FLOOD_DIMMER", &panelLEDs[20]},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 71;
  for (uint16_t i = 0; i < 71; ++i) {
    uint16_t idx = (startH + i >= 71) ? (startH + i - 71) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
