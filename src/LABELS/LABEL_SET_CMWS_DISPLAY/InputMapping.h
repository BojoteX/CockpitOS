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
    { "PLT_CMWS_ARM_ARM"       , "GPIO" ,  0 ,  0 ,   1 , "PLT_CMWS_ARM"     ,     1 , "selector"                      ,  1 ,  0 },
    { "PLT_CMWS_ARM_SAFE"      , "GPIO" , -1 ,  0 ,  -1 , "PLT_CMWS_ARM"     ,     0 , "selector"                      ,  1 ,  0 },
    { "PLT_CMWS_BYPASS_BYPASS" , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_BYPASS"  ,     1 , "selector"                      ,  2 ,  0 },
    { "PLT_CMWS_BYPASS_AUTO"   , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_BYPASS"  ,     0 , "selector"                      ,  2 ,  0 },
    { "PLT_CMWS_JETT_POS0"     , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_JETT"    ,     0 , "selector"                      ,  3 ,  0 },
    { "PLT_CMWS_JETT_POS1"     , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_JETT"    ,     1 , "selector"                      ,  3 ,  0 },
    { "PLT_CMWS_JETT_CVR_OPEN" , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_JETT_CVR",     1 , "selector"                      ,  4 ,  0 },
    { "PLT_CMWS_JETT_CVR_CLOSE", "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_JETT_CVR",     0 , "selector"                      ,  4 ,  0 },
    { "PLT_CMWS_LAMP"          , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_LAMP"    , 65535 , "analog"                        ,  0 ,  0 },
    { "PLT_CMWS_LAMP_DEC"      , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_LAMP"    ,     0 , "variable_step"                 ,  0 ,  0 },
    { "PLT_CMWS_LAMP_INC"      , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_LAMP"    ,     1 , "variable_step"                 ,  0 ,  0 },
    { "PLT_CMWS_MODE_CMWS"     , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_MODE"    ,     1 , "selector"                      ,  5 ,  0 },
    { "PLT_CMWS_MODE_NAV"      , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_MODE"    ,     0 , "selector"                      ,  5 ,  0 },
    { "PLT_CMWS_PW_TEST"       , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_PW"      ,     0 , "3pos_2command_switch_openclose",  6 ,  0 },
    { "PLT_CMWS_PW_ON"         , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_PW"      ,     1 , "3pos_2command_switch_openclose",  6 ,  0 },
    { "PLT_CMWS_PW_OFF"        , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_PW"      ,     2 , "3pos_2command_switch_openclose",  6 ,  0 },
    { "PLT_CMWS_VOL"           , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_VOL"     , 65535 , "analog"                        ,  0 ,  0 },
    { "PLT_CMWS_VOL_DEC"       , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_VOL"     ,     0 , "variable_step"                 ,  0 ,  0 },
    { "PLT_CMWS_VOL_INC"       , "NONE" ,  0 ,  0 ,  -1 , "PLT_CMWS_VOL"     ,     1 , "variable_step"                 ,  0 ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "PLT_CMWS_ARM",
    "PLT_CMWS_BYPASS",
    "PLT_CMWS_JETT",
    "PLT_CMWS_JETT_CVR",
    "PLT_CMWS_MODE",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_BYPASS_AUTO", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_JETT_CVR_OPEN", &InputMappings[6]},
  {"PLT_CMWS_LAMP", &InputMappings[8]},
  {"PLT_CMWS_VOL", &InputMappings[16]},
  {"PLT_CMWS_VOL_DEC", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_ARM_SAFE", &InputMappings[1]},
  {"PLT_CMWS_JETT_CVR_CLOSE", &InputMappings[7]},
  {"PLT_CMWS_PW_OFF", &InputMappings[15]},
  {"PLT_CMWS_VOL_INC", &InputMappings[18]},
  {"PLT_CMWS_ARM_ARM", &InputMappings[0]},
  {"PLT_CMWS_BYPASS_BYPASS", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_PW_ON", &InputMappings[14]},
  {nullptr, nullptr},
  {"PLT_CMWS_LAMP_INC", &InputMappings[10]},
  {nullptr, nullptr},
  {"PLT_CMWS_LAMP_DEC", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_JETT_POS1", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_MODE_CMWS", &InputMappings[11]},
  {nullptr, nullptr},
  {"PLT_CMWS_PW_TEST", &InputMappings[13]},
  {"PLT_CMWS_MODE_NAV", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_JETT_POS0", &InputMappings[4]},
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
