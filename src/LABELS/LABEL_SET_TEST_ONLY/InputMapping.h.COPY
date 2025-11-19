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
    { "HOOK_LEVER_POS0"            , "NONE" ,  0 ,  0 ,  -1 , "HOOK_LEVER"             ,     0 , "selector"     ,  1 },
    { "HOOK_LEVER_POS1"            , "NONE" ,  0 ,  0 ,  -1 , "HOOK_LEVER"             ,     1 , "selector"     ,  1 },
    { "AUX_REL_SW_NORM"            , "NONE" ,  0 ,  0 ,  -1 , "AUX_REL_SW"             ,     0 , "selector"     ,  2 },
    { "AUX_REL_SW_ENABLE"          , "NONE" ,  0 ,  0 ,  -1 , "AUX_REL_SW"             ,     1 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_OFF"       , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"       ,     0 , "selector"     ,  3 },
    { "CMSD_DISPENSE_SW_ON"        , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"       ,     1 , "selector"     ,  3 },
    { "CMSD_DISPENSE_SW_BYPASS"    , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"       ,     2 , "selector"     ,  3 },
    { "CMSD_JET_SEL_BTN"           , "GPIO" , 14 ,  0 ,   1 , "CMSD_JET_SEL_BTN"       ,     1 , "momentary"    ,  0 },
    { "ECM_MODE_SW_OFF"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"            ,     0 , "selector"     ,  4 },
    { "ECM_MODE_SW_STBY"           , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"            ,     1 , "selector"     ,  4 },
    { "ECM_MODE_SW_BIT"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"            ,     2 , "selector"     ,  4 },
    { "ECM_MODE_SW_REC"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"            ,     3 , "selector"     ,  4 },
    { "ECM_MODE_SW_XMIT"           , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"            ,     4 , "selector"     ,  4 },
    { "ECM_MODE_SW_DEC"            , "GPIO" , 13 ,  0 ,   2 , "ECM_MODE_SW"            ,     0 , "fixed_step"   ,  0 },
    { "ECM_MODE_SW_INC"            , "GPIO" , 23 ,  0 ,   3 , "ECM_MODE_SW"            ,     1 , "fixed_step"   ,  0 },
    { "CHART_DIMMER"               , "GPIO" ,  1 ,  0 ,  -1 , "CHART_DIMMER"           , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     0 , "selector"     ,  5 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     1 , "selector"     ,  5 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     2 , "selector"     ,  5 },
    { "CONSOLES_DIMMER"            , "GPIO" ,  3 ,  0 ,  -1 , "CONSOLES_DIMMER"        , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     0 , "selector"     ,  6 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     1 , "selector"     ,  6 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    ,     1 , "variable_step",  0 },
    { "CB_FCS_CHAN1"               , "GPIO" , 25 ,  0 ,   4 , "CB_FCS_CHAN1"           ,     1 , "momentary"    ,  0 },
    { "CB_FCS_CHAN2"               , "GPIO" , 22 ,  0 ,   5 , "CB_FCS_CHAN2"           ,     1 , "momentary"    ,  0 },
    { "CB_LAUNCH_BAR"              , "GPIO" , 12 ,  0 ,   6 , "CB_LAUNCH_BAR"          ,     1 , "momentary"    ,  0 },
    { "CB_SPD_BRK"                 , "GPIO" , 11 ,  0 ,   7 , "CB_SPD_BRK"             ,     1 , "momentary"    ,  0 },
    { "MASTER_CAUTION_RESET_SW"    , "GPIO" , 10 ,  0 ,   8 , "MASTER_CAUTION_RESET_SW",     1 , "momentary"    ,  0 },
    { "SJ_CTR"                     , "GPIO" ,  0 ,  0 ,   9 , "SJ_CTR"                 ,     1 , "momentary"    ,  0 },
    { "SJ_LI"                      , "GPIO" ,  8 ,  0 ,  10 , "SJ_LI"                  ,     1 , "momentary"    ,  0 },
    { "SJ_LO"                      , "NONE" ,  0 ,  0 ,  -1 , "SJ_LO"                  ,     1 , "momentary"    ,  0 },
    { "SJ_RI"                      , "NONE" ,  0 ,  0 ,  -1 , "SJ_RI"                  ,     1 , "momentary"    ,  0 },
    { "SJ_RO"                      , "NONE" ,  0 ,  0 ,  -1 , "SJ_RO"                  ,     1 , "momentary"    ,  0 },
    { "WING_FOLD_PULL_POS0"        , "NONE" ,  0 ,  0 ,  -1 , "WING_FOLD_PULL"         ,     0 , "selector"     ,  7 },
    { "WING_FOLD_PULL_POS1"        , "NONE" ,  0 ,  0 ,  -1 , "WING_FOLD_PULL"         ,     1 , "selector"     ,  7 },
    { "WING_FOLD_ROTATE_UNFOLD"    , "NONE" ,  0 ,  0 ,  -1 , "WING_FOLD_ROTATE"       ,     0 , "selector"     ,  8 },
    { "WING_FOLD_ROTATE_HOLD"      , "NONE" ,  0 ,  0 ,  -1 , "WING_FOLD_ROTATE"       ,     1 , "selector"     ,  8 },
    { "WING_FOLD_ROTATE_FOLD"      , "NONE" ,  0 ,  0 ,  -1 , "WING_FOLD_ROTATE"       ,     2 , "selector"     ,  8 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "AUX_REL_SW",
    "CMSD_DISPENSE_SW",
    "COCKKPIT_LIGHT_MODE_SW",
    "ECM_MODE_SW",
    "HOOK_LEVER",
    "LIGHTS_TEST_SW",
    "WING_FOLD_PULL",
    "WING_FOLD_ROTATE",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[101] = {
  {"WING_FOLD_ROTATE_UNFOLD", &InputMappings[47]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_BYPASS", &InputMappings[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[16]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_XMIT", &InputMappings[12]},
  {"ECM_MODE_SW_INC", &InputMappings[14]},
  {"INST_PNL_DIMMER_INC", &InputMappings[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_BIT", &InputMappings[10]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_REC", &InputMappings[11]},
  {"ECM_MODE_SW_OFF", &InputMappings[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_DEC", &InputMappings[13]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[28]},
  {"ECM_MODE_SW_STBY", &InputMappings[9]},
  {"CB_FCS_CHAN1", &InputMappings[35]},
  {"CB_FCS_CHAN2", &InputMappings[36]},
  {"WING_FOLD_ROTATE_FOLD", &InputMappings[49]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_BTN", &InputMappings[7]},
  {nullptr, nullptr},
  {"MASTER_CAUTION_RESET_SW", &InputMappings[39]},
  {"CMSD_DISPENSE_SW_OFF", &InputMappings[4]},
  {"AUX_REL_SW_ENABLE", &InputMappings[3]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[18]},
  {"CB_LAUNCH_BAR", &InputMappings[37]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SJ_RI", &InputMappings[43]},
  {nullptr, nullptr},
  {"CHART_DIMMER", &InputMappings[15]},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[31]},
  {"AUX_REL_SW_NORM", &InputMappings[2]},
  {"SJ_LI", &InputMappings[41]},
  {"SJ_RO", &InputMappings[44]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SJ_LO", &InputMappings[42]},
  {nullptr, nullptr},
  {"HOOK_LEVER_POS0", &InputMappings[0]},
  {"HOOK_LEVER_POS1", &InputMappings[1]},
  {"WARN_CAUTION_DIMMER", &InputMappings[32]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[34]},
  {"FLOOD_DIMMER_INC", &InputMappings[26]},
  {"CB_SPD_BRK", &InputMappings[38]},
  {"CONSOLES_DIMMER_INC", &InputMappings[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WING_FOLD_PULL_POS0", &InputMappings[45]},
  {"WING_FOLD_PULL_POS1", &InputMappings[46]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[33]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[27]},
  {"FLOOD_DIMMER_DEC", &InputMappings[25]},
  {"CONSOLES_DIMMER", &InputMappings[21]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[22]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WING_FOLD_ROTATE_HOLD", &InputMappings[48]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[20]},
  {"CHART_DIMMER_INC", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_ON", &InputMappings[5]},
  {"SJ_CTR", &InputMappings[40]},
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
