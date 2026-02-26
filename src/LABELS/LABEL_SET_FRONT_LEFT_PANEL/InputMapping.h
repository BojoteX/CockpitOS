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
    uint16_t    releaseValue; // DCS-BIOS value sent on momentary release (0 = default)
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group  rel
static const InputMapping InputMappings[] = {
    { "FORMATION_DIMMER"           , "GPIO" , PIN(1) ,  0 ,  -1 , "FORMATION_DIMMER"      , 65535 , "analog"       ,  0 ,  0 },
    { "FORMATION_DIMMER_DEC"       , "NONE" ,  0 ,  0 ,  -1 , "FORMATION_DIMMER"      ,     0 , "variable_step",  0 ,  0 },
    { "FORMATION_DIMMER_INC"       , "NONE" ,  0 ,  0 ,  -1 , "FORMATION_DIMMER"      ,     1 , "variable_step",  0 ,  0 },
    { "INT_WNG_TANK_SW_NORM"       , "GPIO" , -1 ,  0 ,  -1 , "INT_WNG_TANK_SW"       ,     0 , "selector"     ,  1 ,  0 },
    { "INT_WNG_TANK_SW_INHIBIT"    , "GPIO" , PIN(34) ,  0 ,  -1 , "INT_WNG_TANK_SW"       ,     1 , "selector"     ,  1 ,  0 },
    { "POSITION_DIMMER"            , "GPIO" , PIN(2) ,  0 ,  -1 , "POSITION_DIMMER"       , 65535 , "analog"       ,  0 ,  0 },
    { "POSITION_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "POSITION_DIMMER"       ,     0 , "variable_step",  0 ,  0 },
    { "POSITION_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "POSITION_DIMMER"       ,     1 , "variable_step",  0 ,  0 },
    { "STROBE_SW_POS0"             , "GPIO" , PIN(5) ,  0 ,  -1 , "STROBE_SW"             ,     0 , "selector"     ,  2 ,  0 },
    { "STROBE_SW_POS1"             , "GPIO" , -1 ,  0 ,  -1 , "STROBE_SW"             ,     1 , "selector"     ,  2 ,  0 },
    { "STROBE_SW_POS2"             , "GPIO" , PIN(4) ,  0 ,  -1 , "STROBE_SW"             ,     2 , "selector"     ,  2 ,  0 },
    { "EXT_PWR_SW_OFF"             , "GPIO" , PIN(18) ,  0 ,  -1 , "EXT_PWR_SW"            ,     0 , "selector"     ,  5 ,  0 },
    { "EXT_PWR_SW_NORM"            , "GPIO" , -1 ,  0 ,  -1 , "EXT_PWR_SW"            ,     1 , "selector"     ,  5 ,  0 },
    { "EXT_PWR_SW_RESET"           , "GPIO" , PIN(17) ,  0 ,  -1 , "EXT_PWR_SW"            ,     2 , "selector"     ,  5 ,  0 },
    { "GND_PWR_1_SW_B_ON"          , "GPIO" , PIN(10) ,  0 ,  -1 , "GND_PWR_1_SW"          ,     0 , "selector"     ,  6 ,  0 },
    { "GND_PWR_1_SW_AUTO"          , "GPIO" , -1 ,  0 ,  -1 , "GND_PWR_1_SW"          ,     1 , "selector"     ,  6 ,  0 },
    { "GND_PWR_1_SW_A_ON"          , "GPIO" , PIN(9) ,  0 ,  -1 , "GND_PWR_1_SW"          ,     2 , "selector"     ,  6 ,  0 },
    { "GND_PWR_2_SW_B_ON"          , "GPIO" , PIN(13) ,  0 ,  -1 , "GND_PWR_2_SW"          ,     0 , "selector"     ,  7 ,  0 },
    { "GND_PWR_2_SW_AUTO"          , "GPIO" , -1 ,  0 ,  -1 , "GND_PWR_2_SW"          ,     1 , "selector"     ,  7 ,  0 },
    { "GND_PWR_2_SW_A_ON"          , "GPIO" , PIN(11) ,  0 ,  -1 , "GND_PWR_2_SW"          ,     2 , "selector"     ,  7 ,  0 },
    { "GND_PWR_3_SW_B_ON"          , "GPIO" , PIN(16) ,  0 ,  -1 , "GND_PWR_3_SW"          ,     0 , "selector"     ,  8 ,  0 },
    { "GND_PWR_3_SW_AUTO"          , "GPIO" , -1 ,  0 ,  -1 , "GND_PWR_3_SW"          ,     1 , "selector"     ,  8 ,  0 },
    { "GND_PWR_3_SW_A_ON"          , "GPIO" , PIN(14) ,  0 ,  -1 , "GND_PWR_3_SW"          ,     2 , "selector"     ,  8 ,  0 },
    { "GND_PWR_4_SW_B_ON"          , "GPIO" , PIN(8) ,  0 ,  -1 , "GND_PWR_4_SW"          ,     0 , "selector"     ,  9 ,  0 },
    { "GND_PWR_4_SW_AUTO"          , "GPIO" , -1 ,  0 ,  -1 , "GND_PWR_4_SW"          ,     1 , "selector"     ,  9 ,  0 },
    { "GND_PWR_4_SW_A_ON"          , "GPIO" , PIN(7) ,  0 ,  -1 , "GND_PWR_4_SW"          ,     2 , "selector"     ,  9 ,  0 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 ,  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     , 10 ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     , 10 ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     , 10 ,  0 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 ,  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 ,  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 ,  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 ,  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 ,  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 ,  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     , 11 ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     , 11 ,  0 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 ,  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "COCKKPIT_LIGHT_MODE_SW",
    "EXT_PWR_SW",
    "GND_PWR_1_SW",
    "GND_PWR_2_SW",
    "GND_PWR_3_SW",
    "GND_PWR_4_SW",
    "INT_WNG_TANK_SW",
    "LIGHTS_TEST_SW",
    "STROBE_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[97] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[41]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[45]},
  {"POSITION_DIMMER_INC", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"POSITION_DIMMER_DEC", &InputMappings[6]},
  {"GND_PWR_2_SW_A_ON", &InputMappings[19]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[30]},
  {"INST_PNL_DIMMER_INC", &InputMappings[40]},
  {nullptr, nullptr},
  {"GND_PWR_1_SW_A_ON", &InputMappings[16]},
  {nullptr, nullptr},
  {"STROBE_SW_POS2", &InputMappings[10]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[39]},
  {nullptr, nullptr},
  {"STROBE_SW_POS0", &InputMappings[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_4_SW_AUTO", &InputMappings[24]},
  {"POSITION_DIMMER", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FORMATION_DIMMER_INC", &InputMappings[2]},
  {"GND_PWR_3_SW_AUTO", &InputMappings[21]},
  {"INT_WNG_TANK_SW_INHIBIT", &InputMappings[4]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[38]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_4_SW_A_ON", &InputMappings[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[31]},
  {"GND_PWR_3_SW_A_ON", &InputMappings[22]},
  {"WARN_CAUTION_DIMMER", &InputMappings[43]},
  {"EXT_PWR_SW_RESET", &InputMappings[13]},
  {"CHART_DIMMER_INC", &InputMappings[28]},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[27]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[29]},
  {nullptr, nullptr},
  {"FLOOD_DIMMER_DEC", &InputMappings[36]},
  {"FORMATION_DIMMER_DEC", &InputMappings[1]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[42]},
  {"GND_PWR_3_SW_B_ON", &InputMappings[20]},
  {nullptr, nullptr},
  {"CHART_DIMMER", &InputMappings[26]},
  {"EXT_PWR_SW_OFF", &InputMappings[11]},
  {"GND_PWR_2_SW_B_ON", &InputMappings[17]},
  {"FLOOD_DIMMER", &InputMappings[35]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[44]},
  {nullptr, nullptr},
  {"GND_PWR_1_SW_B_ON", &InputMappings[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"STROBE_SW_POS1", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[34]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_PWR_SW_NORM", &InputMappings[12]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_DEC", &InputMappings[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INT_WNG_TANK_SW_NORM", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FORMATION_DIMMER", &InputMappings[0]},
  {"FLOOD_DIMMER_INC", &InputMappings[37]},
  {"GND_PWR_4_SW_B_ON", &InputMappings[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_2_SW_AUTO", &InputMappings[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_1_SW_AUTO", &InputMappings[15]},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 97;
  for (uint16_t i = 0; i < 97; ++i) {
    uint16_t idx = (startH + i >= 97) ? (startH + i - 97) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
