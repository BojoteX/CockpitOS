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
    { "APU_FIRE_BTN"               , "PCA_0x00" ,  0 ,  0 ,  12 , "APU_FIRE_BTN"           ,     1 , "momentary",  0 },
    { "EMER_JETT_BTN"              , "PCA_0x5B" ,  0 ,  4 ,  -1 , "EMER_JETT_BTN"          ,     1 , "momentary",  0 },
    { "FIRE_EXT_BTN"               , "PCA_0x00" ,  0 ,  0 ,  16 , "FIRE_EXT_BTN"           ,     1 , "momentary",  0 },
    { "CHART_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "CHART_DIMMER"           , 0xFFFF , "analog"   ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     2 , "selector" ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     1 , "selector" ,  4 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW" ,     0 , "selector" ,  4 },
    { "CONSOLES_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"        , 0xFFFF , "analog"   ,  0 },
    { "FLOOD_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"           , 0xFFFF , "analog"   ,  0 },
    { "INST_PNL_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"        , 0xFFFF , "analog"   ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     1 , "selector" ,  5 },
    { "LIGHTS_TEST_SW_OFF"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"         ,     0 , "selector" ,  5 },
    { "WARN_CAUTION_DIMMER"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"    , 0xFFFF , "analog"   ,  0 },
    { "LEFT_FIRE_BTN"              , "PCA_0x00" ,  0 ,  0 ,  15 , "LEFT_FIRE_BTN"          ,     1 , "momentary",  0 },
    { "LEFT_FIRE_BTN_COVER"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LEFT_FIRE_BTN_COVER"    ,     1 , "momentary",  0 },
    { "HMD_OFF_BRT"                , "PCA_0x00" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"            , 0xFFFF , "analog"   ,  0 },
    { "IR_COOL_SW_ORIDE"           , "PCA_0x26" ,  0 ,  0 ,  25 , "IR_COOL_SW"             ,     2 , "selector" ,  6 },
    { "IR_COOL_SW_NORM"            , "PCA_0x26" ,  0 ,  0 ,  24 , "IR_COOL_SW"             ,     1 , "selector" ,  6 },
    { "IR_COOL_SW_OFF"             , "PCA_0x26" ,  0 ,  0 ,  23 , "IR_COOL_SW"             ,     0 , "selector" ,  6 },
    { "SPIN_RECOVERY_COVER"        , "PCA_0x26" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_COVER"    ,     1 , "momentary",  0 },
    { "SPIN_RECOVERY_SW_RCVY"      , "PCA_0x26" ,  0 ,  0 ,  21 , "SPIN_RECOVERY_SW"       ,     1 , "selector" ,  7 },
    { "SPIN_RECOVERY_SW_NORM"      , "PCA_0x26" ,  0 ,  0 ,  22 , "SPIN_RECOVERY_SW"       ,     0 , "selector" ,  7 },
    { "MASTER_ARM_SW_ARM"          , "PCA_0x5B" ,  0 ,  0 ,  17 , "MASTER_ARM_SW"          ,     1 , "selector" ,  8 },
    { "MASTER_ARM_SW_SAFE"         , "PCA_0x5B" ,  0 ,  0 ,  18 , "MASTER_ARM_SW"          ,     0 , "selector" ,  8 },
    { "MASTER_MODE_AA"             , "PCA_0x5B" ,  0 ,  0 ,  19 , "MASTER_MODE_AA"         ,     1 , "momentary",  0 },
    { "MASTER_MODE_AG"             , "PCA_0x5B" ,  0 ,  0 ,  20 , "MASTER_MODE_AG"         ,     1 , "momentary",  0 },
    { "MASTER_CAUTION_RESET_SW"    , "PCA_0x00" ,  0 ,  0 ,  14 , "MASTER_CAUTION_RESET_SW",     1 , "momentary",  0 },
    { "RIGHT_FIRE_BTN"             , "PCA_0x00" ,  0 ,  0 ,  13 , "RIGHT_FIRE_BTN"         ,     1 , "momentary",  0 },
    { "RIGHT_FIRE_BTN_COVER"       , "PCA_0x00" ,  0 ,  0 ,  -1 , "RIGHT_FIRE_BTN_COVER"   ,     1 , "momentary",  0 },
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
static const InputHashEntry inputHashTable[59] = {
  {"LEFT_FIRE_BTN", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[9]},
  {"SPIN_RECOVERY_SW_NORM", &InputMappings[21]},
  {"MASTER_CAUTION_RESET_SW", &InputMappings[26]},
  {"RIGHT_FIRE_BTN", &InputMappings[27]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[11]},
  {"RIGHT_FIRE_BTN_COVER", &InputMappings[28]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[10]},
  {"LEFT_FIRE_BTN_COVER", &InputMappings[14]},
  {nullptr, nullptr},
  {"HMD_OFF_BRT", &InputMappings[15]},
  {nullptr, nullptr},
  {"MASTER_ARM_SW_ARM", &InputMappings[22]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_ARM_SW_SAFE", &InputMappings[23]},
  {"FLOOD_DIMMER", &InputMappings[8]},
  {nullptr, nullptr},
  {"SPIN_RECOVERY_COVER", &InputMappings[19]},
  {"SPIN_RECOVERY_SW_RCVY", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IR_COOL_SW_OFF", &InputMappings[18]},
  {"IR_COOL_SW_NORM", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[6]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[4]},
  {"WARN_CAUTION_DIMMER", &InputMappings[12]},
  {nullptr, nullptr},
  {"EMER_JETT_BTN", &InputMappings[1]},
  {"CONSOLES_DIMMER", &InputMappings[7]},
  {nullptr, nullptr},
  {"IR_COOL_SW_ORIDE", &InputMappings[16]},
  {"MASTER_MODE_AA", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AG", &InputMappings[25]},
  {"FIRE_EXT_BTN", &InputMappings[2]},
  {"APU_FIRE_BTN", &InputMappings[0]},
  {"CHART_DIMMER", &InputMappings[3]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[5]},
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
