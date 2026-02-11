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
    { "FIRE_TEST_SW_POS0"            , "PCA_0x26" ,  0 ,  5 ,  -1 , "FIRE_TEST_SW"           ,     0 , "selector"     ,  1 },
    { "FIRE_TEST_SW_POS1"            , "PCA_0x26" ,  0 , -1 ,  -1 , "FIRE_TEST_SW"           ,     1 , "selector"     ,  1 },
    { "FIRE_TEST_SW_POS2"            , "PCA_0x26" ,  0 ,  6 ,  -1 , "FIRE_TEST_SW"           ,     2 , "selector"     ,  1 },
    { "GEN_TIE_COVER"                , "NONE" ,  0 ,  0 ,  -1 , "GEN_TIE_COVER"          ,     1 , "momentary"    ,  0 },
    { "GEN_TIE_SW_RESET"             , "PCA_0x26" ,  1 ,  3 ,  -1 , "GEN_TIE_SW"             ,     1 , "selector"     ,  2 },
    { "GEN_TIE_SW_NORM"              , "PCA_0x26" ,  1 , -1 ,  -1 , "GEN_TIE_SW"             ,     0 , "selector"     ,  2 },
    { "CHART_DIMMER"                 , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"             , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"             , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY"   , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     2 , "selector"     , 11 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE"  , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     1 , "selector"     , 11 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG"   , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     0 , "selector"     , 11 },
    { "CONSOLES_DIMMER"              , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"          , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"          , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"                 , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"             , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"             , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"              , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"          , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"          , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_OFF"           , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     1 , "selector"     , 12 },
    { "LIGHTS_TEST_SW_TEST"          , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     0 , "selector"     , 12 },
    { "WARN_CAUTION_DIMMER"          , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"      , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"      , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    ,     1 , "variable_step",  0 },
    { "HYD_ISOLATE_OVERRIDE_SW_ORIDE", "PCA_0x26" ,  1 , -1 ,  -1 , "HYD_ISOLATE_OVERRIDE_SW",     1 , "selector"     ,  3 },
    { "HYD_ISOLATE_OVERRIDE_SW_NORM" , "PCA_0x26" ,  1 ,  2 ,  -1 , "HYD_ISOLATE_OVERRIDE_SW",     0 , "selector"     ,  3 },
    { "MC_SW_2_OFF"                  , "PCA_0x26" ,  1 ,  0 ,  -1 , "MC_SW"                  ,     2 , "selector"     ,  4 },
    { "MC_SW_NORM"                   , "PCA_0x26" ,  1 , -1 ,  -1 , "MC_SW"                  ,     1 , "selector"     ,  4 },
    { "MC_SW_1_OFF"                  , "PCA_0x26" ,  1 ,  1 ,  -1 , "MC_SW"                  ,     0 , "selector"     ,  4 },
    { "ANTI_SKID_SW_PRESS"           , "NONE" ,  0 ,  0 ,  -1 , "ANTI_SKID_SW"           ,     0 , "selector"     ,  5 },
    { "ANTI_SKID_SW_RELEASE"         , "NONE" ,  0 ,  0 ,  -1 , "ANTI_SKID_SW"           ,     1 , "selector"     ,  5 },
    { "FLAP_SW_FULL"                 , "NONE" ,  0 ,  0 ,  -1 , "FLAP_SW"                ,     2 , "selector"     ,  6 },
    { "FLAP_SW_HALF"                 , "NONE" ,  0 ,  0 ,  -1 , "FLAP_SW"                ,     1 , "selector"     ,  6 },
    { "FLAP_SW_AUTO"                 , "NONE" ,  0 ,  0 ,  -1 , "FLAP_SW"                ,     0 , "selector"     ,  6 },
    { "HOOK_BYPASS_SW_CARRIER"       , "NONE" ,  0 ,  0 ,  -1 , "HOOK_BYPASS_SW"         ,     1 , "selector"     ,  7 },
    { "HOOK_BYPASS_SW_FIELD"         , "NONE" ,  0 ,  0 ,  -1 , "HOOK_BYPASS_SW"         ,     0 , "selector"     ,  7 },
    { "LAUNCH_BAR_SW_PRESS"          , "NONE" ,  0 ,  0 ,  -1 , "LAUNCH_BAR_SW"          ,     0 , "selector"     ,  8 },
    { "LAUNCH_BAR_SW_RELEASE"        , "NONE" ,  0 ,  0 ,  -1 , "LAUNCH_BAR_SW"          ,     1 , "selector"     ,  8 },
    { "LDG_TAXI_SW_TAXI_LIGHT_SWITCH", "NONE" ,  0 ,  0 ,  -1 , "LDG_TAXI_SW"            ,     1 , "selector"     ,  9 },
    { "LDG_TAXI_SW_LDG"              , "NONE" ,  0 ,  0 ,  -1 , "LDG_TAXI_SW"            ,     0 , "selector"     ,  9 },
    { "SEL_JETT_BTN"                 , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_BTN"           ,     1 , "momentary"    ,  0 },
    { "SEL_JETT_KNOB_POS0"           , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     0 , "selector"     , 10 },
    { "SEL_JETT_KNOB_POS1"           , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     1 , "selector"     , 10 },
    { "SEL_JETT_KNOB_POS2"           , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     2 , "selector"     , 10 },
    { "SEL_JETT_KNOB_POS3"           , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     3 , "selector"     , 10 },
    { "SEL_JETT_KNOB_POS4"           , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     4 , "selector"     , 10 },
    { "SEL_JETT_KNOB_DEC"            , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     0 , "fixed_step"   ,  0 },
    { "SEL_JETT_KNOB_INC"            , "NONE" ,  0 ,  0 ,  -1 , "SEL_JETT_KNOB"          ,     1 , "fixed_step"   ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "ANTI_SKID_SW",
    "COCKKPIT_LIGHT_MODE_SW",
    "FIRE_TEST_SW",
    "FLAP_SW",
    "GEN_TIE_SW",
    "HOOK_BYPASS_SW",
    "HYD_ISOLATE_OVERRIDE_SW",
    "LAUNCH_BAR_SW",
    "LDG_TAXI_SW",
    "LIGHTS_TEST_SW",
    "MC_SW",
    "SEL_JETT_KNOB",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[101] = {
  {"INST_PNL_DIMMER_INC", &InputMappings[20]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB_DEC", &InputMappings[48]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB_POS3", &InputMappings[46]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[22]},
  {"ANTI_SKID_SW_PRESS", &InputMappings[31]},
  {"LDG_TAXI_SW_LDG", &InputMappings[41]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LAUNCH_BAR_SW_RELEASE", &InputMappings[39]},
  {nullptr, nullptr},
  {"GEN_TIE_SW_RESET", &InputMappings[4]},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB_INC", &InputMappings[49]},
  {nullptr, nullptr},
  {"ANTI_SKID_SW_RELEASE", &InputMappings[32]},
  {nullptr, nullptr},
  {"FIRE_TEST_SW_POS2", &InputMappings[2]},
  {"CHART_DIMMER", &InputMappings[6]},
  {"FLOOD_DIMMER", &InputMappings[15]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[21]},
  {"MC_SW_1_OFF", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HOOK_BYPASS_SW_CARRIER", &InputMappings[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB_POS4", &InputMappings[47]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[25]},
  {"SEL_JETT_KNOB_POS0", &InputMappings[43]},
  {"INST_PNL_DIMMER", &InputMappings[18]},
  {nullptr, nullptr},
  {"SEL_JETT_BTN", &InputMappings[42]},
  {"MC_SW_NORM", &InputMappings[29]},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[24]},
  {"MC_SW_2_OFF", &InputMappings[28]},
  {nullptr, nullptr},
  {"HOOK_BYPASS_SW_FIELD", &InputMappings[37]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_TEST_SW_POS1", &InputMappings[1]},
  {"FLAP_SW_HALF", &InputMappings[34]},
  {"SEL_JETT_KNOB_POS1", &InputMappings[44]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_DEC", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HYD_ISOLATE_OVERRIDE_SW_NORM", &InputMappings[27]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_TEST_SW_POS0", &InputMappings[0]},
  {nullptr, nullptr},
  {"GEN_TIE_COVER", &InputMappings[3]},
  {nullptr, nullptr},
  {"FLAP_SW_AUTO", &InputMappings[35]},
  {nullptr, nullptr},
  {"LAUNCH_BAR_SW_PRESS", &InputMappings[38]},
  {"FLAP_SW_FULL", &InputMappings[33]},
  {nullptr, nullptr},
  {"CHART_DIMMER_INC", &InputMappings[8]},
  {nullptr, nullptr},
  {"SEL_JETT_KNOB_POS2", &InputMappings[45]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[10]},
  {"HYD_ISOLATE_OVERRIDE_SW_ORIDE", &InputMappings[26]},
  {"CHART_DIMMER_DEC", &InputMappings[7]},
  {"GEN_TIE_SW_NORM", &InputMappings[5]},
  {"FLOOD_DIMMER_DEC", &InputMappings[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LDG_TAXI_SW_TAXI_LIGHT_SWITCH", &InputMappings[40]},
  {"CONSOLES_DIMMER_INC", &InputMappings[14]},
  {"FLOOD_DIMMER_INC", &InputMappings[17]},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &InputMappings[23]},
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
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
