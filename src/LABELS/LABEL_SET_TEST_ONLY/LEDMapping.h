// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_GN1640T,
  DEVICE_WS2812,
  DEVICE_NONE,
  DEVICE_PCA9555,
  DEVICE_GAUGE,
  DEVICE_GPIO,
  DEVICE_TM1637,
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
    struct { uint8_t index; } ws2812Info;
  } info;
  bool dimmable;
  bool activeLow;
};

// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "FIRE_APU_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LH_ADV_ASPJ_OH"    , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 5, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 5 Bit 1,
  { "LH_ADV_GO"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 0, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 0 Bit 0,
  { "LH_ADV_L_BAR_GREEN", DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 4, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 4 Bit 0,
  { "LH_ADV_L_BAR_RED"  , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 3, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 3 Bit 0,
  { "LH_ADV_L_BLEED"    , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 1, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 1 Bit 0,
  { "LH_ADV_NO_GO"      , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 0, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 0 Bit 1,
  { "LH_ADV_REC"        , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 3, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 3 Bit 1,
  { "LH_ADV_R_BLEED"    , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 1, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 1 Bit 1,
  { "LH_ADV_SPD_BRK"    , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 2, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 2 Bit 0,
  { "LH_ADV_STBY"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 2, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 2 Bit 1,
  { "LH_ADV_XMIT"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 4, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 4 Bit 1,
  { "FIRE_LEFT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_CAUTION_LT" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RH_ADV_AAA"        , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 4, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 4 Bit 0,
  { "RH_ADV_AI"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 4, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 4 Bit 1,
  { "RH_ADV_CW"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 5, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 5 Bit 1,
  { "RH_ADV_DISP"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 0, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 0 Bit 0,
  { "RH_ADV_RCDR_ON"    , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 0, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 0 Bit 1,
  { "RH_ADV_SAM"        , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 3, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 3 Bit 0,
  { "RH_ADV_SPARE_RH1"  , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 1, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 1 Bit 1,
  { "RH_ADV_SPARE_RH2"  , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 2, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 2 Bit 1,
  { "RH_ADV_SPARE_RH3"  , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 3, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 3 Bit 1,
  { "RH_ADV_SPARE_RH4"  , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 1, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 1 Bit 0,
  { "RH_ADV_SPARE_RH5"  , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 2, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 2 Bit 0,
  { "FIRE_RIGHT_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_APU_LT", &panelLEDs[0]},
  {"RH_ADV_AI", &panelLEDs[15]},
  {"RH_ADV_DISP", &panelLEDs[17]},
  {"LH_ADV_REC", &panelLEDs[7]},
  {"LH_ADV_SPD_BRK", &panelLEDs[9]},
  {"LH_ADV_ASPJ_OH", &panelLEDs[1]},
  {"LH_ADV_L_BLEED", &panelLEDs[5]},
  {"LH_ADV_R_BLEED", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RH_ADV_AAA", &panelLEDs[14]},
  {nullptr, nullptr},
  {"LH_ADV_GO", &panelLEDs[2]},
  {"LH_ADV_XMIT", &panelLEDs[11]},
  {"RH_ADV_SAM", &panelLEDs[19]},
  {"RH_ADV_SPARE_RH1", &panelLEDs[20]},
  {"RH_ADV_SPARE_RH2", &panelLEDs[21]},
  {"LH_ADV_L_BAR_GREEN", &panelLEDs[3]},
  {"RH_ADV_SPARE_RH3", &panelLEDs[22]},
  {"LH_ADV_L_BAR_RED", &panelLEDs[4]},
  {"RH_ADV_RCDR_ON", &panelLEDs[18]},
  {"RH_ADV_SPARE_RH4", &panelLEDs[23]},
  {"RH_ADV_SPARE_RH5", &panelLEDs[24]},
  {"RH_ADV_CW", &panelLEDs[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_LEFT_LT", &panelLEDs[12]},
  {"LH_ADV_STBY", &panelLEDs[10]},
  {"FIRE_RIGHT_LT", &panelLEDs[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_NO_GO", &panelLEDs[6]},
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
