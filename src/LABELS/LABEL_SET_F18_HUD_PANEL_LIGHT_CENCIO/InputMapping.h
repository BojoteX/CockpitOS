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
    { "HMD_OFF_BRT"          , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"        , 65535 , "analog"       ,  0 },
    { "HMD_OFF_BRT_DEC"      , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"        ,     0 , "variable_step",  0 },
    { "HMD_OFF_BRT_INC"      , "NONE" ,  0 ,  0 ,  -1 , "HMD_OFF_BRT"        ,     1 , "variable_step",  0 },
    { "IR_COOL_SW_OFF"       , "GPIO" ,  2 ,  0 ,  -1 , "IR_COOL_SW"         ,     0 , "selector"     ,  2 },
    { "IR_COOL_SW_NORM"      , "GPIO" , -1 ,  0 ,  -1 , "IR_COOL_SW"         ,     1 , "selector"     ,  2 },
    { "IR_COOL_SW_ORIDE"     , "GPIO" ,  3 ,  0 ,  -1 , "IR_COOL_SW"         ,     2 , "selector"     ,  2 },
    { "SPIN_RECOVERY_COVER"  , "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_COVER",     1 , "momentary"    ,  0 },
    { "SPIN_RECOVERY_SW_NORM", "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_SW"   ,     0 , "selector"     ,  3 },
    { "SPIN_RECOVERY_SW_RCVY", "NONE" ,  0 ,  0 ,  -1 , "SPIN_RECOVERY_SW"   ,     1 , "selector"     ,  3 },
    { "MASTER_ARM_SW_SAFE"   , "GPIO" , -1 ,  0 ,  -1 , "MASTER_ARM_SW"      ,     0 , "selector"     ,  1 },
    { "MASTER_ARM_SW_ARM"    , "GPIO" ,  1 ,  0 ,  -1 , "MASTER_ARM_SW"      ,     1 , "selector"     ,  1 },
    { "MASTER_MODE_AA"       , "GPIO" ,  0 ,  0 ,  -1 , "MASTER_MODE_AA"     ,     1 , "momentary"    ,  0 },
    { "MASTER_MODE_AG"       , "NONE" ,  0 ,  0 ,  -1 , "MASTER_MODE_AG"     ,     1 , "momentary"    ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "IR_COOL_SW",
    "MASTER_ARM_SW",
    "SPIN_RECOVERY_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IR_COOL_SW_NORM", &InputMappings[4]},
  {"SPIN_RECOVERY_COVER", &InputMappings[6]},
  {"SPIN_RECOVERY_SW_NORM", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_MODE_AA", &InputMappings[11]},
  {nullptr, nullptr},
  {"SPIN_RECOVERY_SW_RCVY", &InputMappings[8]},
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
  {"HMD_OFF_BRT", &InputMappings[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_ARM_SW_ARM", &InputMappings[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"HMD_OFF_BRT_DEC", &InputMappings[1]},
  {nullptr, nullptr},
  {"IR_COOL_SW_ORIDE", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_ARM_SW_SAFE", &InputMappings[9]},
  {"MASTER_MODE_AG", &InputMappings[12]},
  {nullptr, nullptr},
  {"HMD_OFF_BRT_INC", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IR_COOL_SW_OFF", &InputMappings[3]},
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
  uint16_t startH = inputHash(label) % 53;
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
