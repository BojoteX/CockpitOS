// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_WS2812,
  DEVICE_GAUGE,
  DEVICE_TM1637,
  DEVICE_GN1640T,
  DEVICE_PCA9555,
  DEVICE_GPIO,
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
  { "CANOPY_JETT_HANDLE_PULL"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CANOPY_JETT_HANDLE_UNLOCK", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_APU_ACC_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_BATT_SW_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_CK_SEAT_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_FCES_LT"             , DEVICE_GPIO    , {.gpioInfo = {5}}, false, false }, // GPIO 5,
  { "CLIP_FCS_HOT_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_FUEL_LO_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_GEN_TIE_LT"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_L_GEN_LT"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_R_GEN_LT"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_SPARE_CTN1_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_SPARE_CTN2_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLIP_SPARE_CTN3_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLOCK_ELAPSED_MINUTES"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLOCK_ELAPSED_SECONDS"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLOCK_HOURS"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CLOCK_MINUTES"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AUX_REL_SW"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_DISPENSE_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_JET_SEL_BTN"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CMSD_JET_SEL_L"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "ECM_MODE_SW"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FIRE_LEFT_LT"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LEFT_FIRE_BTN"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LEFT_FIRE_BTN_COVER"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "HMD_OFF_BRT"              , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IR_COOL_SW"               , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SPIN_LT"                  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SPIN_RECOVERY_COVER"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SPIN_RECOVERY_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[67] = {
  {"CANOPY_JETT_HANDLE_PULL", &panelLEDs[0]},
  {"ECM_MODE_SW", &panelLEDs[22]},
  {nullptr, nullptr},
  {"AUX_REL_SW", &panelLEDs[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_R_GEN_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LEFT_FIRE_BTN_COVER", &panelLEDs[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_BATT_SW_LT", &panelLEDs[3]},
  {nullptr, nullptr},
  {"CLIP_GEN_TIE_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLOCK_HOURS", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IR_COOL_SW", &panelLEDs[27]},
  {nullptr, nullptr},
  {"CLIP_CK_SEAT_LT", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_FUEL_LO_LT", &panelLEDs[7]},
  {"CMSD_DISPENSE_SW", &panelLEDs[19]},
  {"CMSD_JET_SEL_L", &panelLEDs[21]},
  {"CMSD_JET_SEL_BTN", &panelLEDs[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLOCK_ELAPSED_SECONDS", &panelLEDs[15]},
  {"SPIN_RECOVERY_COVER", &panelLEDs[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_L_GEN_LT", &panelLEDs[9]},
  {"CLOCK_ELAPSED_MINUTES", &panelLEDs[14]},
  {"SPIN_LT", &panelLEDs[28]},
  {"CLIP_FCES_LT", &panelLEDs[5]},
  {"CLIP_SPARE_CTN3_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {"FIRE_LEFT_LT", &panelLEDs[23]},
  {"CANOPY_JETT_HANDLE_UNLOCK", &panelLEDs[1]},
  {"LEFT_FIRE_BTN", &panelLEDs[24]},
  {"SPIN_RECOVERY_SW", &panelLEDs[30]},
  {"CLOCK_MINUTES", &panelLEDs[17]},
  {"HMD_OFF_BRT", &panelLEDs[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_APU_ACC_LT", &panelLEDs[2]},
  {"CLIP_FCS_HOT_LT", &panelLEDs[6]},
  {"CLIP_SPARE_CTN1_LT", &panelLEDs[11]},
  {"CLIP_SPARE_CTN2_LT", &panelLEDs[12]},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 67;
  for (uint16_t i = 0; i < 67; ++i) {
    uint16_t idx = (startH + i >= 67) ? (startH + i - 67) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
