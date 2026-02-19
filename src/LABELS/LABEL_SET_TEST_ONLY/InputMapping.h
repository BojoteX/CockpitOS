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
    { "CANOPY_JETT_HANDLE_PULL"  , "GPIO" ,  5 ,  0 ,  -1 , "CANOPY_JETT_HANDLE_PULL"  ,     1 , "momentary"    ,  0 },
    { "CANOPY_JETT_HANDLE_UNLOCK", "PCA_0x20" ,  0 ,  7 ,  -1 , "CANOPY_JETT_HANDLE_UNLOCK",     1 , "momentary"    ,  0 },
    { "AUX_REL_SW_NORM"          , "NONE" ,  0 ,  0 ,  -1 , "AUX_REL_SW"               ,     0 , "selector"     ,  1 },
    { "AUX_REL_SW_ENABLE"        , "NONE" ,  0 ,  0 ,  -1 , "AUX_REL_SW"               ,     1 , "selector"     ,  1 },
    { "CMSD_DISPENSE_SW_OFF"     , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"         ,     0 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_ON"      , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"         ,     1 , "selector"     ,  2 },
    { "CMSD_DISPENSE_SW_BYPASS"  , "NONE" ,  0 ,  0 ,  -1 , "CMSD_DISPENSE_SW"         ,     2 , "selector"     ,  2 },
    { "CMSD_JET_SEL_BTN"         , "GPIO" ,  5 ,  0 ,  -1 , "CMSD_JET_SEL_BTN"         ,     1 , "momentary"    ,  0 },
    { "ECM_MODE_SW_OFF"          , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     0 , "selector"     ,  3 },
    { "ECM_MODE_SW_STBY"         , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     1 , "selector"     ,  3 },
    { "ECM_MODE_SW_BIT"          , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     2 , "selector"     ,  3 },
    { "ECM_MODE_SW_REC"          , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     3 , "selector"     ,  3 },
    { "ECM_MODE_SW_XMIT"         , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     4 , "selector"     ,  3 },
    { "ECM_MODE_SW_DEC"          , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     0 , "fixed_step"   ,  0 },
    { "ECM_MODE_SW_INC"          , "NONE" ,  0 ,  0 ,  -1 , "ECM_MODE_SW"              ,     1 , "fixed_step"   ,  0 },
    { "LEFT_FIRE_BTN"            , "GPIO" , 10 ,  0 ,  -1 , "LEFT_FIRE_BTN"            ,     1 , "momentary"    ,  0 },
    { "LEFT_FIRE_BTN_COVER"      , "NONE" ,  0 ,  0 ,  -1 , "LEFT_FIRE_BTN_COVER"      ,     1 , "momentary"    ,  0 },
    { "HMD_OFF_BRT"              , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"              , 65535 , "analog"       ,  0 },
    { "HMD_OFF_BRT_DEC"          , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"              ,     0 , "variable_step",  0 },
    { "HMD_OFF_BRT_INC"          , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"              ,     1 , "variable_step",  0 },
    { "IR_COOL_SW_OFF"           , "NONE" ,  0 ,  0 ,  -1 , "IR_COOL_SW"               ,     0 , "selector"     ,  4 },
    { "IR_COOL_SW_NORM"          , "NONE" ,  0 ,  0 ,  -1 , "IR_COOL_SW"               ,     1 , "selector"     ,  4 },
    { "IR_COOL_SW_ORIDE"         , "NONE" ,  0 ,  0 ,  -1 , "IR_COOL_SW"               ,     2 , "selector"     ,  4 },
    { "SPIN_RECOVERY_COVER"      , "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_COVER"      ,     1 , "momentary"    ,  0 },
    { "SPIN_RECOVERY_SW_NORM"    , "GPIO" , -1 ,  0 ,  -1 , "SPIN_RECOVERY_SW"         ,     0 , "selector"     ,  5 },
    { "SPIN_RECOVERY_SW_RCVY"    , "GPIO" ,  7 ,  0 ,  -1 , "SPIN_RECOVERY_SW"         ,     1 , "selector"     ,  5 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "AUX_REL_SW",
    "CMSD_DISPENSE_SW",
    "ECM_MODE_SW",
    "IR_COOL_SW",
    "SPIN_RECOVERY_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {"CANOPY_JETT_HANDLE_UNLOCK", &InputMappings[1]},
  {"CMSD_DISPENSE_SW_ON", &InputMappings[5]},
  {"IR_COOL_SW_NORM", &InputMappings[21]},
  {"ECM_MODE_SW_REC", &InputMappings[11]},
  {"SPIN_RECOVERY_COVER", &InputMappings[23]},
  {"AUX_REL_SW_NORM", &InputMappings[2]},
  {"SPIN_RECOVERY_SW_NORM", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_STBY", &InputMappings[9]},
  {"SPIN_RECOVERY_SW_RCVY", &InputMappings[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_OFF", &InputMappings[8]},
  {nullptr, nullptr},
  {"AUX_REL_SW_ENABLE", &InputMappings[3]},
  {"LEFT_FIRE_BTN", &InputMappings[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_INC", &InputMappings[14]},
  {"ECM_MODE_SW_BIT", &InputMappings[10]},
  {"HMD_OFF_BRT", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ECM_MODE_SW_XMIT", &InputMappings[12]},
  {"CMSD_DISPENSE_SW_BYPASS", &InputMappings[6]},
  {"HMD_OFF_BRT_DEC", &InputMappings[18]},
  {nullptr, nullptr},
  {"IR_COOL_SW_ORIDE", &InputMappings[22]},
  {nullptr, nullptr},
  {"CMSD_JET_SEL_BTN", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"CMSD_DISPENSE_SW_OFF", &InputMappings[4]},
  {"ECM_MODE_SW_DEC", &InputMappings[13]},
  {nullptr, nullptr},
  {"HMD_OFF_BRT_INC", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IR_COOL_SW_OFF", &InputMappings[20]},
  {"CANOPY_JETT_HANDLE_PULL", &InputMappings[0]},
  {"LEFT_FIRE_BTN_COVER", &InputMappings[16]},
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
