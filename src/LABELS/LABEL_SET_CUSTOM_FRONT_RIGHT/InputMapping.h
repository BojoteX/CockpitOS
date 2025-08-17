// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

struct InputMapping {
    const char* label;        // Unique selector label
    const char* source;       // Hardware source identifier
    int8_t     port;         // Port index
    int8_t     bit;          // Bit position
    int8_t      hidId;        // HID usage ID
    const char* oride_label;  // Override command label (dcsCommand)
    uint16_t    oride_value;  // Override command value (value)
    const char* controlType;  // Control type, e.g., "selector"
    uint16_t    group;        // Group ID for exclusive selectors
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group
static const InputMapping InputMappings[] = {
    { "HOOK_LEVER_POS0"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "HOOK_LEVER"            ,     0 , "selector" ,  5 },
    { "HOOK_LEVER_POS1"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "HOOK_LEVER"            ,     1 , "selector" ,  5 },
    { "AV_COOL_SW_NORM"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "AV_COOL_SW"            ,     1 , "selector" ,  1 },
    { "AV_COOL_SW_EMERG"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "AV_COOL_SW"            ,     0 , "selector" ,  1 },
    { "CHART_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "GPIO" , 39 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector" ,  2 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "GPIO" , 39 ,  1 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector" ,  2 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "GPIO" , 40 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector" ,  2 },
    { "CONSOLES_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "FLOOD_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "INST_PNL_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector" ,  3 },
    { "LIGHTS_TEST_SW_OFF"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector" ,  3 },
    { "WARN_CAUTION_DIMMER"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 0xFFFF , "analog"   ,  0 },
    { "RADALT_TEST_SW"             , "PCA_0x00" ,  0 ,  0 ,  -1 , "RADALT_TEST_SW"        ,     1 , "momentary",  0 },
    { "WING_FOLD_PULL_POS0"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WING_FOLD_PULL"        ,     0 , "selector" ,  6 },
    { "WING_FOLD_PULL_POS1"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WING_FOLD_PULL"        ,     1 , "selector" ,  6 },
    { "WING_FOLD_ROTATE_FOLD"      , "PCA_0x00" ,  0 ,  0 ,  -1 , "WING_FOLD_ROTATE"      ,     2 , "selector" ,  4 },
    { "WING_FOLD_ROTATE_HOLD"      , "PCA_0x00" ,  0 ,  0 ,  -1 , "WING_FOLD_ROTATE"      ,     1 , "selector" ,  4 },
    { "WING_FOLD_ROTATE_UNFOLD"    , "PCA_0x00" ,  0 ,  0 ,  -1 , "WING_FOLD_ROTATE"      ,     0 , "selector" ,  4 },
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
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {"WING_FOLD_PULL_POS0", &InputMappings[15]},
  {"AV_COOL_SW_NORM", &InputMappings[2]},
  {"FLOOD_DIMMER", &InputMappings[9]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[11]},
  {"WING_FOLD_PULL_POS1", &InputMappings[16]},
  {nullptr, nullptr},
  {"HOOK_LEVER_POS0", &InputMappings[0]},
  {"HOOK_LEVER_POS1", &InputMappings[1]},
  {"WARN_CAUTION_DIMMER", &InputMappings[13]},
  {"WING_FOLD_ROTATE_UNFOLD", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[8]},
  {"INST_PNL_DIMMER", &InputMappings[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[6]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADALT_TEST_SW", &InputMappings[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AV_COOL_SW_EMERG", &InputMappings[3]},
  {"WING_FOLD_ROTATE_FOLD", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WING_FOLD_ROTATE_HOLD", &InputMappings[18]},
  {"CHART_DIMMER", &InputMappings[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 53;
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
