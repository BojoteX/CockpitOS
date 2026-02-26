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
  { "DEFOG_HANDLE"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WSHIELD_ANTI_ICE_SW"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "BATTERY_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "L_GEN_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "R_GEN_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "VOLT_E"                , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "VOLT_U"                , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "BLEED_AIR_KNOB"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "BLEED_AIR_PULL"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CABIN_PRESS_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CABIN_TEMP"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ECS_MODE_SW"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "ENG_ANTIICE_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "PITOT_HEAT_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "SUIT_TEMP"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CONSOLES_DIMMER"       , DEVICE_GPIO    , {.gpioInfo = {PIN(14)}}, true, false }, // GPIO PIN(14),
  { "FLOOD_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LIGHTS_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "WARN_CAUTION_DIMMER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CANOPY_POS"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CANOPY_SW"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_FCS_CHAN3"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_FCS_CHAN4"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_HOOOK"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_LG"                 , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "FCS_BIT_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "FLIR_SW"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "INS_SW"                , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "LST_NFLR_SW"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "LTD_R_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RADAR_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RADAR_SW_PULL"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[71] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENG_ANTIICE_SW", &panelLEDs[12]},
  {"CANOPY_POS", &panelLEDs[22]},
  {"FCS_BIT_SW", &panelLEDs[28]},
  {"BLEED_AIR_PULL", &panelLEDs[8]},
  {"CB_HOOOK", &panelLEDs[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[16]},
  {"CB_FCS_CHAN3", &panelLEDs[24]},
  {nullptr, nullptr},
  {"CABIN_TEMP", &panelLEDs[10]},
  {"CABIN_PRESS_SW", &panelLEDs[9]},
  {"CANOPY_SW", &panelLEDs[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LST_NFLR_SW", &panelLEDs[31]},
  {"PITOT_HEAT_SW", &panelLEDs[13]},
  {nullptr, nullptr},
  {"INS_SW", &panelLEDs[30]},
  {"CHART_DIMMER", &panelLEDs[15]},
  {"LTD_R_SW", &panelLEDs[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CB_FCS_CHAN4", &panelLEDs[25]},
  {nullptr, nullptr},
  {"VOLT_E", &panelLEDs[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"DEFOG_HANDLE", &panelLEDs[0]},
  {"WSHIELD_ANTI_ICE_SW", &panelLEDs[1]},
  {"CONSOLES_DIMMER", &panelLEDs[17]},
  {nullptr, nullptr},
  {"RADAR_SW_PULL", &panelLEDs[34]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SUIT_TEMP", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[19]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"VOLT_U", &panelLEDs[6]},
  {"LIGHTS_TEST_SW", &panelLEDs[20]},
  {nullptr, nullptr},
  {"CB_LG", &panelLEDs[27]},
  {nullptr, nullptr},
  {"ECS_MODE_SW", &panelLEDs[11]},
  {"L_GEN_SW", &panelLEDs[3]},
  {nullptr, nullptr},
  {"BLEED_AIR_KNOB", &panelLEDs[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"R_GEN_SW", &panelLEDs[4]},
  {"FLIR_SW", &panelLEDs[29]},
  {"RADAR_SW", &panelLEDs[33]},
  {nullptr, nullptr},
  {"BATTERY_SW", &panelLEDs[2]},
  {"FLOOD_DIMMER", &panelLEDs[18]},
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
