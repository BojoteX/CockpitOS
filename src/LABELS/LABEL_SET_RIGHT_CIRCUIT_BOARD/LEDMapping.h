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
  { "CHART_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLE_INT_LT"     , DEVICE_GPIO    , {.gpioInfo = {6}}, true, false }, // GPIO 6,
  { "EMERG_INSTR_INT_LT" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ENG_INSTR_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_BTN_INT_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IFEI_DISP_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INSTR_INT_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "NVG_FLOOD_INT_LT"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "STBY_COMPASS_INT_LT", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CB_FCS_CHAN3"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_FCS_CHAN4"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_HOOOK"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "CB_LG"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "FCS_BIT_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EMERG_INSTR_INT_LT", &panelLEDs[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_BTN_INT_LT", &panelLEDs[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLE_INT_LT", &panelLEDs[1]},
  {"FCS_BIT_SW", &panelLEDs[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_INT_LT", &panelLEDs[4]},
  {"CB_HOOOK", &panelLEDs[12]},
  {"INSTR_INT_LT", &panelLEDs[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DISP_INT_LT", &panelLEDs[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CB_FCS_CHAN3", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENG_INSTR_INT_LT", &panelLEDs[3]},
  {"NVG_FLOOD_INT_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_INT_LT", &panelLEDs[0]},
  {nullptr, nullptr},
  {"STBY_COMPASS_INT_LT", &panelLEDs[9]},
  {"CB_FCS_CHAN4", &panelLEDs[11]},
  {"CB_LG", &panelLEDs[13]},
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
