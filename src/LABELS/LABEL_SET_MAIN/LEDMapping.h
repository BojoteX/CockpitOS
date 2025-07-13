// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

// Embedded LEDMapping structure and enums
enum LEDDeviceType {
  DEVICE_TM1637,
  DEVICE_NONE,
  DEVICE_GAUGE,
  DEVICE_GN1640T,
  DEVICE_WS2812,
  DEVICE_GPIO,
  DEVICE_PCA9555,
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
  { "FIRE_APU_LT"        , DEVICE_TM1637  , {.tm1637Info = {37, 40, 5, 0}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 5 Bit 0,
  { "CLIP_APU_ACC_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 1, 0}}, false, false }, // GN1640 Addr 0 Col 1 Row 0,
  { "CLIP_BATT_SW_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 2, 0}}, false, false }, // GN1640 Addr 0 Col 2 Row 0,
  { "CLIP_CK_SEAT_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 0, 0}}, false, false }, // GN1640 Addr 0 Col 0 Row 0,
  { "CLIP_FCES_LT"       , DEVICE_GN1640T , {.gn1640Info = {0, 1, 2}}, false, false }, // GN1640 Addr 0 Col 1 Row 2,
  { "CLIP_FCS_HOT_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 0, 1}}, false, false }, // GN1640 Addr 0 Col 0 Row 1,
  { "CLIP_FUEL_LO_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 0, 2}}, false, false }, // GN1640 Addr 0 Col 0 Row 2,
  { "CLIP_GEN_TIE_LT"    , DEVICE_GN1640T , {.gn1640Info = {0, 1, 1}}, false, false }, // GN1640 Addr 0 Col 1 Row 1,
  { "CLIP_L_GEN_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 0, 3}}, false, false }, // GN1640 Addr 0 Col 0 Row 3,
  { "CLIP_R_GEN_LT"      , DEVICE_GN1640T , {.gn1640Info = {0, 1, 3}}, false, false }, // GN1640 Addr 0 Col 1 Row 3,
  { "CLIP_SPARE_CTN1_LT" , DEVICE_GN1640T , {.gn1640Info = {0, 2, 1}}, false, false }, // GN1640 Addr 0 Col 2 Row 1,
  { "CLIP_SPARE_CTN2_LT" , DEVICE_GN1640T , {.gn1640Info = {0, 2, 2}}, false, false }, // GN1640 Addr 0 Col 2 Row 2,
  { "CLIP_SPARE_CTN3_LT" , DEVICE_GN1640T , {.gn1640Info = {0, 2, 3}}, false, false }, // GN1640 Addr 0 Col 2 Row 3,
  { "CHART_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "CONSOLES_DIMMER"    , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "FLOOD_DIMMER"       , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "INST_PNL_DIMMER"    , DEVICE_GPIO    , {.gpioInfo = {6}}, true, false }, // GPIO 6,
  { "WARN_CAUTION_DIMMER", DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "LH_ADV_ASPJ_OH"     , DEVICE_TM1637  , {.tm1637Info = {37, 39, 5, 1}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 5 Bit 1,
  { "LH_ADV_GO"          , DEVICE_TM1637  , {.tm1637Info = {37, 39, 0, 0}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 0 Bit 0,
  { "LH_ADV_L_BAR_GREEN" , DEVICE_TM1637  , {.tm1637Info = {37, 39, 4, 0}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 4 Bit 0,
  { "LH_ADV_L_BAR_RED"   , DEVICE_TM1637  , {.tm1637Info = {37, 39, 3, 0}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 3 Bit 0,
  { "LH_ADV_L_BLEED"     , DEVICE_TM1637  , {.tm1637Info = {37, 39, 1, 0}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 1 Bit 0,
  { "LH_ADV_NO_GO"       , DEVICE_TM1637  , {.tm1637Info = {37, 39, 0, 1}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 0 Bit 1,
  { "LH_ADV_REC"         , DEVICE_TM1637  , {.tm1637Info = {37, 39, 3, 1}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 3 Bit 1,
  { "LH_ADV_R_BLEED"     , DEVICE_TM1637  , {.tm1637Info = {37, 39, 1, 1}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 1 Bit 1,
  { "LH_ADV_SPD_BRK"     , DEVICE_TM1637  , {.tm1637Info = {37, 39, 2, 0}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 2 Bit 0,
  { "LH_ADV_STBY"        , DEVICE_TM1637  , {.tm1637Info = {37, 39, 2, 1}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 2 Bit 1,
  { "LH_ADV_XMIT"        , DEVICE_TM1637  , {.tm1637Info = {37, 39, 4, 1}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 4 Bit 1,
  { "FIRE_LEFT_LT"       , DEVICE_TM1637  , {.tm1637Info = {37, 39, 0, 2}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 0 Bit 2,
  { "LS_LOCK"            , DEVICE_WS2812  , {.ws2812Info = {0}}, false, false }, // WS2812 Index 0,
  { "LS_SHOOT"           , DEVICE_WS2812  , {.ws2812Info = {1}}, false, false }, // WS2812 Index 1,
  { "LS_SHOOT_STROBE"    , DEVICE_WS2812  , {.ws2812Info = {2}}, false, false }, // WS2812 Index 2,
  { "HMD_OFF_BRT"        , DEVICE_NONE    , {.gpioInfo = {0}}, false, false }, // No Info,
  { "SPIN_LT"            , DEVICE_GPIO    , {.gpioInfo = {34}}, false, false }, // GPIO 34,
  { "MASTER_MODE_AA_LT"  , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 3}}, false, true }, // PCA 0x5B Port 1 Bit 3,
  { "MASTER_MODE_AG_LT"  , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 4}}, false, true }, // PCA 0x5B Port 1 Bit 4,
  { "MC_DISCH"           , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 6}}, false, true }, // PCA 0x5B Port 1 Bit 6,
  { "MC_READY"           , DEVICE_PCA9555 , {.pcaInfo = {0x5B, 1, 5}}, false, true }, // PCA 0x5B Port 1 Bit 5,
  { "MASTER_CAUTION_LT"  , DEVICE_TM1637  , {.tm1637Info = {37, 39, 5, 0}}, false, false }, // TM1637 CLK 37 DIO 39 Seg 5 Bit 0,
  { "RH_ADV_AAA"         , DEVICE_TM1637  , {.tm1637Info = {37, 40, 4, 0}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 4 Bit 0,
  { "RH_ADV_AI"          , DEVICE_TM1637  , {.tm1637Info = {37, 40, 4, 1}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 4 Bit 1,
  { "RH_ADV_CW"          , DEVICE_TM1637  , {.tm1637Info = {37, 40, 5, 1}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 5 Bit 1,
  { "RH_ADV_DISP"        , DEVICE_TM1637  , {.tm1637Info = {37, 40, 0, 0}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 0 Bit 0,
  { "RH_ADV_RCDR_ON"     , DEVICE_TM1637  , {.tm1637Info = {37, 40, 0, 1}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 0 Bit 1,
  { "RH_ADV_SAM"         , DEVICE_TM1637  , {.tm1637Info = {37, 40, 3, 0}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 3 Bit 0,
  { "RH_ADV_SPARE_RH1"   , DEVICE_TM1637  , {.tm1637Info = {37, 40, 1, 1}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 1 Bit 1,
  { "RH_ADV_SPARE_RH2"   , DEVICE_TM1637  , {.tm1637Info = {37, 40, 2, 1}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 2 Bit 1,
  { "RH_ADV_SPARE_RH3"   , DEVICE_TM1637  , {.tm1637Info = {37, 40, 3, 1}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 3 Bit 1,
  { "RH_ADV_SPARE_RH4"   , DEVICE_TM1637  , {.tm1637Info = {37, 40, 1, 0}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 1 Bit 0,
  { "RH_ADV_SPARE_RH5"   , DEVICE_TM1637  , {.tm1637Info = {37, 40, 2, 0}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 2 Bit 0,
  { "FIRE_RIGHT_LT"      , DEVICE_TM1637  , {.tm1637Info = {37, 40, 0, 2}}, false, false }, // TM1637 CLK 37 DIO 40 Seg 0 Bit 2
};

static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);

// Auto-generated hash table
struct LEDHashEntry { const char* label; const LEDMapping* led; };
static const LEDHashEntry ledHashTable[107] = {
  {"CLIP_SPARE_CTN2_LT", &panelLEDs[11]},
  {"WARN_CAUTION_DIMMER", &panelLEDs[17]},
  {"LS_SHOOT", &panelLEDs[31]},
  {"RH_ADV_CW", &panelLEDs[42]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_R_GEN_LT", &panelLEDs[9]},
  {"LH_ADV_REC", &panelLEDs[24]},
  {"CONSOLES_DIMMER", &panelLEDs[14]},
  {"SPIN_LT", &panelLEDs[34]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN1_LT", &panelLEDs[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LS_LOCK", &panelLEDs[30]},
  {"LH_ADV_L_BAR_GREEN", &panelLEDs[20]},
  {nullptr, nullptr},
  {"LH_ADV_SPD_BRK", &panelLEDs[26]},
  {"RH_ADV_DISP", &panelLEDs[43]},
  {"LH_ADV_STBY", &panelLEDs[27]},
  {nullptr, nullptr},
  {"MASTER_MODE_AA_LT", &panelLEDs[35]},
  {"LS_SHOOT_STROBE", &panelLEDs[32]},
  {"RH_ADV_AI", &panelLEDs[41]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RH_ADV_SPARE_RH1", &panelLEDs[46]},
  {"RH_ADV_SPARE_RH2", &panelLEDs[47]},
  {"RH_ADV_SPARE_RH3", &panelLEDs[48]},
  {"RH_ADV_SPARE_RH4", &panelLEDs[49]},
  {"RH_ADV_SPARE_RH5", &panelLEDs[50]},
  {"FIRE_RIGHT_LT", &panelLEDs[51]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_XMIT", &panelLEDs[28]},
  {"MASTER_MODE_AG_LT", &panelLEDs[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_GO", &panelLEDs[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_LEFT_LT", &panelLEDs[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_FCS_HOT_LT", &panelLEDs[5]},
  {"INST_PNL_DIMMER", &panelLEDs[16]},
  {"CLIP_FCES_LT", &panelLEDs[4]},
  {"CLIP_FUEL_LO_LT", &panelLEDs[6]},
  {"LH_ADV_R_BLEED", &panelLEDs[25]},
  {"HMD_OFF_BRT", &panelLEDs[33]},
  {"MASTER_CAUTION_LT", &panelLEDs[39]},
  {"RH_ADV_AAA", &panelLEDs[40]},
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
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &panelLEDs[15]},
  {"RH_ADV_RCDR_ON", &panelLEDs[44]},
  {nullptr, nullptr},
  {"CLIP_L_GEN_LT", &panelLEDs[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_L_BAR_RED", &panelLEDs[21]},
  {"CLIP_GEN_TIE_LT", &panelLEDs[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_BATT_SW_LT", &panelLEDs[2]},
  {nullptr, nullptr},
  {"CLIP_SPARE_CTN3_LT", &panelLEDs[12]},
  {"CHART_DIMMER", &panelLEDs[13]},
  {"RH_ADV_SAM", &panelLEDs[45]},
  {nullptr, nullptr},
  {"LH_ADV_L_BLEED", &panelLEDs[22]},
  {"MC_DISCH", &panelLEDs[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CLIP_APU_ACC_LT", &panelLEDs[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LH_ADV_NO_GO", &panelLEDs[23]},
  {"MC_READY", &panelLEDs[38]},
  {"LH_ADV_ASPJ_OH", &panelLEDs[18]},
  {"FIRE_APU_LT", &panelLEDs[0]},
  {"CLIP_CK_SEAT_LT", &panelLEDs[3]},
};

// Reuse shared recursive hash implementation
constexpr uint16_t ledHash(const char* s) { return labelHash(s); }

// findLED lookup
inline const LEDMapping* findLED(const char* label) {
  uint16_t startH = ledHash(label) % 107;
  for (uint16_t i = 0; i < 107; ++i) {
    uint16_t idx = (startH + i >= 107) ? (startH + i - 107) : (startH + i);
    const auto& entry = ledHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.led;
  }
  return nullptr;
}
