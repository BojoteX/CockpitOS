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
    { "FORMATION_DIMMER"           , "PCA_0x00" ,  0 ,  0 ,  -1 , "FORMATION_DIMMER"      , 0xFFFF , "analog"   ,  0 },
    { "INT_WNG_TANK_SW_INHIBIT"    , "GPIO" , 34 ,  0 ,  -1 , "INT_WNG_TANK_SW"       ,     1 , "selector" ,  1 },
    { "INT_WNG_TANK_SW_NORM"       , "GPIO" , 34 ,  1 ,  -1 , "INT_WNG_TANK_SW"       ,     0 , "selector" ,  1 },
    { "POSITION_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "POSITION_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "STROBE_SW_POS0"             , "GPIO" ,  5 ,  0 ,  -1 , "STROBE_SW"             ,     0 , "selector" ,  2 },
    { "STROBE_SW_POS1"             , "GPIO" ,  4 ,  1 ,  -1 , "STROBE_SW"             ,     1 , "selector" ,  2 },
    { "STROBE_SW_POS2"             , "GPIO" ,  4 ,  0 ,  -1 , "STROBE_SW"             ,     2 , "selector" ,  2 },
    { "FIRE_TEST_SW_POS0"          , "GPIO" , 33 ,  0 ,  -1 , "FIRE_TEST_SW"          ,     0 , "selector" ,  3 },
    { "FIRE_TEST_SW_POS1"          , "GPIO" , 21 ,  1 ,  -1 , "FIRE_TEST_SW"          ,     1 , "selector" ,  3 },
    { "FIRE_TEST_SW_POS2"          , "GPIO" , 21 ,  0 ,  -1 , "FIRE_TEST_SW"          ,     2 , "selector" ,  3 },
    { "GEN_TIE_COVER"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "GEN_TIE_COVER"         ,     1 , "momentary",  0 },
    { "GEN_TIE_SW_NORM"            , "GPIO" ,  6 ,  1 ,  -1 , "GEN_TIE_SW"            ,     0 , "selector" ,  4 },
    { "GEN_TIE_SW_RESET"           , "GPIO" ,  6 ,  0 ,  -1 , "GEN_TIE_SW"            ,     1 , "selector" ,  4 },
    { "EXT_PWR_SW_RESET"           , "GPIO" , 17 ,  0 ,  -1 , "EXT_PWR_SW"            ,     2 , "selector" ,  5 },
    { "EXT_PWR_SW_NORM"            , "GPIO" , 17 ,  1 ,  -1 , "EXT_PWR_SW"            ,     1 , "selector" ,  5 },
    { "EXT_PWR_SW_OFF"             , "GPIO" , 18 ,  0 ,  -1 , "EXT_PWR_SW"            ,     0 , "selector" ,  5 },
    { "GND_PWR_1_SW_A_ON"          , "GPIO" ,  9 ,  0 ,  -1 , "GND_PWR_1_SW"          ,     2 , "selector" ,  6 },
    { "GND_PWR_1_SW_AUTO"          , "GPIO" ,  9 ,  1 ,  -1 , "GND_PWR_1_SW"          ,     1 , "selector" ,  6 },
    { "GND_PWR_1_SW_B_ON"          , "GPIO" , 10 ,  0 ,  -1 , "GND_PWR_1_SW"          ,     0 , "selector" ,  6 },
    { "GND_PWR_2_SW_A_ON"          , "GPIO" , 11 ,  0 ,  -1 , "GND_PWR_2_SW"          ,     2 , "selector" ,  7 },
    { "GND_PWR_2_SW_AUTO"          , "GPIO" , 11 ,  1 ,  -1 , "GND_PWR_2_SW"          ,     1 , "selector" ,  7 },
    { "GND_PWR_2_SW_B_ON"          , "GPIO" , 13 ,  0 ,  -1 , "GND_PWR_2_SW"          ,     0 , "selector" ,  7 },
    { "GND_PWR_3_SW_A_ON"          , "GPIO" , 14 ,  0 ,  -1 , "GND_PWR_3_SW"          ,     2 , "selector" ,  8 },
    { "GND_PWR_3_SW_AUTO"          , "GPIO" , 14 ,  1 ,  -1 , "GND_PWR_3_SW"          ,     1 , "selector" ,  8 },
    { "GND_PWR_3_SW_B_ON"          , "GPIO" , 16 ,  0 ,  -1 , "GND_PWR_3_SW"          ,     0 , "selector" ,  8 },
    { "GND_PWR_4_SW_A_ON"          , "GPIO" ,  7 ,  0 ,  -1 , "GND_PWR_4_SW"          ,     2 , "selector" ,  9 },
    { "GND_PWR_4_SW_AUTO"          , "GPIO" ,  7 ,  1 ,  -1 , "GND_PWR_4_SW"          ,     1 , "selector" ,  9 },
    { "GND_PWR_4_SW_B_ON"          , "GPIO" ,  8 ,  0 ,  -1 , "GND_PWR_4_SW"          ,     0 , "selector" ,  9 },
    { "CHART_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector" ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector" ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector" ,  0 },
    { "CONSOLES_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "FLOOD_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "INST_PNL_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector" ,  0 },
    { "LIGHTS_TEST_SW_OFF"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector" ,  0 },
    { "WARN_CAUTION_DIMMER"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 0xFFFF , "analog"   ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "EXT_PWR_SW",
    "FIRE_TEST_SW",
    "GEN_TIE_SW",
    "GND_PWR_1_SW",
    "GND_PWR_2_SW",
    "GND_PWR_3_SW",
    "GND_PWR_4_SW",
    "INT_WNG_TANK_SW",
    "STROBE_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[79] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_2_SW_A_ON", &InputMappings[19]},
  {nullptr, nullptr},
  {"INT_WNG_TANK_SW_INHIBIT", &InputMappings[1]},
  {"EXT_PWR_SW_OFF", &InputMappings[15]},
  {"GND_PWR_1_SW_AUTO", &InputMappings[17]},
  {nullptr, nullptr},
  {"FORMATION_DIMMER", &InputMappings[0]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[29]},
  {"GEN_TIE_COVER", &InputMappings[10]},
  {"FLOOD_DIMMER", &InputMappings[33]},
  {"GND_PWR_3_SW_B_ON", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"POSITION_DIMMER", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GEN_TIE_SW_NORM", &InputMappings[11]},
  {"GND_PWR_3_SW_A_ON", &InputMappings[22]},
  {"CHART_DIMMER", &InputMappings[28]},
  {"GND_PWR_2_SW_AUTO", &InputMappings[20]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[30]},
  {"WARN_CAUTION_DIMMER", &InputMappings[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_4_SW_B_ON", &InputMappings[27]},
  {"STROBE_SW_POS0", &InputMappings[4]},
  {"INT_WNG_TANK_SW_NORM", &InputMappings[2]},
  {"STROBE_SW_POS1", &InputMappings[5]},
  {"STROBE_SW_POS2", &InputMappings[6]},
  {"INST_PNL_DIMMER", &InputMappings[34]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[35]},
  {"CONSOLES_DIMMER", &InputMappings[32]},
  {"GND_PWR_4_SW_A_ON", &InputMappings[25]},
  {"GEN_TIE_SW_RESET", &InputMappings[12]},
  {"GND_PWR_3_SW_AUTO", &InputMappings[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FIRE_TEST_SW_POS0", &InputMappings[7]},
  {"FIRE_TEST_SW_POS1", &InputMappings[8]},
  {"FIRE_TEST_SW_POS2", &InputMappings[9]},
  {"EXT_PWR_SW_NORM", &InputMappings[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_1_SW_B_ON", &InputMappings[18]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[31]},
  {"GND_PWR_4_SW_AUTO", &InputMappings[26]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_1_SW_A_ON", &InputMappings[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_PWR_SW_RESET", &InputMappings[13]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GND_PWR_2_SW_B_ON", &InputMappings[21]},
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
  uint16_t startH = inputHash(label) % 79;
  for (uint16_t i = 0; i < 79; ++i) {
    uint16_t idx = (startH + i >= 79) ? (startH + i - 79) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
