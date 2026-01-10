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
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  5 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  5 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  5 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  6 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  6 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 },
    { "KY58_FILL_SELECT_Z_1-5"     , "HC165" ,  0 ,  7 ,   1 , "KY58_FILL_SELECT"      ,     0 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_1"         , "HC165" ,  0 ,  0 ,   2 , "KY58_FILL_SELECT"      ,     1 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_2"         , "HC165" ,  0 ,  1 ,   3 , "KY58_FILL_SELECT"      ,     2 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_3"         , "HC165" ,  0 ,  2 ,   4 , "KY58_FILL_SELECT"      ,     3 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_4"         , "HC165" ,  0 ,  3 ,   5 , "KY58_FILL_SELECT"      ,     4 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_5"         , "HC165" ,  0 ,  4 ,   6 , "KY58_FILL_SELECT"      ,     5 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_6"         , "HC165" ,  0 ,  5 ,   7 , "KY58_FILL_SELECT"      ,     6 , "selector"     ,  1 },
    { "KY58_FILL_SELECT_Z_ALL"     , "HC165" ,  0 ,  6 ,   8 , "KY58_FILL_SELECT"      ,     7 , "selector"     ,  1 },
    { "KY58_FILL_SEL_PULL_POS0"    , "NONE" ,  0 ,  0 ,  -1 , "KY58_FILL_SEL_PULL"    ,     0 , "selector"     ,  2 },
    { "KY58_FILL_SEL_PULL_POS1"    , "NONE" ,  0 ,  0 ,  -1 , "KY58_FILL_SEL_PULL"    ,     1 , "selector"     ,  2 },
    { "KY58_MODE_SELECT_P"         , "HC165" ,  0 ,  8 ,   9 , "KY58_MODE_SELECT"      ,     0 , "selector"     ,  3 },
    { "KY58_MODE_SELECT_C"         , "HC165" ,  0 ,  9 ,  10 , "KY58_MODE_SELECT"      ,     1 , "selector"     ,  3 },
    { "KY58_MODE_SELECT_LD"        , "HC165" ,  0 , 10 ,  11 , "KY58_MODE_SELECT"      ,     2 , "selector"     ,  3 },
    { "KY58_MODE_SELECT_RV"        , "HC165" ,  0 , 11 ,  12 , "KY58_MODE_SELECT"      ,     3 , "selector"     ,  3 },
    { "KY58_POWER_SELECT_OFF"      , "HC165" ,  0 , 12 ,  13 , "KY58_POWER_SELECT"     ,     0 , "selector"     ,  4 },
    { "KY58_POWER_SELECT_ON"       , "HC165" ,  0 , 13 ,  14 , "KY58_POWER_SELECT"     ,     1 , "selector"     ,  4 },
    { "KY58_POWER_SELECT_TD"       , "HC165" ,  0 , 14 ,  15 , "KY58_POWER_SELECT"     ,     2 , "selector"     ,  4 },
    { "KY58_VOLUME"                , "GPIO" , PIN(11) ,  0 ,  -1 , "KY58_VOLUME"           , 65535 , "analog"       ,  0 },
    { "KY58_VOLUME_DEC"            , "NONE" ,  0 ,  0 ,  -1 , "KY58_VOLUME"           ,     0 , "variable_step",  0 },
    { "KY58_VOLUME_INC"            , "NONE" ,  0 ,  0 ,  -1 , "KY58_VOLUME"           ,     1 , "variable_step",  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "COCKKPIT_LIGHT_MODE_SW",
    "KY58_FILL_SELECT",
    "KY58_FILL_SEL_PULL",
    "KY58_MODE_SELECT",
    "KY58_POWER_SELECT",
    "LIGHTS_TEST_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[83] = {
  {"FLOOD_DIMMER_INC", &InputMappings[11]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"KY58_MODE_SELECT_RV", &InputMappings[33]},
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
  {"KY58_POWER_SELECT_OFF", &InputMappings[34]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"KY58_MODE_SELECT_C", &InputMappings[31]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[4]},
  {"CHART_DIMMER_DEC", &InputMappings[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[3]},
  {"KY58_FILL_SELECT_1", &InputMappings[21]},
  {"KY58_FILL_SELECT_2", &InputMappings[22]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[7]},
  {"KY58_FILL_SELECT_3", &InputMappings[23]},
  {"KY58_FILL_SELECT_4", &InputMappings[24]},
  {"KY58_FILL_SELECT_5", &InputMappings[25]},
  {"KY58_FILL_SELECT_6", &InputMappings[26]},
  {"KY58_FILL_SELECT_Z_ALL", &InputMappings[27]},
  {"KY58_MODE_SELECT_P", &InputMappings[30]},
  {"CHART_DIMMER", &InputMappings[0]},
  {"CHART_DIMMER_INC", &InputMappings[2]},
  {"KY58_MODE_SELECT_LD", &InputMappings[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[8]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[9]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[16]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[13]},
  {"KY58_VOLUME_DEC", &InputMappings[38]},
  {"WARN_CAUTION_DIMMER", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER_DEC", &InputMappings[10]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[19]},
  {nullptr, nullptr},
  {"KY58_POWER_SELECT_TD", &InputMappings[36]},
  {"KY58_VOLUME", &InputMappings[37]},
  {"KY58_FILL_SEL_PULL_POS0", &InputMappings[28]},
  {"INST_PNL_DIMMER_INC", &InputMappings[14]},
  {"KY58_FILL_SEL_PULL_POS1", &InputMappings[29]},
  {"INST_PNL_DIMMER", &InputMappings[12]},
  {"KY58_FILL_SELECT_Z_1-5", &InputMappings[20]},
  {"KY58_VOLUME_INC", &InputMappings[39]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"KY58_POWER_SELECT_ON", &InputMappings[35]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 83;
  for (uint16_t i = 0; i < 83; ++i) {
    uint16_t idx = (startH + i >= 83) ? (startH + i - 83) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
