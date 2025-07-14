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
    { "IFEI"                       , "PCA_0x00" ,  0 ,  0 ,  -1 , "IFEI"                  , 0xFFFF , "analog"   ,  0 },
    { "MODE_SELECTOR_SW_MAN"       , "PCA_0x00" ,  0 ,  0 ,  -1 , "MODE_SELECTOR_SW"      ,     2 , "selector" ,  1 },
    { "MODE_SELECTOR_SW_OFF"       , "PCA_0x00" ,  0 ,  0 ,  -1 , "MODE_SELECTOR_SW"      ,     1 , "selector" ,  1 },
    { "MODE_SELECTOR_SW_AUTO"      , "PCA_0x00" ,  0 ,  0 ,  -1 , "MODE_SELECTOR_SW"      ,     0 , "selector" ,  1 },
    { "SELECT_HMD_LDDI_RDDI_HMD"   , "PCA_0x00" ,  0 ,  0 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     2 , "selector" ,  2 },
    { "SELECT_HMD_LDDI_RDDI_LDDI"  , "PCA_0x00" ,  0 ,  0 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     1 , "selector" ,  2 },
    { "SELECT_HMD_LDDI_RDDI_RDDI"  , "PCA_0x00" ,  0 ,  0 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     0 , "selector" ,  2 },
    { "SELECT_HUD_LDDI_RDDI_HUD"   , "PCA_0x00" ,  0 ,  0 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     2 , "selector" ,  3 },
    { "SELECT_HUD_LDDI_RDDI_LDIR"  , "PCA_0x00" ,  0 ,  0 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     1 , "selector" ,  3 },
    { "SELECT_HUD_LDDI_RDDI_RDDI"  , "PCA_0x00" ,  0 ,  0 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     0 , "selector" ,  3 },
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

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "COCKKPIT_LIGHT_MODE_SW",
    "LIGHTS_TEST_SW",
    "MODE_SELECTOR_SW",
    "SELECT_HMD_LDDI_RDDI",
    "SELECT_HUD_LDDI_RDDI",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {"SELECT_HUD_LDDI_RDDI_LDIR", &InputMappings[8]},
  {"SELECT_HMD_LDDI_RDDI_HMD", &InputMappings[4]},
  {nullptr, nullptr},
  {"SELECT_HUD_LDDI_RDDI_RDDI", &InputMappings[9]},
  {"IFEI", &InputMappings[0]},
  {"FLOOD_DIMMER", &InputMappings[21]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[23]},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_MAN", &InputMappings[1]},
  {"WARN_CAUTION_DIMMER", &InputMappings[25]},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_OFF", &InputMappings[2]},
  {"IFEI_QTY_BTN", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[19]},
  {"SELECT_HMD_LDDI_RDDI_LDDI", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DWN_BTN", &InputMappings[10]},
  {"CONSOLES_DIMMER", &InputMappings[20]},
  {"INST_PNL_DIMMER", &InputMappings[22]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_AUTO", &InputMappings[3]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[17]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[18]},
  {nullptr, nullptr},
  {"IFEI_MODE_BTN", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SELECT_HMD_LDDI_RDDI_RDDI", &InputMappings[6]},
  {nullptr, nullptr},
  {"SELECT_HUD_LDDI_RDDI_HUD", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_UP_BTN", &InputMappings[14]},
  {"CHART_DIMMER", &InputMappings[16]},
  {"IFEI_ZONE_BTN", &InputMappings[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_ET_BTN", &InputMappings[11]},
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
