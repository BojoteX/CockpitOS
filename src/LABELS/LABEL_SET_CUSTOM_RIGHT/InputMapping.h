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
    { "HOOK_LEVER_POS0"            , "PCA_0x26" ,  0 , -1 ,  -1 , "HOOK_LEVER"            ,     1 , "selector"     ,  4 },
    { "HOOK_LEVER_POS1"            , "PCA_0x26" ,  0 ,  7 ,  -1 , "HOOK_LEVER"            ,     0 , "selector"     ,  4 },
    { "AV_COOL_SW_EMERG"           , "PCA_0x26" ,  0 ,  4 ,  -1 , "AV_COOL_SW"            ,     1 , "selector"     ,  1 },
    { "AV_COOL_SW_NORM"            , "PCA_0x26" ,  0 , -1 ,  -1 , "AV_COOL_SW"            ,     0 , "selector"     ,  1 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  2 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  2 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  2 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  3 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  3 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 },
    { "RADALT_HEIGHT_POS0"         , "GPIO" , PIN(38) ,  0 ,  -1 , "RADALT_HEIGHT"         ,     0 , "variable_step",  0 },
    { "RADALT_HEIGHT_POS1"         , "GPIO" , PIN(1) ,  0 ,  -1 , "RADALT_HEIGHT"         ,     1 , "variable_step",  0 },
    { "RADALT_TEST_SW"             , "GPIO" , PIN(16) ,  0 ,  -1 , "RADALT_TEST_SW"        ,     1 , "momentary"    ,  0 },
    { "WING_FOLD_PULL_POS0"        , "NONE" ,  0 ,  1 ,   1 , "WING_FOLD_PULL"        ,     0 , "selector"     ,  5 },
    { "WING_FOLD_PULL_POS1"        , "NONE" ,  0 , -1 ,   2 , "WING_FOLD_PULL"        ,     1 , "selector"     ,  5 },
    { "WING_FOLD_ROTATE_UNFOLD"    , "NONE" ,  0 ,  2 ,   3 , "WING_FOLD_ROTATE"      ,     0 , "selector"     ,  6 },
    { "WING_FOLD_ROTATE_HOLD"      , "NONE" ,  0 , -1 ,   4 , "WING_FOLD_ROTATE"      ,     1 , "selector"     ,  6 },
    { "WING_FOLD_ROTATE_FOLD"      , "NONE" ,  0 ,  3 ,   5 , "WING_FOLD_ROTATE"      ,     2 , "selector"     ,  6 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "AV_COOL_SW",
    "COCKKPIT_LIGHT_MODE_SW",
    "HOOK_LEVER",
    "LIGHTS_TEST_SW",
    "WING_FOLD_PULL",
    "WING_FOLD_ROTATE",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[67] = {
  {"WARN_CAUTION_DIMMER", &InputMappings[21]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_DEC", &InputMappings[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HOOK_LEVER_POS0", &InputMappings[0]},
  {"RADALT_HEIGHT_POS0", &InputMappings[24]},
  {"WING_FOLD_PULL_POS0", &InputMappings[27]},
  {"CHART_DIMMER_INC", &InputMappings[6]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AV_COOL_SW_NORM", &InputMappings[3]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_DEC", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER_INC", &InputMappings[15]},
  {nullptr, nullptr},
  {"RADALT_TEST_SW", &InputMappings[26]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[5]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[20]},
  {"WING_FOLD_PULL_POS1", &InputMappings[28]},
  {"WING_FOLD_ROTATE_HOLD", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[19]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[7]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[9]},
  {"AV_COOL_SW_EMERG", &InputMappings[2]},
  {"CHART_DIMMER", &InputMappings[4]},
  {"INST_PNL_DIMMER_INC", &InputMappings[18]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[22]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[23]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[8]},
  {nullptr, nullptr},
  {"FLOOD_DIMMER_DEC", &InputMappings[14]},
  {"WING_FOLD_ROTATE_FOLD", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HOOK_LEVER_POS1", &InputMappings[1]},
  {"CONSOLES_DIMMER", &InputMappings[10]},
  {"FLOOD_DIMMER", &InputMappings[13]},
  {"RADALT_HEIGHT_POS1", &InputMappings[25]},
  {"WING_FOLD_ROTATE_UNFOLD", &InputMappings[29]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 67;
  for (uint16_t i = 0; i < 67; ++i) {
    uint16_t idx = (startH + i >= 67) ? (startH + i - 67) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
