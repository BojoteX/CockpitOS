// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

struct InputMapping {
    const char* label;        // Unique selector label
    const char* source;       // Hardware source identifier
    uint8_t     port;         // Port index
    uint8_t     bit;          // Bit position
    int8_t      hidId;        // HID usage ID
    const char* oride_label;  // Override command label (dcsCommand)
    uint16_t    oride_value;  // Override command value (value)
    const char* controlType;  // Control type, e.g., "selector"
    uint16_t    group;        // Group ID for exclusive selectors
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group
static const InputMapping InputMappings[] = {
    { "IFEI_DWN_BTN"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI_DWN_BTN"          ,     1 , "momentary",  0 },
    { "IFEI_ET_BTN"                , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI_ET_BTN"           ,     1 , "momentary",  0 },
    { "IFEI_MODE_BTN"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI_MODE_BTN"         ,     1 , "momentary",  0 },
    { "IFEI_QTY_BTN"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI_QTY_BTN"          ,     1 , "momentary",  0 },
    { "IFEI_UP_BTN"                , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI_UP_BTN"           ,     1 , "momentary",  0 },
    { "IFEI_ZONE_BTN"              , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI_ZONE_BTN"         ,     1 , "momentary",  0 },
    { "CHART_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector" ,  1 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector" ,  1 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "PCA_0x00" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector" ,  1 },
    { "CONSOLES_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "FLOOD_DIMMER"               , "PCA_0x00" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 0xFFFF , "analog"   ,  0 },
    { "INST_PNL_DIMMER"            , "PCA_0x00" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 0xFFFF , "analog"   ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector" ,  2 },
    { "LIGHTS_TEST_SW_OFF"         , "PCA_0x00" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector" ,  2 },
    { "WARN_CAUTION_DIMMER"        , "PCA_0x00" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 0xFFFF , "analog"   ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[11]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &InputMappings[15]},
  {nullptr, nullptr},
  {"IFEI_QTY_BTN", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DWN_BTN", &InputMappings[0]},
  {"CONSOLES_DIMMER", &InputMappings[10]},
  {"INST_PNL_DIMMER", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[8]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_MODE_BTN", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_UP_BTN", &InputMappings[4]},
  {"CHART_DIMMER", &InputMappings[6]},
  {"IFEI_ZONE_BTN", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_ET_BTN", &InputMappings[1]},
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
