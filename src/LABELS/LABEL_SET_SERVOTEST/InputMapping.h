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
    { "APU_CONTROL_SW_OFF"             , "GPIO" , -1 ,  0 ,  -1 , "APU_CONTROL_SW"     ,     0 , "selector"     ,  4 },
    { "APU_CONTROL_SW_ON"              , "GPIO" ,  2 ,  0 ,  -1 , "APU_CONTROL_SW"     ,     1 , "selector"     ,  4 },
    { "ENGINE_CRANK_SW_RIGHT"          , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW"    ,     0 , "selector"     ,  5 },
    { "ENGINE_CRANK_SW_OFF"            , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW"    ,     1 , "selector"     ,  5 },
    { "ENGINE_CRANK_SW_LEFT"           , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_CRANK_SW"    ,     2 , "selector"     ,  5 },
    { "THROTTLE_ATC_SW"                , "NONE" ,  0 ,  0 ,  -1 , "THROTTLE_ATC_SW"    ,     1 , "momentary"    ,  0 },
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
    "THROTTLE_DISP_SW",
    "THROTTLE_EXT_L_SW",
    "THROTTLE_SPEED_BRK",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_FRICTION_INC", &InputMappings[15]},
  {nullptr, nullptr},
  {"THROTTLE_ATC_SW", &InputMappings[5]},
  {"THROTTLE_DISP_SW_AFT(FLARE)", &InputMappings[9]},
  {"THROTTLE_EXT_L_SW_OFF", &InputMappings[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_RADAR_ELEV_INC", &InputMappings[18]},
  {"APU_CONTROL_SW_OFF", &InputMappings[0]},
  {"THROTTLE_DISP_SW_CENTER(OFF)", &InputMappings[8]},
  {"THROTTLE_RADAR_ELEV", &InputMappings[16]},
  {"THROTTLE_SPEED_BRK_OFF", &InputMappings[20]},
  {nullptr, nullptr},
  {"APU_CONTROL_SW_ON", &InputMappings[1]},
  {"THROTTLE_EXT_L_SW_ON", &InputMappings[11]},
  {"THROTTLE_FRICTION", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_DISP_SW_FORWARD(CHAFF)", &InputMappings[7]},
  {"THROTTLE_FOV_SEL_SW", &InputMappings[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_SPEED_BRK_RETRACT", &InputMappings[19]},
  {nullptr, nullptr},
  {"THROTTLE_RADAR_ELEV_DEC", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW_RIGHT", &InputMappings[2]},
  {nullptr, nullptr},
  {"ENGINE_CRANK_SW_OFF", &InputMappings[3]},
  {"ENGINE_CRANK_SW_LEFT", &InputMappings[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"THROTTLE_CAGE_BTN", &InputMappings[6]},
  {"THROTTLE_SPEED_BRK_EXTEND", &InputMappings[21]},
  {nullptr, nullptr},
  {"THROTTLE_FRICTION_DEC", &InputMappings[14]},
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
