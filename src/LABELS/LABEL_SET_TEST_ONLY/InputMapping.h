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
    { "HOOK_LEVER_POS0"            , "GPIO" , -1 ,  0 ,  -1 , "HOOK_LEVER"            ,     0 , "selector"     , 15 },
    { "HOOK_LEVER_POS1"            , "GPIO" , 14 ,  0 ,  -1 , "HOOK_LEVER"            ,     1 , "selector"     , 15 },
    { "AUX_REL_SW_NORM"            , "NONE" ,  0 ,  0 ,  -1 , "AUX_REL_SW"            ,     0 , "selector"     ,  8 },
    { "AUX_REL_SW_ENABLE"          , "NONE" ,  0 ,  0 ,  -1 , "AUX_REL_SW"            ,     1 , "selector"     ,  8 },
    { "CMSD_DISPENSE_SW_OFF"       , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"      ,     0 , "selector"     ,  9 },
    { "CMSD_DISPENSE_SW_ON"        , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"      ,     1 , "selector"     ,  9 },
    { "CMSD_DISPENSE_SW_BYPASS"    , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"      ,     2 , "selector"     ,  9 },
    { "CMSD_JET_SEL_BTN"           , "NONE" ,  0 ,  0 ,  -1 , "CMSD_JET_SEL_BTN"      ,     1 , "momentary"    ,  0 },
    { "ECM_MODE_SW_OFF"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     0 , "selector"     , 10 },
    { "ECM_MODE_SW_STBY"           , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     1 , "selector"     , 10 },
    { "ECM_MODE_SW_BIT"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     2 , "selector"     , 10 },
    { "ECM_MODE_SW_REC"            , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     3 , "selector"     , 10 },
    { "ECM_MODE_SW_XMIT"           , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"           ,     4 , "selector"     , 10 },
    { "FIRE_TEST_SW_POS0"          , "NONE" ,  0 ,  0 ,  -1 , "FIRE_TEST_SW"          ,     0 , "selector"     , 11 },
    { "FIRE_TEST_SW_POS1"          , "NONE" ,  0 ,  0 ,  -1 , "FIRE_TEST_SW"          ,     1 , "selector"     , 11 },
    { "FIRE_TEST_SW_POS2"          , "NONE" ,  0 ,  0 ,  -1 , "FIRE_TEST_SW"          ,     2 , "selector"     , 11 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     , 16 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     , 16 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     , 16 },
    { "CONSOLES_DIMMER"            , "GPIO" ,  1 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "GPIO" ,  5 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     , 17 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     , 17 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 },
    { "HMD_OFF_BRT"                , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"           , 65535 , "analog"       ,  0 },
    { "HMD_OFF_BRT_DEC"            , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"           ,     0 , "variable_step",  0 },
    { "HMD_OFF_BRT_INC"            , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"           ,     1 , "variable_step",  0 },
    { "IR_COOL_SW_OFF"             , "NONE" ,  0 ,  0 ,  -1 , "IR_COOL_SW"            ,     0 , "selector"     , 12 },
    { "IR_COOL_SW_NORM"            , "NONE" ,  0 ,  0 ,  -1 , "IR_COOL_SW"            ,     1 , "selector"     , 12 },
    { "IR_COOL_SW_ORIDE"           , "NONE" ,  0 ,  0 ,  -1 , "IR_COOL_SW"            ,     2 , "selector"     , 12 },
    { "SPIN_RECOVERY_COVER"        , "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_COVER"   ,     1 , "momentary"    ,  0 },
    { "SPIN_RECOVERY_SW_NORM"      , "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_SW"      ,     0 , "selector"     , 13 },
    { "SPIN_RECOVERY_SW_RCVY"      , "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_SW"      ,     1 , "selector"     , 13 },
    { "MASTER_ARM_SW_SAFE"         , "NONE" ,  0 ,  0 ,  -1 , "MASTER_ARM_SW"         ,     0 , "selector"     , 14 },
    { "MASTER_ARM_SW_ARM"          , "NONE" ,  0 ,  0 ,  -1 , "MASTER_ARM_SW"         ,     1 , "selector"     , 14 },
    { "MASTER_MODE_AA"             , "NONE" ,  0 ,  0 ,  -1 , "MASTER_MODE_AA"        ,     1 , "momentary"    ,  0 },
    { "MASTER_MODE_AG"             , "NONE" ,  0 ,  0 ,  -1 , "MASTER_MODE_AG"        ,     1 , "momentary"    ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "AUX_REL_SW",
    "CMSD_DISPENSE_SW",
    "COCKKPIT_LIGHT_MODE_SW",
    "ECM_MODE_SW",
    "FIRE_TEST_SW",
    "HOOK_LEVER",
    "IR_COOL_SW",
    "LIGHTS_TEST_SW",
    "MASTER_ARM_SW",
    "SPIN_RECOVERY_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[101] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_BYPASS", &InputMappings[6]},
  {nullptr, nullptr},
  {"IR_COOL_SW_ORIDE", &InputMappings[41]},
  {"CHART_DIMMER_DEC", &InputMappings[17]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_XMIT", &InputMappings[12]},
  {"INST_PNL_DIMMER_INC", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SPIN_RECOVERY_COVER", &InputMappings[42]},
  {nullptr, nullptr},
  {"ECM_MODE_SW_BIT", &InputMappings[10]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_REC", &InputMappings[11]},
  {"ECM_MODE_SW_OFF", &InputMappings[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_DEC", &InputMappings[29]},
  {"IR_COOL_SW_NORM", &InputMappings[40]},
  {"ECM_MODE_SW_STBY", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_BTN", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_OFF", &InputMappings[4]},
  {"AUX_REL_SW_ENABLE", &InputMappings[3]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[19]},
  {"SPIN_RECOVERY_SW_NORM", &InputMappings[43]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER", &InputMappings[16]},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[32]},
  {"AUX_REL_SW_NORM", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_ARM_SW_ARM", &InputMappings[46]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HOOK_LEVER_POS0", &InputMappings[0]},
  {"HOOK_LEVER_POS1", &InputMappings[1]},
  {"WARN_CAUTION_DIMMER", &InputMappings[33]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[35]},
  {"FLOOD_DIMMER_INC", &InputMappings[27]},
  {"MASTER_ARM_SW_SAFE", &InputMappings[45]},
  {"CONSOLES_DIMMER_INC", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HMD_OFF_BRT_INC", &InputMappings[38]},
  {"FIRE_TEST_SW_POS0", &InputMappings[13]},
  {"FIRE_TEST_SW_POS1", &InputMappings[14]},
  {"FIRE_TEST_SW_POS2", &InputMappings[15]},
  {"SPIN_RECOVERY_SW_RCVY", &InputMappings[44]},
  {"HMD_OFF_BRT", &InputMappings[36]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[34]},
  {"MASTER_MODE_AA", &InputMappings[47]},
  {"INST_PNL_DIMMER", &InputMappings[28]},
  {"FLOOD_DIMMER_DEC", &InputMappings[26]},
  {"CONSOLES_DIMMER", &InputMappings[22]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[23]},
  {nullptr, nullptr},
  {"MASTER_MODE_AG", &InputMappings[48]},
  {nullptr, nullptr},
  {"HMD_OFF_BRT_DEC", &InputMappings[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[21]},
  {"CHART_DIMMER_INC", &InputMappings[18]},
  {"IR_COOL_SW_OFF", &InputMappings[39]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_ON", &InputMappings[5]},
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
