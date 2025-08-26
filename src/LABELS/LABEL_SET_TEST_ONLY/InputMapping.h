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
    { "APU_FIRE_BTN"           , "TM1637" , PIN(40) ,  3 ,  -1 , "APU_FIRE_BTN"           ,     1 , "momentary"    ,  0 },
    { "LEFT_FIRE_BTN"          , "TM1637" , PIN(39) ,  3 ,  -1 , "LEFT_FIRE_BTN"          ,     1 , "momentary"    ,  0 },
    { "LEFT_FIRE_BTN_COVER"    , "NONE" ,  0 ,  0 ,  -1 , "LEFT_FIRE_BTN_COVER"    ,     1 , "momentary"    ,  0 },
    { "MASTER_CAUTION_RESET_SW", "TM1637" , PIN(39) ,  0 ,  -1 , "MASTER_CAUTION_RESET_SW",     1 , "momentary"    ,  0 },
    { "RWR_AUDIO_CTRL"         , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"         , 65535 , "analog"       ,  0 },
    { "RWR_AUDIO_CTRL_DEC"     , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"         ,     0 , "variable_step",  0 },
    { "RWR_AUDIO_CTRL_INC"     , "NONE" ,  0 ,  0 ,  -1 , "RWR_AUDIO_CTRL"         ,     1 , "variable_step",  0 },
    { "RWR_BIT_BTN"            , "NONE" ,  0 ,  0 ,  -1 , "RWR_BIT_BTN"            ,     1 , "momentary"    ,  0 },
    { "RWR_DISPLAY_BTN"        , "NONE" ,  0 ,  0 ,  -1 , "RWR_DISPLAY_BTN"        ,     1 , "momentary"    ,  0 },
    { "RWR_DIS_TYPE_SW_N"      , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"        ,     0 , "selector"     ,  1 },
    { "RWR_DIS_TYPE_SW_I"      , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"        ,     1 , "selector"     ,  1 },
    { "RWR_DIS_TYPE_SW_A"      , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"        ,     2 , "selector"     ,  1 },
    { "RWR_DIS_TYPE_SW_U"      , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"        ,     3 , "selector"     ,  1 },
    { "RWR_DIS_TYPE_SW_F"      , "NONE" ,  0 ,  0 ,  -1 , "RWR_DIS_TYPE_SW"        ,     4 , "selector"     ,  1 },
    { "RWR_DMR_CTRL"           , "NONE" ,  0 ,  0 ,  -1 , "RWR_DMR_CTRL"           , 65535 , "analog"       ,  0 },
    { "RWR_DMR_CTRL_DEC"       , "NONE" ,  0 ,  0 ,  -1 , "RWR_DMR_CTRL"           ,     0 , "variable_step",  0 },
    { "RWR_DMR_CTRL_INC"       , "NONE" ,  0 ,  0 ,  -1 , "RWR_DMR_CTRL"           ,     1 , "variable_step",  0 },
    { "RWR_OFFSET_BTN"         , "NONE" ,  0 ,  0 ,  -1 , "RWR_OFFSET_BTN"         ,     1 , "momentary"    ,  0 },
    { "RWR_POWER_BTN"          , "GPIO" ,  1 ,  0 ,  -1 , "RWR_POWER_BTN"          ,     1 , "momentary"    ,  0 },
    { "RWR_RWR_INTESITY"       , "NONE" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"       , 65535 , "analog"       ,  0 },
    { "RWR_RWR_INTESITY_DEC"   , "NONE" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"       ,     0 , "variable_step",  0 },
    { "RWR_RWR_INTESITY_INC"   , "NONE" ,  0 ,  0 ,  -1 , "RWR_RWR_INTESITY"       ,     1 , "variable_step",  0 },
    { "RWR_SPECIAL_BTN"        , "NONE" ,  0 ,  0 ,  -1 , "RWR_SPECIAL_BTN"        ,     1 , "momentary"    ,  0 },
    { "RIGHT_FIRE_BTN"         , "TM1637" , PIN(40) ,  0 ,  -1 , "RIGHT_FIRE_BTN"         ,     1 , "momentary"    ,  0 },
    { "RIGHT_FIRE_BTN_COVER"   , "NONE" ,  0 ,  0 ,  -1 , "RIGHT_FIRE_BTN_COVER"   ,     1 , "momentary"    ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "RWR_DIS_TYPE_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {"RWR_DIS_TYPE_SW_U", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_RESET_SW", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_RWR_INTESITY_INC", &InputMappings[21]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_AUDIO_CTRL_INC", &InputMappings[6]},
  {"RWR_DMR_CTRL_INC", &InputMappings[16]},
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
  {"RWR_RWR_INTESITY", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LEFT_FIRE_BTN", &InputMappings[1]},
  {"APU_FIRE_BTN", &InputMappings[0]},
  {"RIGHT_FIRE_BTN", &InputMappings[23]},
  {"RWR_AUDIO_CTRL", &InputMappings[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RWR_DIS_TYPE_SW_A", &InputMappings[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LEFT_FIRE_BTN_COVER", &InputMappings[2]},
  {"RWR_DIS_TYPE_SW_F", &InputMappings[13]},
  {"RWR_DISPLAY_BTN", &InputMappings[8]},
  {"RWR_DIS_TYPE_SW_I", &InputMappings[10]},
  {"RWR_OFFSET_BTN", &InputMappings[17]},
  {"RWR_RWR_INTESITY_DEC", &InputMappings[20]},
  {"RWR_POWER_BTN", &InputMappings[18]},
  {"RWR_AUDIO_CTRL_DEC", &InputMappings[5]},
  {"RWR_DIS_TYPE_SW_N", &InputMappings[9]},
  {"RWR_BIT_BTN", &InputMappings[7]},
  {"RWR_DMR_CTRL", &InputMappings[14]},
  {"RWR_DMR_CTRL_DEC", &InputMappings[15]},
  {"RWR_SPECIAL_BTN", &InputMappings[22]},
  {"RIGHT_FIRE_BTN_COVER", &InputMappings[24]},
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
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
