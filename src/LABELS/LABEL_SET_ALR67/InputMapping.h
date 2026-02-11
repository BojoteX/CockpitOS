// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE
// You can use a PIN(X) macro where X is an S2 PIN to AUTO-CONVERT to its equivalent position in an S3 device.
// So, PIN(4) will always be PIN 4 if you compile with an S2 but will get automatically converted to 5 if you compile the firmware on an S3.
// This is to easily use S2 or S3 devices on same backplane/hardware physically connected to specific PINs
#pragma once

struct InputMapping {
    const char* label;        // Unique selector label, auto-generated.
    const char* source;      // Hardware source identifier. (e.g PCA_0x26, HC165, GPIO, NONE etc)
    int8_t     port;           // Port index (For PCA use port 0 or 1, HC165 does not use port. For GPIO use PIN and use -1 when sharing GPIOs to differentiate HIGH/LOW)
    int8_t     bit;            // Bit position is used for PCA & HC165. GPIO also uses it but ONLY for one-hot selectors (GPIO assigned for each position) in such cases set as -1
    int8_t      hidId;        // HID usage ID
    const char* oride_label;  // Override command label (dcsCommand)
    uint16_t    oride_value;  // Override command value (value)
    const char* controlType;  // Control type, e.g., "selector"
    uint16_t    group;        // Group ID for exclusive selectors
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group
static const InputMapping InputMappings[] = {
    { "AUX_REL_SW_NORM"            , "PCA_0x22" ,  0 ,  1 ,  10 , "AUX_REL_SW"            ,     0 , "selector"     ,  1 },
    { "AUX_REL_SW_ENABLE"          , "PCA_0x22" ,  0 , -1 ,  11 , "AUX_REL_SW"            ,     1 , "selector"     ,  1 },
    { "CMSD_DISPENSE_SW_OFF"       , "PCA_0x22" ,  0 ,  3 ,   7 , "CMSD_DISPENSE_SW"      ,     0 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_ON"        , "PCA_0x22" ,  0 , -1 ,   8 , "CMSD_DISPENSE_SW"      ,     1 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_BYPASS"    , "PCA_0x22" ,  0 ,  2 ,   9 , "CMSD_DISPENSE_SW"      ,     2 , "selector"     ,  2 },
    { "CMSD_JET_SEL_BTN"           , "PCA_0x22" ,  0 ,  0 ,   6 , "CMSD_JET_SEL_BTN"      ,     1 , "momentary"    ,  0 },
    { "ECM_MODE_SW_OFF"            , "PCA_0x22" ,  0 ,  4 ,   1 , "ECM_MODE_SW"           ,     0 , "selector"     ,  3 },
    { "ECM_MODE_SW_STBY"           , "PCA_0x22" ,  0 ,  5 ,   2 , "ECM_MODE_SW"           ,     1 , "selector"     ,  3 },
    { "ECM_MODE_SW_BIT"            , "PCA_0x22" ,  0 ,  6 ,   3 , "ECM_MODE_SW"           ,     2 , "selector"     ,  3 },
    { "ECM_MODE_SW_REC"            , "PCA_0x22" ,  0 ,  7 ,   4 , "ECM_MODE_SW"           ,     3 , "selector"     ,  3 },
    { "ECM_MODE_SW_XMIT"           , "PCA_0x22" ,  1 ,  0 ,   5 , "ECM_MODE_SW"           ,     4 , "selector"     ,  3 },
    { "ECM_MODE_SW_DEC"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     0 , "fixed_step"   ,  0 },
    { "ECM_MODE_SW_INC"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     1 , "fixed_step"   ,  0 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  4 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  5 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  5 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 },
    { "RWR_AUDIO_CTRL"             , "GPIO" , PIN(2) ,  0 ,  -1 , "RWR_AUDIO_CTRL"        , 65535 , "analog"       ,  0 },
    { "RWR_AUDIO_CTRL_DEC"         , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"        ,     0 , "variable_step",  0 },
    { "RWR_AUDIO_CTRL_INC"         , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"        ,     1 , "variable_step",  0 },
    { "RWR_BIT_BTN"                , "HC165" ,  0 ,  0 ,  -1 , "RWR_BIT_BTN"           ,     1 , "momentary"    ,  0 },
    { "RWR_DISPLAY_BTN"            , "HC165" ,  0 ,  3 ,  -1 , "RWR_DISPLAY_BTN"       ,     1 , "momentary"    ,  0 },
    { "RWR_DIS_TYPE_SW_F"          , "MATRIX" , ALR67_DataPin , 15 ,  -1 , "RWR_DIS_TYPE_SW"       ,     4 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_U"          , "MATRIX" , ALR67_STROBE_1 ,  1 ,  -1 , "RWR_DIS_TYPE_SW"       ,     3 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_A"          , "MATRIX" , ALR67_STROBE_2 ,  2 ,  -1 , "RWR_DIS_TYPE_SW"       ,     2 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_I"          , "MATRIX" , ALR67_STROBE_3 ,  4 ,  -1 , "RWR_DIS_TYPE_SW"       ,     1 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_N"          , "MATRIX" , ALR67_STROBE_4 ,  8 ,  -1 , "RWR_DIS_TYPE_SW"       ,     0 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     0 , "fixed_step"   ,  0 },
    { "RWR_DIS_TYPE_SW_INC"        , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"       ,     1 , "fixed_step"   ,  0 },
    { "RWR_DMR_CTRL"               , "GPIO" , PIN(1) ,  0 ,  -1 , "RWR_DMR_CTRL"          , 65535 , "analog"       ,  0 },
    { "RWR_DMR_CTRL_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "RWR_DMR_CTRL"          ,     0 , "variable_step",  0 },
    { "RWR_DMR_CTRL_INC"           , "NONE" ,  0 ,  0 ,  -1 , "RWR_DMR_CTRL"          ,     1 , "variable_step",  0 },
    { "RWR_OFFSET_BTN"             , "HC165" ,  0 ,  1 ,  -1 , "RWR_OFFSET_BTN"        ,     1 , "momentary"    ,  0 },
    { "RWR_POWER_BTN"              , "HC165" ,  0 ,  4 ,  -1 , "RWR_POWER_BTN"         ,     1 , "momentary"    ,  0 },
    { "RWR_RWR_INTESITY"           , "NONE" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"      , 65535 , "analog"       ,  0 },
    { "RWR_RWR_INTESITY_DEC"       , "NONE" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"      ,     0 , "variable_step",  0 },
    { "RWR_RWR_INTESITY_INC"       , "NONE" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"      ,     1 , "variable_step",  0 },
    { "RWR_SPECIAL_BTN"            , "HC165" ,  0 ,  2 ,  -1 , "RWR_SPECIAL_BTN"       ,     1 , "momentary"    ,  0 },
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
static const InputHashEntry inputHashTable[109] = {
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[18]},
  {"CONSOLES_DIMMER", &InputMappings[19]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[16]},
  {"RWR_AUDIO_CTRL_DEC", &InputMappings[34]},
  {"RWR_DIS_TYPE_SW_DEC", &InputMappings[43]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_DEC", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_AUDIO_CTRL_INC", &InputMappings[35]},
  {"ECM_MODE_SW_REC", &InputMappings[9]},
  {"RWR_BIT_BTN", &InputMappings[36]},
  {"RWR_POWER_BTN", &InputMappings[49]},
  {"RWR_SPECIAL_BTN", &InputMappings[53]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[32]},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[31]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_STBY", &InputMappings[7]},
  {nullptr, nullptr},
  {"FLOOD_DIMMER_INC", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_INC", &InputMappings[15]},
  {"AUX_REL_SW_ENABLE", &InputMappings[1]},
  {"RWR_DIS_TYPE_SW_U", &InputMappings[39]},
  {"RWR_DMR_CTRL_INC", &InputMappings[47]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_OFFSET_BTN", &InputMappings[48]},
  {"CHART_DIMMER", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[14]},
  {"CMSD_JET_SEL_BTN", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_AUDIO_CTRL", &InputMappings[33]},
  {"RWR_DIS_TYPE_SW_A", &InputMappings[40]},
  {nullptr, nullptr},
  {"RWR_DIS_TYPE_SW_F", &InputMappings[38]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_DEC", &InputMappings[11]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_INC", &InputMappings[27]},
  {"FLOOD_DIMMER", &InputMappings[22]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[28]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[17]},
  {"FLOOD_DIMMER_DEC", &InputMappings[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_DMR_CTRL", &InputMappings[45]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_DMR_CTRL_DEC", &InputMappings[46]},
  {"AUX_REL_SW_NORM", &InputMappings[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_BYPASS", &InputMappings[4]},
  {"ECM_MODE_SW_INC", &InputMappings[12]},
  {nullptr, nullptr},
  {"RWR_DIS_TYPE_SW_INC", &InputMappings[44]},
  {"ECM_MODE_SW_BIT", &InputMappings[8]},
  {"ECM_MODE_SW_XMIT", &InputMappings[10]},
  {"INST_PNL_DIMMER", &InputMappings[25]},
  {"CMSD_DISPENSE_SW_OFF", &InputMappings[2]},
  {"RWR_DISPLAY_BTN", &InputMappings[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_DEC", &InputMappings[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_RWR_INTESITY", &InputMappings[50]},
  {"RWR_RWR_INTESITY_INC", &InputMappings[52]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_ON", &InputMappings[3]},
  {"WARN_CAUTION_DIMMER", &InputMappings[30]},
  {"RWR_DIS_TYPE_SW_I", &InputMappings[41]},
  {"RWR_RWR_INTESITY_DEC", &InputMappings[51]},
  {"RWR_DIS_TYPE_SW_N", &InputMappings[42]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_OFF", &InputMappings[6]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 109;
  for (uint16_t i = 0; i < 109; ++i) {
    uint16_t idx = (startH + i >= 109) ? (startH + i - 109) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
