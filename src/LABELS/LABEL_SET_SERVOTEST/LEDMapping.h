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
  { "APU_CONTROL_SW"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "APU_READY_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENGINE_CRANK_SW"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLIR_SW"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INS_SW"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LST_NFLR_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LTD_R_SW"           , DEVICE_GAUGE   , {.gaugeInfo = {12, 800, 2200, 20000}}, false, false }, // GAUGE GPIO 12,
  { "RADAR_SW"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADAR_SW_PULL"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INT_THROTTLE_LEFT"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INT_THROTTLE_RIGHT" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_ATC_SW"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_CAGE_BTN"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_DISP_SW"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_EXT_L_SW"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_FOV_SEL_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_FRICTION"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_RADAR_ELEV", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_SPEED_BRK" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADAR_SW_PULL", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_ATC_SW", &panelLEDs[11]},
  {"APU_CONTROL_SW", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW", &panelLEDs[2]},
  {"THROTTLE_RADAR_ELEV", &panelLEDs[17]},
  {nullptr, nullptr},
  {"RADAR_SW", &panelLEDs[7]},
  {"THROTTLE_FRICTION", &panelLEDs[16]},
  {nullptr, nullptr},
  {"THROTTLE_SPEED_BRK", &panelLEDs[18]},
  {"LST_NFLR_SW", &panelLEDs[5]},
  {"THROTTLE_FOV_SEL_SW", &panelLEDs[15]},
  {nullptr, nullptr},
  {"THROTTLE_EXT_L_SW", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_DISP_SW", &panelLEDs[13]},
  {nullptr, nullptr},
  {"FLIR_SW", &panelLEDs[3]},
  {"INS_SW", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INT_THROTTLE_RIGHT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_READY_LT", &panelLEDs[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LTD_R_SW", &panelLEDs[6]},
  {"INT_THROTTLE_LEFT", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_CAGE_BTN", &panelLEDs[12]},
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
