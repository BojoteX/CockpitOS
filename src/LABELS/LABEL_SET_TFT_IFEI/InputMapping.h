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
    uint16_t    releaseValue; // Deferred release value (0 = default, >0 = atomic press-delay-release)
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group  rel
static const InputMapping InputMappings[] = {
    { "IFEI_DWN_BTN"               , "NONE" ,  0 ,  0 ,  -1 , "IFEI_DWN_BTN"          ,     1 , "momentary"    ,  0 , 0 },
    { "IFEI_ET_BTN"                , "NONE" ,  0 ,  0 ,  -1 , "IFEI_ET_BTN"           ,     1 , "momentary"    ,  0 , 0 },
    { "IFEI_MODE_BTN"              , "NONE" ,  0 ,  0 ,  -1 , "IFEI_MODE_BTN"         ,     1 , "momentary"    ,  0 , 0 },
    { "IFEI_QTY_BTN"               , "NONE" ,  0 ,  0 ,  -1 , "IFEI_QTY_BTN"          ,     1 , "momentary"    ,  0 , 0 },
    { "IFEI_UP_BTN"                , "NONE" ,  0 ,  0 ,  -1 , "IFEI_UP_BTN"           ,     1 , "momentary"    ,  0 , 0 },
    { "IFEI_ZONE_BTN"              , "NONE" ,  0 ,  0 ,  -1 , "IFEI_ZONE_BTN"         ,     1 , "momentary"    ,  0 , 0 },
    { "CHART_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          , 65535 , "analog"       ,  0 , 0 },
    { "CHART_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     0 , "variable_step",  0 , 0 },
    { "CHART_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "CHART_DIMMER"          ,     1 , "variable_step",  0 , 0 },
    { "COCKKPIT_LIGHT_MODE_SW_DAY" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     0 , "selector"     ,  1 , 0 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     1 , "selector"     ,  1 , 0 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG" , "NONE" ,  0 ,  0 ,  -1 , "COCKKPIT_LIGHT_MODE_SW",     2 , "selector"     ,  1 , 0 },
    { "CONSOLES_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       , 65535 , "analog"       ,  0 , 0 },
    { "CONSOLES_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     0 , "variable_step",  0 , 0 },
    { "CONSOLES_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "CONSOLES_DIMMER"       ,     1 , "variable_step",  0 , 0 },
    { "FLOOD_DIMMER"               , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          , 65535 , "analog"       ,  0 , 0 },
    { "FLOOD_DIMMER_DEC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     0 , "variable_step",  0 , 0 },
    { "FLOOD_DIMMER_INC"           , "NONE" ,  0 ,  0 ,  -1 , "FLOOD_DIMMER"          ,     1 , "variable_step",  0 , 0 },
    { "INST_PNL_DIMMER"            , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       , 65535 , "analog"       ,  0 , 0 },
    { "INST_PNL_DIMMER_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     0 , "variable_step",  0 , 0 },
    { "INST_PNL_DIMMER_INC"        , "NONE" ,  0 ,  0 ,  -1 , "INST_PNL_DIMMER"       ,     1 , "variable_step",  0 , 0 },
    { "LIGHTS_TEST_SW_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     0 , "selector"     ,  2 , 0 },
    { "LIGHTS_TEST_SW_TEST"        , "NONE" ,  0 ,  0 ,  -1 , "LIGHTS_TEST_SW"        ,     1 , "selector"     ,  2 , 0 },
    { "WARN_CAUTION_DIMMER"        , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   , 65535 , "analog"       ,  0 , 0 },
    { "WARN_CAUTION_DIMMER_DEC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     0 , "variable_step",  0 , 0 },
    { "WARN_CAUTION_DIMMER_INC"    , "NONE" ,  0 ,  0 ,  -1 , "WARN_CAUTION_DIMMER"   ,     1 , "variable_step",  0 , 0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "COCKKPIT_LIGHT_MODE_SW",
    "LIGHTS_TEST_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {"WARN_CAUTION_DIMMER_DEC", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_ET_BTN", &InputMappings[1]},
  {"WARN_CAUTION_DIMMER_INC", &InputMappings[25]},
  {"CHART_DIMMER_INC", &InputMappings[8]},
  {"INST_PNL_DIMMER", &InputMappings[18]},
  {"CONSOLES_DIMMER_DEC", &InputMappings[13]},
  {"LIGHTS_TEST_SW_TEST", &InputMappings[22]},
  {nullptr, nullptr},
  {"IFEI_QTY_BTN", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER_INC", &InputMappings[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_NVG", &InputMappings[11]},
  {"CHART_DIMMER", &InputMappings[6]},
  {nullptr, nullptr},
  {"CHART_DIMMER_DEC", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLOOD_DIMMER", &InputMappings[15]},
  {"IFEI_UP_BTN", &InputMappings[4]},
  {"FLOOD_DIMMER_DEC", &InputMappings[16]},
  {nullptr, nullptr},
  {"CONSOLES_DIMMER", &InputMappings[12]},
  {"IFEI_ZONE_BTN", &InputMappings[5]},
  {nullptr, nullptr},
  {"COCKKPIT_LIGHT_MODE_SW_DAY", &InputMappings[9]},
  {"COCKKPIT_LIGHT_MODE_SW_NITE", &InputMappings[10]},
  {"WARN_CAUTION_DIMMER", &InputMappings[23]},
  {"FLOOD_DIMMER_INC", &InputMappings[17]},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_DEC", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INST_PNL_DIMMER_INC", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_DWN_BTN", &InputMappings[0]},
  {"IFEI_MODE_BTN", &InputMappings[2]},
  {"LIGHTS_TEST_SW_OFF", &InputMappings[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
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
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
