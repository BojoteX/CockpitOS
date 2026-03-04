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
    { "APU_CONTROL_SW_OFF"            , "GPIO" , -1 ,  0 ,  -1 , "APU_CONTROL_SW" ,     0 , "selector"  ,  1 ,  0 },
    { "APU_CONTROL_SW_ON"             , "GPIO" ,  4 ,  0 ,  -1 , "APU_CONTROL_SW" ,     1 , "selector"  ,  1 ,  0 },
    { "ENGINE_CRANK_SW_RIGHT"         , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW",     0 , "selector"  ,  2 ,  0 },
    { "ENGINE_CRANK_SW_OFF"           , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW",     1 , "selector"  ,  2 ,  0 },
    { "ENGINE_CRANK_SW_LEFT"          , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW",     2 , "selector"  ,  2 ,  0 },
    { "MASTER_ARM_SW_SAFE"            , "NONE" ,  0 ,  0 ,  -1 , "MASTER_ARM_SW"  ,     0 , "selector"  ,  3 ,  0 },
    { "MASTER_ARM_SW_ARM"             , "NONE" ,  0 ,  0 ,  -1 , "MASTER_ARM_SW"  ,     1 , "selector"  ,  3 ,  0 },
    { "MASTER_MODE_AA"                , "NONE" ,  0 ,  0 ,  -1 , "MASTER_MODE_AA" ,     1 , "momentary" ,  0 ,  0 },
    { "MASTER_MODE_AG"                , "NONE" ,  0 ,  0 ,  -1 , "MASTER_MODE_AG" ,     1 , "momentary" ,  0 ,  0 },
    { "FLIR_SW_OFF"                   , "NONE" ,  0 ,  0 ,  -1 , "FLIR_SW"        ,     0 , "selector"  ,  4 ,  0 },
    { "FLIR_SW_STBY"                  , "NONE" ,  0 ,  0 ,  -1 , "FLIR_SW"        ,     1 , "selector"  ,  4 ,  0 },
    { "FLIR_SW_ON"                    , "NONE" ,  0 ,  0 ,  -1 , "FLIR_SW"        ,     2 , "selector"  ,  4 ,  0 },
    { "INS_SW_TEST"                   , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     0 , "selector"  ,  5 ,  0 },
    { "INS_SW_GB"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     1 , "selector"  ,  5 ,  0 },
    { "INS_SW_GYRO"                   , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     2 , "selector"  ,  5 ,  0 },
    { "INS_SW_IFA"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     3 , "selector"  ,  5 ,  0 },
    { "INS_SW_NAV"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     4 , "selector"  ,  5 ,  0 },
    { "INS_SW_GND"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     5 , "selector"  ,  5 ,  0 },
    { "INS_SW_CV"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     6 , "selector"  ,  5 ,  0 },
    { "INS_SW_OFF"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     7 , "selector"  ,  5 ,  0 },
    { "INS_SW_DEC"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     0 , "fixed_step",  0 ,  0 },
    { "INS_SW_INC"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"         ,     1 , "fixed_step",  0 ,  0 },
    { "LST_NFLR_SW_OFF"               , "NONE" ,  0 ,  0 ,  -1 , "LST_NFLR_SW"    ,     0 , "selector"  ,  6 ,  0 },
    { "LST_NFLR_SW_ON"                , "NONE" ,  0 ,  0 ,  -1 , "LST_NFLR_SW"    ,     1 , "selector"  ,  6 ,  0 },
    { "LTD_R_SW_SAFE"                 , "NONE" ,  0 ,  0 ,  -1 , "LTD_R_SW"       ,     0 , "selector"  ,  7 ,  0 },
    { "LTD_R_SW_ARM"                  , "NONE" ,  0 ,  0 ,  -1 , "LTD_R_SW"       ,     1 , "selector"  ,  7 ,  0 },
    { "RADAR_SW_EMERG(PULL)"          , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"       ,     0 , "selector"  ,  8 ,  0 },
    { "RADAR_SW_OPR"                  , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"       ,     1 , "selector"  ,  8 ,  0 },
    { "RADAR_SW_STBY"                 , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"       ,     2 , "selector"  ,  8 ,  0 },
    { "RADAR_SW_OFF"                  , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"       ,     3 , "selector"  ,  8 ,  0 },
    { "RADAR_SW_DEC"                  , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"       ,     0 , "fixed_step",  0 ,  0 },
    { "RADAR_SW_INC"                  , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"       ,     1 , "fixed_step",  0 ,  0 },
    { "RADAR_SW_PULL"                 , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW_PULL"  ,     1 , "momentary" ,  0 ,  0 },
    { "ENGINE_CRANK_SW_CUSTOM_PRESS"  , "GPIO" ,  3 ,  0 ,  -1 , "ENGINE_CRANK_SW",     2 , "momentary" ,  0 ,  1 },
    { "ENGINE_CRANK_SW_CUSTOM_2_PRESS", "GPIO" ,  2 ,  0 ,  -1 , "ENGINE_CRANK_SW",     0 , "momentary" ,  0 ,  1 },
    { "LTD_R_SW_CUSTOM_PRESS"         , "GPIO" ,  0 ,  0 ,  -1 , "LTD_R_SW"       ,     1 , "momentary" ,  0 ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "APU_CONTROL_SW",
    "ENGINE_CRANK_SW",
    "FLIR_SW",
    "INS_SW",
    "LST_NFLR_SW",
    "LTD_R_SW",
    "MASTER_ARM_SW",
    "RADAR_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[73] = {
  {"RADAR_SW_INC", &InputMappings[31]},
  {"MASTER_ARM_SW_SAFE", &InputMappings[5]},
  {"FLIR_SW_OFF", &InputMappings[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_CONTROL_SW_OFF", &InputMappings[0]},
  {"RADAR_SW_OPR", &InputMappings[27]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"LTD_R_SW_SAFE", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADAR_SW_EMERG(PULL)", &InputMappings[26]},
  {"MASTER_MODE_AG", &InputMappings[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INS_SW_IFA", &InputMappings[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADAR_SW_PULL", &InputMappings[32]},
  {"INS_SW_GYRO", &InputMappings[14]},
  {"INS_SW_NAV", &InputMappings[16]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADAR_SW_DEC", &InputMappings[30]},
  {"INS_SW_GB", &InputMappings[13]},
  {nullptr, nullptr},
  {"INS_SW_CV", &InputMappings[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"FLIR_SW_STBY", &InputMappings[10]},
  {"RADAR_SW_STBY", &InputMappings[28]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW_RIGHT", &InputMappings[2]},
  {"APU_CONTROL_SW_ON", &InputMappings[1]},
  {"MASTER_MODE_AA", &InputMappings[7]},
  {"ENGINE_CRANK_SW_OFF", &InputMappings[3]},
  {"INS_SW_OFF", &InputMappings[19]},
  {"INS_SW_DEC", &InputMappings[20]},
  {"LST_NFLR_SW_ON", &InputMappings[23]},
  {"LTD_R_SW_ARM", &InputMappings[25]},
  {"ENGINE_CRANK_SW_CUSTOM_2_PRESS", &InputMappings[34]},
  {"RADAR_SW_OFF", &InputMappings[29]},
  {"FLIR_SW_ON", &InputMappings[11]},
  {"ENGINE_CRANK_SW_CUSTOM_PRESS", &InputMappings[33]},
  {"MASTER_ARM_SW_ARM", &InputMappings[6]},
  {"ENGINE_CRANK_SW_LEFT", &InputMappings[4]},
  {"INS_SW_TEST", &InputMappings[12]},
  {"LST_NFLR_SW_OFF", &InputMappings[22]},
  {"LTD_R_SW_CUSTOM_PRESS", &InputMappings[35]},
  {nullptr, nullptr},
  {"INS_SW_GND", &InputMappings[17]},
  {"INS_SW_INC", &InputMappings[21]},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 73;
  for (uint16_t i = 0; i < 73; ++i) {
    uint16_t idx = (startH + i >= 73) ? (startH + i - 73) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
