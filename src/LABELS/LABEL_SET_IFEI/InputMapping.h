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
    { "IFEI"                       , "GPIO" , PIN(3) ,  0 ,  -1 , "IFEI"                  , 65535 , "analog"       ,  0 ,  0 },
    { "IFEI_DEC"                   , "NONE" ,  0 ,  0 ,  -1 , "IFEI"                  ,     0 , "variable_step",  0 ,  0 },
    { "IFEI_INC"                   , "NONE" ,  0 ,  0 ,  -1 , "IFEI"                  ,     1 , "variable_step",  0 ,  0 },
    { "MODE_SELECTOR_SW_AUTO"      , "HC165" ,  0 ,  6 ,  -1 , "MODE_SELECTOR_SW"      ,     0 , "selector"     ,  5 ,  0 },
    { "MODE_SELECTOR_SW_OFF"       , "HC165" ,  0 , -1 ,  -1 , "MODE_SELECTOR_SW"      ,     1 , "selector"     ,  5 ,  0 },
    { "MODE_SELECTOR_SW_MAN"       , "HC165" ,  0 ,  0 ,  -1 , "MODE_SELECTOR_SW"      ,     2 , "selector"     ,  5 ,  0 },
    { "SELECT_HMD_LDDI_RDDI_RDDI"  , "HC165" ,  0 ,  1 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     0 , "selector"     ,  4 ,  0 },
    { "SELECT_HMD_LDDI_RDDI_LDDI"  , "HC165" ,  0 , -1 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     1 , "selector"     ,  4 ,  0 },
    { "SELECT_HMD_LDDI_RDDI_HMD"   , "HC165" ,  0 ,  2 ,  -1 , "SELECT_HMD_LDDI_RDDI"  ,     2 , "selector"     ,  4 ,  0 },
    { "SELECT_HUD_LDDI_RDDI_RDDI"  , "HC165" ,  0 ,  3 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     0 , "selector"     ,  3 ,  0 },
    { "SELECT_HUD_LDDI_RDDI_LDIR"  , "HC165" ,  0 , -1 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     1 , "selector"     ,  3 ,  0 },
    { "SELECT_HUD_LDDI_RDDI_HUD"   , "HC165" ,  0 ,  4 ,  -1 , "SELECT_HUD_LDDI_RDDI"  ,     2 , "selector"     ,  3 ,  0 },
    { "IFEI_DWN_BTN"               , "HC165" ,  0 , 11 ,  -1 , "IFEI_DWN_BTN"          ,     1 , "momentary"    ,  0 ,  0 },
    { "IFEI_ET_BTN"                , "HC165" ,  0 , 13 ,  -1 , "IFEI_ET_BTN"           ,     1 , "momentary"    ,  0 ,  0 },
    { "IFEI_MODE_BTN"              , "HC165" ,  0 ,  8 ,  -1 , "IFEI_MODE_BTN"         ,     1 , "momentary"    ,  0 ,  0 },
    { "IFEI_QTY_BTN"               , "HC165" ,  0 ,  9 ,  -1 , "IFEI_QTY_BTN"          ,     1 , "momentary"    ,  0 ,  0 },
    { "IFEI_UP_BTN"                , "HC165" ,  0 , 10 ,  -1 , "IFEI_UP_BTN"           ,     1 , "momentary"    ,  0 ,  0 },
    { "IFEI_ZONE_BTN"              , "HC165" ,  0 , 12 ,  -1 , "IFEI_ZONE_BTN"         ,     1 , "momentary"    ,  0 ,  0 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 ,  0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 ,  0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  1 ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  1 ,  0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  1 ,  0 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 ,  0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 ,  0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 ,  0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 ,  0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 ,  0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 ,  0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 ,  0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 ,  0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 ,  0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  2 ,  0 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  2 ,  0 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 ,  0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 ,  0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 ,  0 },
    { "SJ_CTR"                     , "TM1637" , PIN(8) ,  1 ,   1 , "SJ_CTR"                ,     1 , "momentary"    ,  0 ,  0 },
    { "SJ_LI"                      , "TM1637" , PIN(8) ,  0 ,   2 , "SJ_LI"                 ,     1 , "momentary"    ,  0 ,  0 },
    { "SJ_LO"                      , "TM1637" , PIN(8) ,  3 ,   3 , "SJ_LO"                 ,     1 , "momentary"    ,  0 ,  0 },
    { "SJ_RI"                      , "TM1637" , PIN(8) ,  2 ,   4 , "SJ_RI"                 ,     1 , "momentary"    ,  0 ,  0 },
    { "SJ_RO"                      , "TM1637" , PIN(8) ,  4 ,   5 , "SJ_RO"                 ,     1 , "momentary"    ,  0 ,  0 },
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
static const InputHashEntry inputHashTable[89] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SELECT_HMD_LDDI_RDDI_HMD", &InputMappings[8]},
  {"IFEI_DEC", &InputMappings[1]},
  {"SELECT_HUD_LDDI_RDDI_HUD", &InputMappings[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SELECT_HMD_LDDI_RDDI_LDDI", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[26]},
  {"IFEI_QTY_BTN", &InputMappings[15]},
  {"SJ_RO", &InputMappings[42]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SJ_LI", &InputMappings[39]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[27]},
  {nullptr, nullptr},
  {"IFEI_INC", &InputMappings[2]},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI", &InputMappings[0]},
  {"SELECT_HUD_LDDI_RDDI_RDDI", &InputMappings[9]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[37]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_OFF", &InputMappings[4]},
  {"SELECT_HUD_LDDI_RDDI_LDIR", &InputMappings[10]},
  {nullptr, nullptr},
  {"SELECT_HMD_LDDI_RDDI_RDDI", &InputMappings[6]},
  {nullptr, nullptr},
  {"SJ_LO", &InputMappings[40]},
  {"CHART_DIMMER", &InputMappings[18]},
  {"MODE_SELECTOR_SW_AUTO", &InputMappings[3]},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[21]},
  {"FLOOD_DIMMER_DEC", &InputMappings[28]},
  {"SJ_RI", &InputMappings[41]},
  {nullptr, nullptr},
  {"MODE_SELECTOR_SW_MAN", &InputMappings[5]},
  {"IFEI_MODE_BTN", &InputMappings[14]},
  {"CHART_DIMMER_INC", &InputMappings[20]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[33]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[34]},
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER", &InputMappings[35]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[23]},
  {"INST_PNL_DIMMER_DEC", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[19]},
  {nullptr, nullptr},
  {"IFEI_ET_BTN", &InputMappings[13]},
  {"IFEI_UP_BTN", &InputMappings[16]},
  {"IFEI_ZONE_BTN", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[22]},
  {"INST_PNL_DIMMER", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_DEC", &InputMappings[25]},
  {nullptr, nullptr},
  {"IFEI_DWN_BTN", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"SJ_CTR", &InputMappings[38]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_INC", &InputMappings[32]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[24]},
  {"FLOOD_DIMMER_INC", &InputMappings[29]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 89;
  for (uint16_t i = 0; i < 89; ++i) {
    uint16_t idx = (startH + i >= 89) ? (startH + i - 89) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
