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
    { "APU_CONTROL_SW_OFF"             , "NONE" ,  0 ,  0 ,  -1 , "APU_CONTROL_SW"     ,     0 , "selector"     ,  4 },
    { "APU_CONTROL_SW_ON"              , "NONE" ,  0 ,  0 ,  -1 , "APU_CONTROL_SW"     ,     1 , "selector"     ,  4 },
    { "ENGINE_CRANK_SW_RIGHT"          , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW"    ,     0 , "selector"     ,  5 },
    { "ENGINE_CRANK_SW_OFF"            , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW"    ,     1 , "selector"     ,  5 },
    { "ENGINE_CRANK_SW_LEFT"           , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW"    ,     2 , "selector"     ,  5 },
    { "FLIR_SW_OFF"                    , "NONE" ,  0 ,  0 ,  -1 , "FLIR_SW"            ,     0 , "selector"     ,  7 },
    { "FLIR_SW_STBY"                   , "NONE" ,  0 ,  0 ,  -1 , "FLIR_SW"            ,     1 , "selector"     ,  7 },
    { "FLIR_SW_ON"                     , "NONE" ,  0 ,  0 ,  -1 , "FLIR_SW"            ,     2 , "selector"     ,  7 },
    { "INS_SW_TEST"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     0 , "selector"     ,  8 },
    { "INS_SW_GB"                      , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     1 , "selector"     ,  8 },
    { "INS_SW_GYRO"                    , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     2 , "selector"     ,  8 },
    { "INS_SW_IFA"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     3 , "selector"     ,  8 },
    { "INS_SW_NAV"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     4 , "selector"     ,  8 },
    { "INS_SW_GND"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     5 , "selector"     ,  8 },
    { "INS_SW_CV"                      , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     6 , "selector"     ,  8 },
    { "INS_SW_OFF"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     7 , "selector"     ,  8 },
    { "INS_SW_DEC"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     0 , "fixed_step"   ,  0 },
    { "INS_SW_INC"                     , "NONE" ,  0 ,  0 ,  -1 , "INS_SW"             ,     1 , "fixed_step"   ,  0 },
    { "LST_NFLR_SW_OFF"                , "NONE" ,  0 ,  0 ,  -1 , "LST_NFLR_SW"        ,     0 , "selector"     ,  9 },
    { "LST_NFLR_SW_ON"                 , "NONE" ,  0 ,  0 ,  -1 , "LST_NFLR_SW"        ,     1 , "selector"     ,  9 },
    { "LTD_R_SW_SAFE"                  , "GPIO" , -1 ,  0 ,  -1 , "LTD_R_SW"           ,     0 , "selector"     , 12 },
    { "LTD_R_SW_ARM"                   , "GPIO" ,  2 ,  0 ,  -1 , "LTD_R_SW"           ,     1 , "selector"     , 12 },
    { "RADAR_SW_EMERG(PULL)"           , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"           ,     0 , "selector"     , 11 },
    { "RADAR_SW_OPR"                   , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"           ,     1 , "selector"     , 11 },
    { "RADAR_SW_STBY"                  , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"           ,     2 , "selector"     , 11 },
    { "RADAR_SW_OFF"                   , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"           ,     3 , "selector"     , 11 },
    { "RADAR_SW_DEC"                   , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"           ,     0 , "fixed_step"   ,  0 },
    { "RADAR_SW_INC"                   , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW"           ,     1 , "fixed_step"   ,  0 },
    { "RADAR_SW_PULL"                  , "NONE" ,  0 ,  0 ,  -1 , "RADAR_SW_PULL"      ,     1 , "momentary"    ,  0 },
    { "THROTTLE_ATC_SW"                , "GPIO" ,  9 ,  0 ,  -1 , "THROTTLE_ATC_SW"    ,     1 , "momentary"    ,  0 },
    { "THROTTLE_CAGE_BTN"              , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_CAGE_BTN"  ,     1 , "momentary"    ,  0 },
    { "THROTTLE_DISP_SW_FORWARD(CHAFF)", "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_DISP_SW"   ,     0 , "selector"     ,  1 },
    { "THROTTLE_DISP_SW_CENTER(OFF)"   , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_DISP_SW"   ,     1 , "selector"     ,  1 },
    { "THROTTLE_DISP_SW_AFT(FLARE)"    , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_DISP_SW"   ,     2 , "selector"     ,  1 },
    { "THROTTLE_EXT_L_SW_OFF"          , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_EXT_L_SW"  ,     0 , "selector"     ,  2 },
    { "THROTTLE_EXT_L_SW_ON"           , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_EXT_L_SW"  ,     1 , "selector"     ,  2 },
    { "THROTTLE_FOV_SEL_SW"            , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_FOV_SEL_SW",     1 , "momentary"    ,  0 },
    { "THROTTLE_FRICTION"              , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_FRICTION"  , 65535 , "analog"       ,  0 },
    { "THROTTLE_FRICTION_DEC"          , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_FRICTION"  ,     0 , "variable_step",  0 },
    { "THROTTLE_FRICTION_INC"          , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_FRICTION"  ,     1 , "variable_step",  0 },
    { "THROTTLE_RADAR_ELEV"            , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_RADAR_ELEV", 65535 , "analog"       ,  0 },
    { "THROTTLE_RADAR_ELEV_DEC"        , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_RADAR_ELEV",     0 , "variable_step",  0 },
    { "THROTTLE_RADAR_ELEV_INC"        , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_RADAR_ELEV",     1 , "variable_step",  0 },
    { "THROTTLE_SPEED_BRK_RETRACT"     , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_SPEED_BRK" ,     0 , "selector"     ,  3 },
    { "THROTTLE_SPEED_BRK_OFF"         , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_SPEED_BRK" ,     1 , "selector"     ,  3 },
    { "THROTTLE_SPEED_BRK_EXTEND"      , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_SPEED_BRK" ,     2 , "selector"     ,  3 },
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
    "RADAR_SW",
    "THROTTLE_DISP_SW",
    "THROTTLE_EXT_L_SW",
    "THROTTLE_SPEED_BRK",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[97] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INS_SW_CV", &InputMappings[14]},
  {"APU_CONTROL_SW_OFF", &InputMappings[0]},
  {"INS_SW_NAV", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_EXT_L_SW_OFF", &InputMappings[34]},
  {"THROTTLE_DISP_SW_AFT(FLARE)", &InputMappings[33]},
  {"THROTTLE_FRICTION_DEC", &InputMappings[38]},
  {"ENGINE_CRANK_SW_OFF", &InputMappings[3]},
  {"INS_SW_TEST", &InputMappings[8]},
  {nullptr, nullptr},
  {"THROTTLE_ATC_SW", &InputMappings[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW_RIGHT", &InputMappings[2]},
  {"ENGINE_CRANK_SW_LEFT", &InputMappings[4]},
  {"INS_SW_GND", &InputMappings[13]},
  {"LTD_R_SW_ARM", &InputMappings[21]},
  {nullptr, nullptr},
  {"THROTTLE_FRICTION_INC", &InputMappings[39]},
  {"THROTTLE_SPEED_BRK_RETRACT", &InputMappings[43]},
  {"INS_SW_IFA", &InputMappings[11]},
  {"THROTTLE_SPEED_BRK_OFF", &InputMappings[44]},
  {"INS_SW_DEC", &InputMappings[16]},
  {nullptr, nullptr},
  {"RADAR_SW_STBY", &InputMappings[24]},
  {"RADAR_SW_INC", &InputMappings[27]},
  {"THROTTLE_DISP_SW_CENTER(OFF)", &InputMappings[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_SPEED_BRK_EXTEND", &InputMappings[45]},
  {nullptr, nullptr},
  {"RADAR_SW_DEC", &InputMappings[26]},
  {nullptr, nullptr},
  {"THROTTLE_FOV_SEL_SW", &InputMappings[36]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"RADAR_SW_EMERG(PULL)", &InputMappings[22]},
  {"THROTTLE_FRICTION", &InputMappings[37]},
  {"RADAR_SW_OFF", &InputMappings[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INS_SW_INC", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INS_SW_GYRO", &InputMappings[10]},
  {nullptr, nullptr},
  {"INS_SW_GB", &InputMappings[9]},
  {"FLIR_SW_STBY", &InputMappings[6]},
  {"FLIR_SW_ON", &InputMappings[7]},
  {"RADAR_SW_OPR", &InputMappings[23]},
  {"RADAR_SW_PULL", &InputMappings[28]},
  {"THROTTLE_DISP_SW_FORWARD(CHAFF)", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"INS_SW_OFF", &InputMappings[15]},
  {"THROTTLE_RADAR_ELEV_INC", &InputMappings[42]},
  {nullptr, nullptr},
  {"FLIR_SW_OFF", &InputMappings[5]},
  {"THROTTLE_RADAR_ELEV", &InputMappings[40]},
  {"LST_NFLR_SW_OFF", &InputMappings[18]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"APU_CONTROL_SW_ON", &InputMappings[1]},
  {"LTD_R_SW_SAFE", &InputMappings[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_RADAR_ELEV_DEC", &InputMappings[41]},
  {nullptr, nullptr},
  {"LST_NFLR_SW_ON", &InputMappings[19]},
  {"THROTTLE_CAGE_BTN", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_EXT_L_SW_ON", &InputMappings[35]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 97;
  for (uint16_t i = 0; i < 97; ++i) {
    uint16_t idx = (startH + i >= 97) ? (startH + i - 97) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
