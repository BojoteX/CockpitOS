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
    { "AUX_REL_SW_ENABLE"          , "PCA_0x22" ,  0 , -1 ,  11 , "AUX_REL_SW"            ,     1 , "selector"     ,  1 },
    { "AUX_REL_SW_NORM"            , "PCA_0x22" ,  0 ,  1 ,  10 , "AUX_REL_SW"            ,     0 , "selector"     ,  1 },
    { "CMSD_DISPENSE_SW_BYPASS"    , "PCA_0x22" ,  0 ,  2 ,   9 , "CMSD_DISPENSE_SW"      ,     2 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_ON"        , "PCA_0x22" ,  0 , -1 ,   8 , "CMSD_DISPENSE_SW"      ,     1 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_OFF"       , "PCA_0x22" ,  0 ,  3 ,   7 , "CMSD_DISPENSE_SW"      ,     0 , "selector"     ,  2 },
    { "CMSD_JET_SEL_BTN"           , "PCA_0x22" ,  0 ,  0 ,   6 , "CMSD_JET_SEL_BTN"      ,     1 , "momentary"    ,  0 },
    { "ECM_MODE_SW_XMIT"           , "PCA_0x22" ,  1 ,  0 ,   5 , "ECM_MODE_SW"           ,     4 , "selector"     ,  3 },
    { "ECM_MODE_SW_REC"            , "PCA_0x22" ,  0 ,  7 ,   4 , "ECM_MODE_SW"           ,     3 , "selector"     ,  3 },
    { "ECM_MODE_SW_BIT"            , "PCA_0x22" ,  0 ,  6 ,   3 , "ECM_MODE_SW"           ,     2 , "selector"     ,  3 },
    { "ECM_MODE_SW_STBY"           , "PCA_0x22" ,  0 ,  5 ,   2 , "ECM_MODE_SW"           ,     1 , "selector"     ,  3 },
    { "ECM_MODE_SW_OFF"            , "PCA_0x22" ,  0 ,  4 ,   1 , "ECM_MODE_SW"           ,     0 , "selector"     ,  3 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  4 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  5 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  5 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 },
    { "RWR_AUDIO_CTRL"             , "GPIO" , PIN(2) ,  0 ,  -1 , "RWR_AUDIO_CTRL"        , 65535 , "analog"       ,  0 },
    { "RWR_AUDIO_CTRL_DEC"         , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"        ,     0 , "variable_step",  0 },
    { "RWR_AUDIO_CTRL_INC"         , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"        ,     1 , "variable_step",  0 },
    { "RWR_BIT_BTN"                , "HC165" ,  0 ,  0 ,  -1 , "RWR_BIT_BTN"           ,     1 , "momentary"    ,  0 },
    { "RWR_DISPLAY_BTN"            , "HC165" ,  0 ,  3 ,  -1 , "RWR_DISPLAY_BTN"       ,     1 , "momentary"    ,  0 },
    { "RWR_DIS_TYPE_SW_N"          , "MATRIX" , ALR67_STROBE_4 ,  8 ,  -1 , "RWR_DIS_TYPE_SW"       ,     0 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_I"          , "MATRIX" , ALR67_STROBE_3 ,  4 ,  -1 , "RWR_DIS_TYPE_SW"       ,     1 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_A"          , "MATRIX" , ALR67_STROBE_2 ,  2 ,  -1 , "RWR_DIS_TYPE_SW"       ,     2 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_U"          , "MATRIX" , ALR67_STROBE_1 ,  1 ,  -1 , "RWR_DIS_TYPE_SW"       ,     3 , "selector"     ,  6 },
    { "RWR_DIS_TYPE_SW_F"          , "MATRIX" , ALR67_DataPin , 15 ,  -1 , "RWR_DIS_TYPE_SW"       ,     4 , "selector"     ,  6 },
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
static const InputHashEntry inputHashTable[101] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_AUDIO_CTRL_DEC", &InputMappings[32]},
  {nullptr, nullptr},
  {"RWR_RWR_INTESITY", &InputMappings[46]},
  {"CMSD_DISPENSE_SW_BYPASS", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[12]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_XMIT", &InputMappings[6]},
  {"INST_PNL_DIMMER_INC", &InputMappings[25]},
  {"RWR_AUDIO_CTRL", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_DISPLAY_BTN", &InputMappings[35]},
  {"ECM_MODE_SW_BIT", &InputMappings[8]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[27]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_REC", &InputMappings[7]},
  {"ECM_MODE_SW_OFF", &InputMappings[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_DEC", &InputMappings[24]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_STBY", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_RWR_INTESITY_INC", &InputMappings[48]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_BTN", &InputMappings[5]},
  {"RWR_SPECIAL_BTN", &InputMappings[49]},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_OFF", &InputMappings[4]},
  {"AUX_REL_SW_ENABLE", &InputMappings[0]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[15]},
  {"RWR_DIS_TYPE_SW_A", &InputMappings[38]},
  {"RWR_RWR_INTESITY_DEC", &InputMappings[47]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_DIS_TYPE_SW_F", &InputMappings[40]},
  {"CHART_DIMMER", &InputMappings[11]},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[26]},
  {"AUX_REL_SW_NORM", &InputMappings[1]},
  {"RWR_DIS_TYPE_SW_I", &InputMappings[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_DIS_TYPE_SW_N", &InputMappings[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &InputMappings[28]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[30]},
  {"RWR_DMR_CTRL_INC", &InputMappings[43]},
  {"FLOOD_DIMMER_INC", &InputMappings[22]},
  {"RWR_DIS_TYPE_SW_U", &InputMappings[39]},
  {"CONSOLES_DIMMER_INC", &InputMappings[19]},
  {"RWR_POWER_BTN", &InputMappings[45]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_OFFSET_BTN", &InputMappings[44]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[29]},
  {"RWR_DMR_CTRL_DEC", &InputMappings[42]},
  {"INST_PNL_DIMMER", &InputMappings[23]},
  {"FLOOD_DIMMER_DEC", &InputMappings[21]},
  {"CONSOLES_DIMMER", &InputMappings[17]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[18]},
  {"RWR_BIT_BTN", &InputMappings[34]},
  {nullptr, nullptr},
  {"RWR_AUDIO_CTRL_INC", &InputMappings[33]},
  {"RWR_DMR_CTRL", &InputMappings[41]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[14]},
  {"CHART_DIMMER_INC", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_ON", &InputMappings[3]},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 101;
  for (uint16_t i = 0; i < 101; ++i) {
    uint16_t idx = (startH + i >= 101) ? (startH + i - 101) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
