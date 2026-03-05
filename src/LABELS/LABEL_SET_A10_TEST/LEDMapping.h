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
  { "AHCP_ALT_SCE"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_CICU"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_GUNPAC"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_HUD_DAYNIGHT"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_HUD_MODE"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_IFFCC"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_JTRS"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_LASER_ARM"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_MASTER_ARM"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AHCP_TGP"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ANTI_SKID_SWITCH"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "DOWNLOCK_OVERRIDE"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENGINE_TEMS_DATA"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLAP_POS"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GEAR_HORN_SILENCE"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GEAR_LEVER"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GEAR_L_SAFE"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GEAR_N_SAFE"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "GEAR_R_SAFE"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "HANDLE_GEAR_WARNING", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LANDING_LIGHTS"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {"AHCP_ALT_SCE", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GEAR_L_SAFE", &panelLEDs[16]},
  {"AHCP_HUD_DAYNIGHT", &panelLEDs[3]},
  {"FLAP_POS", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GEAR_N_SAFE", &panelLEDs[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_IFFCC", &panelLEDs[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_GUNPAC", &panelLEDs[2]},
  {"GEAR_R_SAFE", &panelLEDs[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_TEMS_DATA", &panelLEDs[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_HUD_MODE", &panelLEDs[4]},
  {"AHCP_JTRS", &panelLEDs[6]},
  {"AHCP_LASER_ARM", &panelLEDs[7]},
  {"AHCP_CICU", &panelLEDs[1]},
  {"GEAR_LEVER", &panelLEDs[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GEAR_HORN_SILENCE", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_TGP", &panelLEDs[9]},
  {nullptr, nullptr},
  {"DOWNLOCK_OVERRIDE", &panelLEDs[11]},
  {"ANTI_SKID_SWITCH", &panelLEDs[10]},
  {nullptr, nullptr},
  {"AHCP_MASTER_ARM", &panelLEDs[8]},
  {nullptr, nullptr},
  {"HANDLE_GEAR_WARNING", &panelLEDs[19]},
  {"LANDING_LIGHTS", &panelLEDs[20]},
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
