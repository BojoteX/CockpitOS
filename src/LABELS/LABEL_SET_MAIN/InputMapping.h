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
    { "APU_FIRE_BTN"               , "TM1637" , PIN(40) ,  0 ,  -1 , "APU_FIRE_BTN"           ,     1 , "momentary"    ,  0 },
    { "EMER_JETT_BTN"              , "PCA_0x5B" ,  0 ,  4 ,  -1 , "EMER_JETT_BTN"          ,     1 , "momentary"    ,  0 },
    { "FIRE_EXT_BTN"               , "PCA_0x5B" ,  0 ,  2 ,  16 , "FIRE_EXT_BTN"           ,     1 , "momentary"    ,  0 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     0 , "selector"     ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     1 , "selector"     ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     2 , "selector"     ,  4 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     0 , "selector"     ,  5 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     1 , "selector"     ,  5 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    ,     1 , "variable_step",  0 },
    { "LEFT_FIRE_BTN"              , "TM1637" , PIN(39) ,  0 ,  -1 , "LEFT_FIRE_BTN"          ,     1 , "momentary"    ,  0 },
    { "LEFT_FIRE_BTN_COVER"        , "NONE" ,  0 ,  0 ,  -1 , "LEFT_FIRE_BTN_COVER"    ,     1 , "momentary"    ,  0 },
    { "HMD_OFF_BRT"                , "GPIO" , PIN(18) ,  0 ,  -1 , "HMD_OFF_BRT"            , 65535 , "analog"       ,  0 },
    { "HMD_OFF_BRT_DEC"            , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"            ,     0 , "variable_step",  0 },
    { "HMD_OFF_BRT_INC"            , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"            ,     1 , "variable_step",  0 },
    { "IR_COOL_SW_OFF"             , "PCA_0x26" ,  1 ,  1 ,  23 , "IR_COOL_SW"             ,     0 , "selector"     ,  6 },
    { "IR_COOL_SW_NORM"            , "PCA_0x26" ,  1 , -1 ,  24 , "IR_COOL_SW"             ,     1 , "selector"     ,  6 },
    { "IR_COOL_SW_ORIDE"           , "PCA_0x26" ,  1 ,  0 ,  25 , "IR_COOL_SW"             ,     2 , "selector"     ,  6 },
    { "SPIN_RECOVERY_COVER"        , "PCA_0x26" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_COVER"    ,     1 , "momentary"    ,  0 },
    { "SPIN_RECOVERY_SW_NORM"      , "PCA_0x26" ,  1 , -1 ,  22 , "SPIN_RECOVERY_SW"       ,     0 , "selector"     ,  7 },
    { "SPIN_RECOVERY_SW_RCVY"      , "PCA_0x26" ,  1 ,  2 ,  21 , "SPIN_RECOVERY_SW"       ,     1 , "selector"     ,  7 },
    { "MASTER_ARM_SW_SAFE"         , "PCA_0x5B" ,  0 ,  3 ,  18 , "MASTER_ARM_SW"          ,     0 , "selector"     ,  8 },
    { "MASTER_ARM_SW_ARM"          , "PCA_0x5B" ,  0 , -1 ,  17 , "MASTER_ARM_SW"          ,     1 , "selector"     ,  8 },
    { "MASTER_MODE_AA"             , "PCA_0x5B" ,  0 ,  1 ,  19 , "MASTER_MODE_AA"         ,     1 , "momentary"    ,  0 },
    { "MASTER_MODE_AG"             , "PCA_0x5B" ,  0 ,  0 ,  20 , "MASTER_MODE_AG"         ,     1 , "momentary"    ,  0 },
    { "MASTER_CAUTION_RESET_SW"    , "TM1637" , PIN(39) ,  9 ,  -1 , "MASTER_CAUTION_RESET_SW",     1 , "momentary"    ,  0 },
    { "RIGHT_FIRE_BTN"             , "TM1637" , PIN(40) ,  9 ,  -1 , "RIGHT_FIRE_BTN"         ,     1 , "momentary"    ,  0 },
    { "RIGHT_FIRE_BTN_COVER"       , "NONE" ,  0 ,  0 ,  -1 , "RIGHT_FIRE_BTN_COVER"   ,     1 , "momentary"    ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "COCKKPIT_LIGHT_MODE_SW",
    "IR_COOL_SW",
    "LIGHTS_TEST_SW",
    "MASTER_ARM_SW",
    "SPIN_RECOVERY_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[83] = {
  {"INST_PNL_DIMMER_INC", &InputMappings[17]},
  {nullptr, nullptr},
  {"RIGHT_FIRE_BTN_COVER", &InputMappings[40]},
  {"SPIN_RECOVERY_SW_RCVY", &InputMappings[33]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[10]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[18]},
  {"IR_COOL_SW_ORIDE", &InputMappings[30]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[8]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[6]},
  {"HMD_OFF_BRT_INC", &InputMappings[27]},
  {"IR_COOL_SW_NORM", &InputMappings[29]},
  {"HMD_OFF_BRT_DEC", &InputMappings[26]},
  {"SPIN_RECOVERY_SW_NORM", &InputMappings[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_ARM_SW_SAFE", &InputMappings[34]},
  {nullptr, nullptr},
  {"MASTER_CAUTION_RESET_SW", &InputMappings[38]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[19]},
  {"APU_FIRE_BTN", &InputMappings[0]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[21]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[22]},
  {"LEFT_FIRE_BTN_COVER", &InputMappings[24]},
  {"INST_PNL_DIMMER", &InputMappings[15]},
  {"MASTER_ARM_SW_ARM", &InputMappings[35]},
  {"RIGHT_FIRE_BTN", &InputMappings[39]},
  {"WARN_CAUTION_DIMMER", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AA", &InputMappings[36]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[11]},
  {"SPIN_RECOVERY_COVER", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_EXT_BTN", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_INC", &InputMappings[5]},
  {"FLOOD_DIMMER_DEC", &InputMappings[13]},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER_INC", &InputMappings[14]},
  {"LEFT_FIRE_BTN", &InputMappings[23]},
  {"HMD_OFF_BRT", &InputMappings[25]},
  {"MASTER_MODE_AG", &InputMappings[37]},
  {nullptr, nullptr},
  {"EMER_JETT_BTN", &InputMappings[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IR_COOL_SW_OFF", &InputMappings[28]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[4]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[9]},
  {"CHART_DIMMER", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
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
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
