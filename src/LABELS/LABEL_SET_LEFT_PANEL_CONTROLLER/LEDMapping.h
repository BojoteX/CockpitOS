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
  { "COMM1_ANT_SELECT_SW"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFF_ANT_SELECT_SW"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "APU_CONTROL_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "APU_READY_LT"          , DEVICE_GPIO    , {.gpioInfo = {PIN(17)}}, false, false }, // GPIO PIN(17),
  { "ENGINE_CRANK_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_AUX"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_COMM_G_XMT_SW"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_COMM_RELAY_SW"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_CRYPTO_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_ICS"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_IFF_MASTER_SW"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_IFF_MODE4_SW"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_ILS_CHANNEL_SW"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_ILS_UFC_MAN_SW"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_MIDS_A"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_MIDS_B"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_RWR"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_TACAN"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_VOX"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COM_WPN"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FCS_RESET_BTN"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GAIN_SWITCH"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GAIN_SWITCH_COVER"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RUD_TRIM"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "TO_TRIM_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "EXT_CNT_TANK_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "EXT_WNG_TANK_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FUEL_DUMP_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "PROBE_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"       , DEVICE_GPIO    , {.gpioInfo = {PIN(12)}}, true, false }, // GPIO PIN(12),
  { "FLOOD_DIMMER"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LIGHTS_TEST_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "OBOGS_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "OXY_FLOW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LEFT_VIDEO_BIT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "NUC_WPN_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RIGHT_VIDEO_BIT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[83] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PROBE_SW", &panelLEDs[28]},
  {"GAIN_SWITCH", &panelLEDs[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_COMM_G_XMT_SW", &panelLEDs[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_ILS_CHANNEL_SW", &panelLEDs[12]},
  {"COM_IFF_MASTER_SW", &panelLEDs[10]},
  {"IFF_ANT_SELECT_SW", &panelLEDs[1]},
  {"FUEL_DUMP_SW", &panelLEDs[27]},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[30]},
  {"EXT_WNG_TANK_SW", &panelLEDs[26]},
  {"FCS_RESET_BTN", &panelLEDs[20]},
  {"COM_RWR", &panelLEDs[16]},
  {"LIGHTS_TEST_SW", &panelLEDs[34]},
  {"OXY_FLOW", &panelLEDs[37]},
  {nullptr, nullptr},
  {"COM_WPN", &panelLEDs[19]},
  {nullptr, nullptr},
  {"COM_AUX", &panelLEDs[5]},
  {"COM_TACAN", &panelLEDs[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"TO_TRIM_BTN", &panelLEDs[24]},
  {"INST_PNL_DIMMER", &panelLEDs[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RUD_TRIM", &panelLEDs[23]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[35]},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW", &panelLEDs[4]},
  {"LEFT_VIDEO_BIT", &panelLEDs[38]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_ICS", &panelLEDs[9]},
  {"NUC_WPN_SW", &panelLEDs[39]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COM_IFF_MODE4_SW", &panelLEDs[11]},
  {"EXT_CNT_TANK_SW", &panelLEDs[25]},
  {nullptr, nullptr},
  {"COM_ILS_UFC_MAN_SW", &panelLEDs[13]},
  {"COM_VOX", &panelLEDs[18]},
  {"COM_MIDS_B", &panelLEDs[15]},
  {"FLOOD_DIMMER", &panelLEDs[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RIGHT_VIDEO_BIT", &panelLEDs[40]},
  {"COMM1_ANT_SELECT_SW", &panelLEDs[0]},
  {nullptr, nullptr},
  {"COM_COMM_RELAY_SW", &panelLEDs[7]},
  {"COM_MIDS_A", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GAIN_SWITCH_COVER", &panelLEDs[22]},
  {"OBOGS_SW", &panelLEDs[36]},
  {nullptr, nullptr},
  {"APU_CONTROL_SW", &panelLEDs[2]},
  {nullptr, nullptr},
  {"APU_READY_LT", &panelLEDs[3]},
  {"COM_CRYPTO_SW", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &panelLEDs[31]},
  {"CHART_DIMMER", &panelLEDs[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 83;
  for (uint16_t i = 0; i < 83; ++i) {
    uint16_t idx = (startH + i >= 83) ? (startH + i - 83) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
