// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

struct InputMapping {
    const char* label;        // Unique selector label
    const char* source;       // Hardware source identifier
    uint8_t     port;         // Port index
    uint8_t     bit;          // Bit position
    int8_t      hidId;        // HID usage ID
    const char* oride_label;  // Override command label (dcsCommand)
    uint16_t    oride_value;  // Override command value (value)
    const char* controlType;  // Control type, e.g., "selector"
    uint16_t    group;        // Group ID for exclusive selectors
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group
static const InputMapping InputMappings[] = {
    { "AUX_REL_SW_ENABLE"          , "PCA_0x22" ,  0 ,  1 ,  11 , "AUX_REL_SW"            ,     1 , "selector" ,  1 },
    { "AUX_REL_SW_NORM"            , "PCA_0x22" ,  0 ,  1 ,  10 , "AUX_REL_SW"            ,     0 , "selector" ,  1 },
    { "CMSD_DISPENSE_SW_BYPASS"    , "PCA_0x22" ,  0 ,  2 ,   9 , "CMSD_DISPENSE_SW"      ,     2 , "selector" ,  2 },
    { "CMSD_DISPENSE_SW_ON"        , "PCA_0x22" ,  0 ,  2 ,   8 , "CMSD_DISPENSE_SW"      ,     1 , "selector" ,  2 },
    { "CMSD_DISPENSE_SW_OFF"       , "PCA_0x22" ,  0 ,  3 ,   7 , "CMSD_DISPENSE_SW"      ,     0 , "selector" ,  2 },
    { "CMSD_JET_SEL_BTN"           , "PCA_0x22" ,  0 ,  0 ,   1 , "CMSD_JET_SEL_BTN"      ,     1 , "momentary",  0 },
    { "ECM_MODE_SW_XMIT"           , "PCA_0x22" ,  0 ,  4 ,   6 , "ECM_MODE_SW"           ,     4 , "selector" ,  3 },
    { "ECM_MODE_SW_REC"            , "PCA_0x22" ,  0 ,  5 ,   5 , "ECM_MODE_SW"           ,     3 , "selector" ,  3 },
    { "ECM_MODE_SW_BIT"            , "PCA_0x22" ,  0 ,  6 ,   4 , "ECM_MODE_SW"           ,     2 , "selector" ,  3 },
    { "ECM_MODE_SW_STBY"           , "PCA_0x22" ,  0 ,  7 ,   3 , "ECM_MODE_SW"           ,     1 , "selector" ,  3 },
    { "ECM_MODE_SW_OFF"            , "PCA_0x22" ,  1 ,  0 ,   2 , "ECM_MODE_SW"           ,     0 , "selector" ,  3 },
    { "CHART_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector" ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector" ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector" ,  4 },
    { "CONSOLES_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "FLOOD_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "INST_PNL_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector" ,  5 },
    { "LIGHTS_TEST_SW_OFF"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector" ,  5 },
    { "WARN_CAUTION_DIMMER"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 0xFFFF , "analog"   ,  0 },
    { "RWR_AUDIO_CTRL"             , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"        , 0xFFFF , "analog"   ,  0 },
    { "RWR_BIT_BTN"                , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_BIT_BTN"           ,     1 , "momentary",  0 },
    { "RWR_DISPLAY_BTN"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DISPLAY_BTN"       ,     1 , "momentary",  0 },
    { "RWR_DIS_TYPE_SW_N"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     4 , "selector" ,  6 },
    { "RWR_DIS_TYPE_SW_I"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     3 , "selector" ,  6 },
    { "RWR_DIS_TYPE_SW_A"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     2 , "selector" ,  6 },
    { "RWR_DIS_TYPE_SW_U"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     1 , "selector" ,  6 },
    { "RWR_DIS_TYPE_SW_F"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     0 , "selector" ,  6 },
    { "RWR_DMR_CTRL"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_DMR_CTRL"          , 0xFFFF , "analog"   ,  0 },
    { "RWR_OFFSET_BTN"             , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_OFFSET_BTN"        ,     1 , "momentary",  0 },
    { "RWR_POWER_BTN"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_POWER_BTN"         ,     1 , "momentary",  0 },
    { "RWR_RWR_INTESITY"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"      , 0xFFFF , "analog"   ,  0 },
    { "RWR_SPECIAL_BTN"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "RWR_SPECIAL_BTN"       ,     1 , "momentary",  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "AUX_REL_SW",
    "CMSD_DISPENSE_SW",
    "COCKKPIT_LIGHT_MODE_SW",
    "ECM_MODE_SW",
    "LIGHTS_TEST_SW",
    "RWR_DIS_TYPE_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[71] = {
  {"CMSD_DISPENSE_SW_ON", &InputMappings[3]},
  {"CMSD_DISPENSE_SW_BYPASS", &InputMappings[2]},
  {"RWR_DIS_TYPE_SW_F", &InputMappings[28]},
  {nullptr, nullptr},
  {"RWR_DIS_TYPE_SW_I", &InputMappings[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[17]},
  {"CONSOLES_DIMMER", &InputMappings[15]},
  {"RWR_DIS_TYPE_SW_N", &InputMappings[24]},
  {"RWR_SPECIAL_BTN", &InputMappings[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_OFF", &InputMappings[10]},
  {"RWR_DIS_TYPE_SW_U", &InputMappings[27]},
  {nullptr, nullptr},
  {"AUX_REL_SW_NORM", &InputMappings[1]},
  {"CMSD_DISPENSE_SW_OFF", &InputMappings[4]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[18]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_XMIT", &InputMappings[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_BTN", &InputMappings[5]},
  {"ECM_MODE_SW_BIT", &InputMappings[8]},
  {"RWR_BIT_BTN", &InputMappings[22]},
  {nullptr, nullptr},
  {"AUX_REL_SW_ENABLE", &InputMappings[0]},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &InputMappings[20]},
  {nullptr, nullptr},
  {"CHART_DIMMER", &InputMappings[11]},
  {"RWR_RWR_INTESITY", &InputMappings[32]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[13]},
  {"RWR_AUDIO_CTRL", &InputMappings[21]},
  {"RWR_POWER_BTN", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[16]},
  {"ECM_MODE_SW_REC", &InputMappings[7]},
  {"ECM_MODE_SW_STBY", &InputMappings[9]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[14]},
  {"RWR_DISPLAY_BTN", &InputMappings[23]},
  {"RWR_OFFSET_BTN", &InputMappings[30]},
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
  {"RWR_DIS_TYPE_SW_A", &InputMappings[26]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[19]},
  {"RWR_DMR_CTRL", &InputMappings[29]},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 71;
  for (uint16_t i = 0; i < 71; ++i) {
    uint16_t idx = (startH + i >= 71) ? (startH + i - 71) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
