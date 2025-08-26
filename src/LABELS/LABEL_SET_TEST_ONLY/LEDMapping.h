// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_PCA9555,
  DEVICE_WS2812,
  DEVICE_GAUGE,
  DEVICE_GN1640T,
  DEVICE_TM1637,
  DEVICE_GPIO,
  DEVICE_NONE,
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
  { "RWR_AUDIO_CTRL"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_BIT_LT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_DISPLAY_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_DMR_CTRL"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_ENABLE_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_FAIL_LT"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_LIMIT_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_LOWER_LT"      , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_LT_BRIGHT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_OFFSET_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_RWR_INTESITY"  , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_SPECIAL_EN_LT" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "RWR_SPECIAL_LT"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false },
  { "FIRE_RIGHT_LT"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[79] = {
  {"RH_ADV_SPARE_RH2", &panelLEDs[21]},
  {"RH_ADV_SPARE_RH3", &panelLEDs[22]},
  {"LH_ADV_L_BAR_GREEN", &panelLEDs[3]},
  {"LH_ADV_NO_GO", &panelLEDs[6]},
  {"RH_ADV_SPARE_RH4", &panelLEDs[23]},
  {"RH_ADV_SPARE_RH5", &panelLEDs[24]},
  {"RWR_SPECIAL_EN_LT", &panelLEDs[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_SPD_BRK", &panelLEDs[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_L_BAR_RED", &panelLEDs[4]},
  {"RWR_DISPLAY_LT", &panelLEDs[27]},
  {"RWR_ENABLE_LT", &panelLEDs[29]},
  {"LH_ADV_ASPJ_OH", &panelLEDs[1]},
  {"RWR_BIT_LT", &panelLEDs[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_LT", &panelLEDs[13]},
  {nullptr, nullptr},
  {"RWR_RWR_INTESITY", &panelLEDs[35]},
  {nullptr, nullptr},
  {"RWR_LOWER_LT", &panelLEDs[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_REC", &panelLEDs[7]},
  {"RH_ADV_DISP", &panelLEDs[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RH_ADV_AAA", &panelLEDs[14]},
  {"RWR_LT_BRIGHT", &panelLEDs[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_R_BLEED", &panelLEDs[8]},
  {"FIRE_LEFT_LT", &panelLEDs[12]},
  {"RWR_AUDIO_CTRL", &panelLEDs[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_XMIT", &panelLEDs[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_L_BLEED", &panelLEDs[5]},
  {"RH_ADV_RCDR_ON", &panelLEDs[18]},
  {"RH_ADV_AI", &panelLEDs[15]},
  {"RH_ADV_CW", &panelLEDs[16]},
  {"RWR_DMR_CTRL", &panelLEDs[28]},
  {"RWR_LIMIT_LT", &panelLEDs[31]},
  {"RWR_OFFSET_LT", &panelLEDs[34]},
  {"RH_ADV_SAM", &panelLEDs[19]},
  {"LH_ADV_STBY", &panelLEDs[10]},
  {"FIRE_RIGHT_LT", &panelLEDs[38]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_FAIL_LT", &panelLEDs[30]},
  {nullptr, nullptr},
  {"RWR_SPECIAL_LT", &panelLEDs[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_APU_LT", &panelLEDs[0]},
  {"LH_ADV_GO", &panelLEDs[2]},
  {"RH_ADV_SPARE_RH1", &panelLEDs[20]},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 79;
  for (uint16_t i = 0; i < 79; ++i) {
    uint16_t idx = (startH + i >= 79) ? (startH + i - 79) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
