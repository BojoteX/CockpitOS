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
  { "PRESSURE_ALT"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_ARM_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AG"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AG_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MC_DISCH"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MC_READY"               , DEVICE_GPIO    , {.gpioInfo = {RS485_TEST_LED_GPIO}}, false, false }, // GPIO RS485_TEST_LED_GPIO,
  { "MASTER_CAUTION_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_CAUTION_RESET_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LOW_ALT_WARN_LT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_ALT_PTR"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_GREEN_LAMP"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_HEIGHT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_MIN_HEIGHT_PTR"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_OFF_FLAG"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RADALT_TEST_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INT_THROTTLE_LEFT"      , DEVICE_GAUGE   , {.gaugeInfo = {3, 800, 2200, 20000}}, false, false }, // GAUGE GPIO 3,
  { "INT_THROTTLE_RIGHT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_ATC_SW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_CAGE_BTN"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_DISP_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_EXT_L_SW"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_FOV_SEL_SW"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_FRICTION"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_RADAR_ELEV"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "THROTTLE_SPEED_BRK"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[59] = {
  {"THROTTLE_ATC_SW", &panelLEDs[19]},
  {nullptr, nullptr},
  {"MC_DISCH", &panelLEDs[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AA", &panelLEDs[2]},
  {"RADALT_ALT_PTR", &panelLEDs[11]},
  {"MASTER_MODE_AA_LT", &panelLEDs[3]},
  {"RADALT_OFF_FLAG", &panelLEDs[15]},
  {"THROTTLE_CAGE_BTN", &panelLEDs[20]},
  {"MASTER_MODE_AG", &panelLEDs[4]},
  {"RADALT_MIN_HEIGHT_PTR", &panelLEDs[14]},
  {"MASTER_MODE_AG_LT", &panelLEDs[5]},
  {"MASTER_ARM_SW", &panelLEDs[1]},
  {"THROTTLE_EXT_L_SW", &panelLEDs[22]},
  {"THROTTLE_FRICTION", &panelLEDs[24]},
  {"MC_READY", &panelLEDs[7]},
  {"THROTTLE_SPEED_BRK", &panelLEDs[26]},
  {"PRESSURE_ALT", &panelLEDs[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INT_THROTTLE_LEFT", &panelLEDs[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_RESET_SW", &panelLEDs[9]},
  {"THROTTLE_RADAR_ELEV", &panelLEDs[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_DISP_SW", &panelLEDs[21]},
  {"RADALT_TEST_SW", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LOW_ALT_WARN_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADALT_HEIGHT", &panelLEDs[13]},
  {"RADALT_GREEN_LAMP", &panelLEDs[12]},
  {"THROTTLE_FOV_SEL_SW", &panelLEDs[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INT_THROTTLE_RIGHT", &panelLEDs[18]},
  {nullptr, nullptr},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 59;
  for (uint16_t i = 0; i < 59; ++i) {
    uint16_t idx = (startH + i >= 59) ? (startH + i - 59) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
