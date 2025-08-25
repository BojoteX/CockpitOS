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
    { "CHART_DIMMER"                 , "PCA_0x00" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG"   , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector" ,  1 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE"  , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector" ,  1 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY"   , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector" ,  1 },
    { "CONSOLES_DIMMER"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "FLOOD_DIMMER"                 , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "INST_PNL_DIMMER"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "LIGHTS_TEST_SW_TEST"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector" ,  2 },
    { "LIGHTS_TEST_SW_OFF"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector" ,  2 },
    { "WARN_CAUTION_DIMMER"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 0xFFFF , "analog"   ,  0 },
    { "ANTI_SKID_SW_PRESS"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "ANTI_SKID_SW"          ,     0 , "selector" ,  4 },
    { "ANTI_SKID_SW_RELEASE"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "ANTI_SKID_SW"          ,     1 , "selector" ,  4 },
    { "FLAP_SW_AUTO"                 , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLAP_SW"               ,     2 , "selector" ,  1 },
    { "FLAP_SW_HALF"                 , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLAP_SW"               ,     1 , "selector" ,  1 },
    { "FLAP_SW_FULL"                 , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLAP_SW"               ,     0 , "selector" ,  1 },
    { "HOOK_BYPASS_SW_FIELD"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "HOOK_BYPASS_SW"        ,     1 , "action"   ,  2 },
    { "HOOK_BYPASS_SW_CARRIER"       , "PCA_0x00" ,  0 ,  0 ,  -1 , "HOOK_BYPASS_SW"        ,     0 , "action"   ,  2 },
    { "LAUNCH_BAR_SW_PRESS"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "LAUNCH_BAR_SW"         ,     0 , "action"   ,  0 },
    { "LAUNCH_BAR_SW_RELEASE"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LAUNCH_BAR_SW"         ,     1 , "action"   ,  0 },
    { "LDG_TAXI_SW_LDG"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "LDG_TAXI_SW"           ,     1 , "selector" ,  3 },
    { "LDG_TAXI_SW_TAXI_LIGHT_SWITCH", "PCA_0x00" ,  0 ,  0 ,  -1 , "LDG_TAXI_SW"           ,     0 , "selector" ,  3 },
    { "SEL_JETT_BTN"                 , "PCA_0x00" ,  0 ,  0 ,  -1 , "SEL_JETT_BTN"          ,     1 , "momentary",  0 },
    { "SEL_JETT_KNOB_POS0"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"         ,     0 , "selector" ,  5 },
    { "SEL_JETT_KNOB_POS1"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"         ,     1 , "selector" ,  5 },
    { "SEL_JETT_KNOB_POS2"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"         ,     2 , "selector" ,  5 },
    { "SEL_JETT_KNOB_POS3"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"         ,     3 , "selector" ,  5 },
    { "SEL_JETT_KNOB_POS4"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"         ,     4 , "selector" ,  5 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "ANTI_SKID_SW",
    "COCKKPIT_LIGHT_MODE_SW",
    "FLAP_SW",
    "LDG_TAXI_SW",
    "LIGHTS_TEST_SW",
    "SEL_JETT_KNOB",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[59] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[6]},
  {nullptr, nullptr},
  {"LAUNCH_BAR_SW_PRESS", &InputMappings[17]},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[8]},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB_POS0", &InputMappings[22]},
  {"SEL_JETT_KNOB_POS1", &InputMappings[23]},
  {"SEL_JETT_KNOB_POS2", &InputMappings[24]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[7]},
  {"SEL_JETT_BTN", &InputMappings[21]},
  {"SEL_JETT_KNOB_POS3", &InputMappings[25]},
  {"SEL_JETT_KNOB_POS4", &InputMappings[26]},
  {"ANTI_SKID_SW_PRESS", &InputMappings[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LDG_TAXI_SW_LDG", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLAP_SW_HALF", &InputMappings[13]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[3]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[1]},
  {"WARN_CAUTION_DIMMER", &InputMappings[9]},
  {"ANTI_SKID_SW_RELEASE", &InputMappings[11]},
  {"FLAP_SW_AUTO", &InputMappings[12]},
  {"CONSOLES_DIMMER", &InputMappings[4]},
  {"FLAP_SW_FULL", &InputMappings[14]},
  {"HOOK_BYPASS_SW_FIELD", &InputMappings[15]},
  {"HOOK_BYPASS_SW_CARRIER", &InputMappings[16]},
  {"LAUNCH_BAR_SW_RELEASE", &InputMappings[18]},
  {"LDG_TAXI_SW_TAXI_LIGHT_SWITCH", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &InputMappings[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[2]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 59;
  for (uint16_t i = 0; i < 59; ++i) {
    uint16_t idx = (startH + i >= 59) ? (startH + i - 59) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
