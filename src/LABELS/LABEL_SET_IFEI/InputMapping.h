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
    { "IFEI"                       , "GPIO" , PIN(3) ,  0 ,  -1 , "IFEI"                  , 65535 , "analog"       ,  0 },
    { "IFEI_DEC"                   , "NONE" ,  0 ,  0 ,  -1 , "IFEI"                  ,     0 , "variable_step",  0 },
    { "IFEI_INC"                   , "NONE" ,  0 ,  0 ,  -1 , "IFEI"                  ,     1 , "variable_step",  0 },
    { "MODE_SELECTOR_SW_MAN"       , "HC165" ,  0 ,  0 ,  -1 , "MODE_SELECTOR_SW"      ,     2 , "selector"     ,  5 },
    { "MODE_SELECTOR_SW_OFF"       , "HC165" ,  0 , -1 ,  -1 , "MODE_SELECTOR_SW"      ,     1 , "selector"     ,  5 },
    { "MODE_SELECTOR_SW_AUTO"      , "HC165" ,  0 ,  6 ,  -1 , "MODE_SELECTOR_SW"      ,     0 , "selector"     ,  5 },
    { "SELECT_HMD_LDDI_RDDI_HMD"   , "HC165" ,  0 ,  2 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     2 , "selector"     ,  4 },
    { "SELECT_HMD_LDDI_RDDI_LDDI"  , "HC165" ,  0 , -1 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     1 , "selector"     ,  4 },
    { "SELECT_HMD_LDDI_RDDI_RDDI"  , "HC165" ,  0 ,  1 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     0 , "selector"     ,  4 },
    { "SELECT_HUD_LDDI_RDDI_HUD"   , "HC165" ,  0 ,  4 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     2 , "selector"     ,  3 },
    { "SELECT_HUD_LDDI_RDDI_LDIR"  , "HC165" ,  0 , -1 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     1 , "selector"     ,  3 },
    { "SELECT_HUD_LDDI_RDDI_RDDI"  , "HC165" ,  0 ,  3 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     0 , "selector"     ,  3 },
    { "IFEI_DWN_BTN"               , "HC165" ,  0 , 11 ,  -1 , "IFEI_DWN_BTN"          ,     1 , "momentary"    ,  0 },
    { "IFEI_ET_BTN"                , "HC165" ,  0 , 13 ,  -1 , "IFEI_ET_BTN"           ,     1 , "momentary"    ,  0 },
    { "IFEI_MODE_BTN"              , "HC165" ,  0 ,  8 ,  -1 , "IFEI_MODE_BTN"         ,     1 , "momentary"    ,  0 },
    { "IFEI_QTY_BTN"               , "HC165" ,  0 ,  9 ,  -1 , "IFEI_QTY_BTN"          ,     1 , "momentary"    ,  0 },
    { "IFEI_UP_BTN"                , "HC165" ,  0 , 10 ,  -1 , "IFEI_UP_BTN"           ,     1 , "momentary"    ,  0 },
    { "IFEI_ZONE_BTN"              , "HC165" ,  0 , 12 ,  -1 , "IFEI_ZONE_BTN"         ,     1 , "momentary"    ,  0 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  1 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  1 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  1 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  2 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  2 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 },
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
static const InputHashEntry inputHashTable[79] = {
  {"FLOOD_DIMMER_INC", &InputMappings[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DWN_BTN", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[26]},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[21]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[31]},
  {"FLOOD_DIMMER", &InputMappings[27]},
  {"MODE_SELECTOR_SW_MAN", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI", &InputMappings[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &InputMappings[35]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[37]},
  {"CHART_DIMMER", &InputMappings[18]},
  {"IFEI_INC", &InputMappings[2]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[22]},
  {"FLOOD_DIMMER_DEC", &InputMappings[28]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER", &InputMappings[30]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[25]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[33]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[24]},
  {"IFEI_ZONE_BTN", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SELECT_HMD_LDDI_RDDI_LDDI", &InputMappings[7]},
  {"SELECT_HUD_LDDI_RDDI_LDIR", &InputMappings[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_INC", &InputMappings[20]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[36]},
  {"IFEI_DEC", &InputMappings[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_QTY_BTN", &InputMappings[15]},
  {"IFEI_UP_BTN", &InputMappings[16]},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_OFF", &InputMappings[4]},
  {"SELECT_HUD_LDDI_RDDI_RDDI", &InputMappings[11]},
  {"IFEI_MODE_BTN", &InputMappings[14]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[23]},
  {"IFEI_ET_BTN", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_INC", &InputMappings[32]},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_AUTO", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[34]},
  {"CHART_DIMMER_DEC", &InputMappings[19]},
  {"SELECT_HMD_LDDI_RDDI_HMD", &InputMappings[6]},
  {"SELECT_HMD_LDDI_RDDI_RDDI", &InputMappings[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SELECT_HUD_LDDI_RDDI_HUD", &InputMappings[9]},
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
