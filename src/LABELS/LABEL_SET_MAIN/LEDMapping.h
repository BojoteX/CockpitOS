// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_GPIO,
  DEVICE_PCA9555,
  DEVICE_NONE,
  DEVICE_WS2812,
  DEVICE_TM1637,
  DEVICE_GAUGE,
  DEVICE_GN1640T,
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
  { "APU_FIRE_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FIRE_APU_LT"            , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 5, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 5 Bit 0,
  { "AOA_INDEXER_HIGH"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AOA_INDEXER_HIGH_F"     , DEVICE_WS2812  , {.ws2812Info = {3, WS2812B_PIN,   0, 255,   0, 200}}, true, false }, // WS2812 Index 3,
  { "AOA_INDEXER_LOW"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AOA_INDEXER_LOW_F"      , DEVICE_WS2812  , {.ws2812Info = {5, WS2812B_PIN, 255,   0,   0, 200}}, true, false }, // WS2812 Index 5,
  { "AOA_INDEXER_NORMAL"     , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "AOA_INDEXER_NORMAL_F"   , DEVICE_WS2812  , {.ws2812Info = {4, WS2812B_PIN, 255, 165,   0, 200}}, true, false }, // WS2812 Index 4,
  { "EMER_JETT_BTN"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FIRE_EXT_BTN"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CHART_DIMMER"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "COCKKPIT_LIGHT_MODE_SW" , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"           , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"        , DEVICE_GPIO    , {.gpioInfo = {PIN(6)}}, true, false }, // GPIO PIN(6),
  { "LIGHTS_TEST_SW"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "WARN_CAUTION_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LH_ADV_ASPJ_OH"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 5, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 5 Bit 1,
  { "LH_ADV_GO"              , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 0, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 0 Bit 0,
  { "LH_ADV_L_BAR_GREEN"     , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 4, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 4 Bit 0,
  { "LH_ADV_L_BAR_RED"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 3, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 3 Bit 0,
  { "LH_ADV_L_BLEED"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 1, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 1 Bit 0,
  { "LH_ADV_NO_GO"           , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 0, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 0 Bit 1,
  { "LH_ADV_REC"             , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 3, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 3 Bit 1,
  { "LH_ADV_R_BLEED"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 1, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 1 Bit 1,
  { "LH_ADV_SPD_BRK"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 2, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 2 Bit 0,
  { "LH_ADV_STBY"            , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 2, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 2 Bit 1,
  { "LH_ADV_XMIT"            , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 4, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 4 Bit 1,
  { "FIRE_LEFT_LT"           , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 0, 2}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 0 Bit 2,
  { "LEFT_FIRE_BTN"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LEFT_FIRE_BTN_COVER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LS_LOCK"                , DEVICE_WS2812  , {.ws2812Info = {0, WS2812B_PIN,   0, 255,   0, 255}}, false, false }, // WS2812 Index 0,
  { "LS_SHOOT"               , DEVICE_WS2812  , {.ws2812Info = {1, WS2812B_PIN,   0, 255,   0, 255}}, false, false }, // WS2812 Index 1,
  { "LS_SHOOT_STROBE"        , DEVICE_WS2812  , {.ws2812Info = {2, WS2812B_PIN,   0, 255,   0, 255}}, false, false }, // WS2812 Index 2,
  { "HMD_OFF_BRT"            , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "IR_COOL_SW"             , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SPIN_LT"                , DEVICE_GPIO    , {.gpioInfo = {PIN(34)}}, false, false }, // GPIO PIN(34),
  { "SPIN_RECOVERY_COVER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SPIN_RECOVERY_SW"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_ARM_SW"          , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AA_LT"      , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 3}}, false, true }, // PCA 0x5B Port 1 Bit 3,
  { "MASTER_MODE_AG"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "MASTER_MODE_AG_LT"      , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 4}}, false, true }, // PCA 0x5B Port 1 Bit 4,
  { "MC_DISCH"               , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 6}}, false, true }, // PCA 0x5B Port 1 Bit 6,
  { "MC_READY"               , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 5}}, false, true }, // PCA 0x5B Port 1 Bit 5,
  { "MASTER_CAUTION_LT"      , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(39), 5, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(39) Seg 5 Bit 0,
  { "MASTER_CAUTION_RESET_SW", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RH_ADV_AAA"             , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 4, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 4 Bit 0,
  { "RH_ADV_AI"              , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 4, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 4 Bit 1,
  { "RH_ADV_CW"              , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 5, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 5 Bit 1,
  { "RH_ADV_DISP"            , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 0, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 0 Bit 0,
  { "RH_ADV_RCDR_ON"         , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 0, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 0 Bit 1,
  { "RH_ADV_SAM"             , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 3, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 3 Bit 0,
  { "RH_ADV_SPARE_RH1"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 1, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 1 Bit 1,
  { "RH_ADV_SPARE_RH2"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 2, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 2 Bit 1,
  { "RH_ADV_SPARE_RH3"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 3, 1}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 3 Bit 1,
  { "RH_ADV_SPARE_RH4"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 1, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 1 Bit 0,
  { "RH_ADV_SPARE_RH5"       , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 2, 0}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 2 Bit 0,
  { "FIRE_RIGHT_LT"          , DEVICE_TM1637  , {.tm1637Info = {PIN(37), PIN(40), 0, 2}}, false, false }, // TM1637 CLK PIN(37) DIO PIN(40) Seg 0 Bit 2,
  { "RIGHT_FIRE_BTN"         , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "RIGHT_FIRE_BTN_COVER"   , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[127] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AOA_INDEXER_LOW_F", &panelLEDs[5]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[16]},
  {"LH_ADV_L_BAR_GREEN", &panelLEDs[19]},
  {"MASTER_CAUTION_LT", &panelLEDs[46]},
  {"CONSOLES_DIMMER", &panelLEDs[12]},
  {"SPIN_RECOVERY_SW", &panelLEDs[38]},
  {"LEFT_FIRE_BTN_COVER", &panelLEDs[30]},
  {nullptr, nullptr},
  {"SPIN_RECOVERY_COVER", &panelLEDs[37]},
  {"MASTER_MODE_AG_LT", &panelLEDs[43]},
  {"RH_ADV_AI", &panelLEDs[49]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_GO", &panelLEDs[18]},
  {"LH_ADV_R_BLEED", &panelLEDs[24]},
  {"AOA_INDEXER_NORMAL", &panelLEDs[6]},
  {"HMD_OFF_BRT", &panelLEDs[34]},
  {"LH_ADV_XMIT", &panelLEDs[27]},
  {"CHART_DIMMER", &panelLEDs[10]},
  {"MASTER_MODE_AG", &panelLEDs[42]},
  {"RH_ADV_SPARE_RH2", &panelLEDs[55]},
  {nullptr, nullptr},
  {"LS_SHOOT", &panelLEDs[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_LEFT_LT", &panelLEDs[28]},
  {"LH_ADV_REC", &panelLEDs[23]},
  {"MC_DISCH", &panelLEDs[44]},
  {"RH_ADV_DISP", &panelLEDs[51]},
  {"RIGHT_FIRE_BTN", &panelLEDs[60]},
  {"LH_ADV_NO_GO", &panelLEDs[22]},
  {"MC_READY", &panelLEDs[45]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AOA_INDEXER_HIGH", &panelLEDs[2]},
  {"RH_ADV_CW", &panelLEDs[50]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_ASPJ_OH", &panelLEDs[17]},
  {"LH_ADV_SPD_BRK", &panelLEDs[25]},
  {nullptr, nullptr},
  {"RH_ADV_SPARE_RH3", &panelLEDs[56]},
  {nullptr, nullptr},
  {"LS_LOCK", &panelLEDs[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_RESET_SW", &panelLEDs[47]},
  {"APU_FIRE_BTN", &panelLEDs[0]},
  {"FLOOD_DIMMER", &panelLEDs[13]},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW", &panelLEDs[15]},
  {nullptr, nullptr},
  {"LH_ADV_L_BLEED", &panelLEDs[21]},
  {"FIRE_RIGHT_LT", &panelLEDs[59]},
  {"RIGHT_FIRE_BTN_COVER", &panelLEDs[61]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &panelLEDs[14]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW", &panelLEDs[11]},
  {"LH_ADV_L_BAR_RED", &panelLEDs[20]},
  {nullptr, nullptr},
  {"LH_ADV_STBY", &panelLEDs[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EMER_JETT_BTN", &panelLEDs[8]},
  {"RH_ADV_SPARE_RH4", &panelLEDs[57]},
  {"LEFT_FIRE_BTN", &panelLEDs[29]},
  {"FIRE_EXT_BTN", &panelLEDs[9]},
  {"IR_COOL_SW", &panelLEDs[35]},
  {"MASTER_ARM_SW", &panelLEDs[39]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LS_SHOOT_STROBE", &panelLEDs[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AA_LT", &panelLEDs[41]},
  {"RH_ADV_RCDR_ON", &panelLEDs[52]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_APU_LT", &panelLEDs[1]},
  {"RH_ADV_AAA", &panelLEDs[48]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AOA_INDEXER_LOW", &panelLEDs[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RH_ADV_SPARE_RH5", &panelLEDs[58]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AA", &panelLEDs[40]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RH_ADV_SAM", &panelLEDs[53]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AOA_INDEXER_HIGH_F", &panelLEDs[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AOA_INDEXER_NORMAL_F", &panelLEDs[7]},
  {"SPIN_LT", &panelLEDs[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RH_ADV_SPARE_RH1", &panelLEDs[54]},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 127;
  for (uint16_t i = 0; i < 127; ++i) {
    uint16_t idx = (startH + i >= 127) ? (startH + i - 127) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
