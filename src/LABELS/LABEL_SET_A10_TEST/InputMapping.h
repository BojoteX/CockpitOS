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
    { "AHCP_ALT_SCE_POS0"     , "NONE" ,  0 ,  0 ,  -1 , "AHCP_ALT_SCE"     ,     0 , "selector" ,  1 , 0 },
    { "AHCP_ALT_SCE_POS1"     , "NONE" ,  0 ,  0 ,  -1 , "AHCP_ALT_SCE"     ,     1 , "selector" ,  1 , 0 },
    { "AHCP_ALT_SCE_POS2"     , "NONE" ,  0 ,  0 ,  -1 , "AHCP_ALT_SCE"     ,     2 , "selector" ,  1 , 0 },
    { "AHCP_CICU_POS0"        , "NONE" ,  0 ,  0 ,  -1 , "AHCP_CICU"        ,     0 , "selector" ,  2 , 0 },
    { "AHCP_CICU_POS1"        , "NONE" ,  0 ,  0 ,  -1 , "AHCP_CICU"        ,     1 , "selector" ,  2 , 0 },
    { "AHCP_GUNPAC_POS0"      , "NONE" ,  0 ,  0 ,  -1 , "AHCP_GUNPAC"      ,     0 , "selector" ,  3 , 0 },
    { "AHCP_GUNPAC_POS1"      , "NONE" ,  0 ,  0 ,  -1 , "AHCP_GUNPAC"      ,     1 , "selector" ,  3 , 0 },
    { "AHCP_GUNPAC_POS2"      , "NONE" ,  0 ,  0 ,  -1 , "AHCP_GUNPAC"      ,     2 , "selector" ,  3 , 0 },
    { "AHCP_HUD_DAYNIGHT_POS0", "NONE" ,  0 ,  0 ,  -1 , "AHCP_HUD_DAYNIGHT",     0 , "selector" ,  4 , 0 },
    { "AHCP_HUD_DAYNIGHT_POS1", "NONE" ,  0 ,  0 ,  -1 , "AHCP_HUD_DAYNIGHT",     1 , "selector" ,  4 , 0 },
    { "AHCP_HUD_MODE_POS0"    , "NONE" ,  0 ,  0 ,  -1 , "AHCP_HUD_MODE"    ,     0 , "selector" ,  5 , 0 },
    { "AHCP_HUD_MODE_POS1"    , "NONE" ,  0 ,  0 ,  -1 , "AHCP_HUD_MODE"    ,     1 , "selector" ,  5 , 0 },
    { "AHCP_IFFCC_POS0"       , "NONE" ,  0 ,  0 ,  -1 , "AHCP_IFFCC"       ,     0 , "selector" ,  6 , 0 },
    { "AHCP_IFFCC_POS1"       , "NONE" ,  0 ,  0 ,  -1 , "AHCP_IFFCC"       ,     1 , "selector" ,  6 , 0 },
    { "AHCP_IFFCC_POS2"       , "NONE" ,  0 ,  0 ,  -1 , "AHCP_IFFCC"       ,     2 , "selector" ,  6 , 0 },
    { "AHCP_JTRS_POS0"        , "NONE" ,  0 ,  0 ,  -1 , "AHCP_JTRS"        ,     0 , "selector" ,  7 , 0 },
    { "AHCP_JTRS_POS1"        , "NONE" ,  0 ,  0 ,  -1 , "AHCP_JTRS"        ,     1 , "selector" ,  7 , 0 },
    { "AHCP_LASER_ARM_POS0"   , "NONE" ,  0 ,  0 ,  -1 , "AHCP_LASER_ARM"   ,     0 , "selector" ,  8 , 0 },
    { "AHCP_LASER_ARM_POS1"   , "NONE" ,  0 ,  0 ,  -1 , "AHCP_LASER_ARM"   ,     1 , "selector" ,  8 , 0 },
    { "AHCP_LASER_ARM_POS2"   , "NONE" ,  0 ,  0 ,  -1 , "AHCP_LASER_ARM"   ,     2 , "selector" ,  8 , 0 },
    { "AHCP_MASTER_ARM_POS0"  , "NONE" ,  0 ,  0 ,  -1 , "AHCP_MASTER_ARM"  ,     0 , "selector" ,  9 , 0 },
    { "AHCP_MASTER_ARM_POS1"  , "NONE" ,  0 ,  0 ,  -1 , "AHCP_MASTER_ARM"  ,     1 , "selector" ,  9 , 0 },
    { "AHCP_MASTER_ARM_POS2"  , "NONE" ,  0 ,  0 ,  -1 , "AHCP_MASTER_ARM"  ,     2 , "selector" ,  9 , 0 },
    { "AHCP_TGP_POS0"         , "NONE" ,  0 ,  0 ,  -1 , "AHCP_TGP"         ,     0 , "selector" , 10 , 0 },
    { "AHCP_TGP_POS1"         , "NONE" ,  0 ,  0 ,  -1 , "AHCP_TGP"         ,     1 , "selector" , 10 , 0 },
    { "ANTI_SKID_SWITCH_POS0" , "NONE" ,  0 ,  0 ,  -1 , "ANTI_SKID_SWITCH" ,     0 , "selector" , 11 , 0 },
    { "ANTI_SKID_SWITCH_POS1" , "NONE" ,  0 ,  0 ,  -1 , "ANTI_SKID_SWITCH" ,     1 , "selector" , 11 , 0 },
    { "DOWNLOCK_OVERRIDE"     , "NONE" ,  0 ,  0 ,  -1 , "DOWNLOCK_OVERRIDE",     1 , "momentary",  0 , 0 },
    { "ENGINE_TEMS_DATA"      , "NONE" ,  0 ,  0 ,  -1 , "ENGINE_TEMS_DATA" ,     1 , "momentary",  0 , 0 },
    { "GEAR_HORN_SILENCE"     , "NONE" ,  0 ,  0 ,  -1 , "GEAR_HORN_SILENCE",     1 , "momentary",  0 , 0 },
    { "GEAR_LEVER_POS0"       , "NONE" ,  0 ,  0 ,  -1 , "GEAR_LEVER"       ,     0 , "selector" , 12 , 0 },
    { "GEAR_LEVER_POS1"       , "NONE" ,  0 ,  0 ,  -1 , "GEAR_LEVER"       ,     1 , "selector" , 12 , 0 },
    { "LANDING_LIGHTS_LAND"   , "NONE" ,  0 ,  0 ,  -1 , "LANDING_LIGHTS"   ,     0 , "selector" , 13 , 0 },
    { "LANDING_LIGHTS_OFF"    , "NONE" ,  0 ,  0 ,  -1 , "LANDING_LIGHTS"   ,     1 , "selector" , 13 , 0 },
    { "LANDING_LIGHTS_TAXI"   , "NONE" ,  0 ,  0 ,  -1 , "LANDING_LIGHTS"   ,     2 , "selector" , 13 , 0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "AHCP_ALT_SCE",
    "AHCP_CICU",
    "AHCP_GUNPAC",
    "AHCP_HUD_DAYNIGHT",
    "AHCP_HUD_MODE",
    "AHCP_IFFCC",
    "AHCP_JTRS",
    "AHCP_LASER_ARM",
    "AHCP_MASTER_ARM",
    "AHCP_TGP",
    "ANTI_SKID_SWITCH",
    "GEAR_LEVER",
    "LANDING_LIGHTS",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[71] = {
  {nullptr, nullptr},
  {"AHCP_HUD_DAYNIGHT_POS1", &InputMappings[9]},
  {nullptr, nullptr},
  {"AHCP_HUD_MODE_POS1", &InputMappings[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_LASER_ARM_POS2", &InputMappings[19]},
  {"AHCP_ALT_SCE_POS0", &InputMappings[0]},
  {"ANTI_SKID_SWITCH_POS1", &InputMappings[26]},
  {"AHCP_HUD_DAYNIGHT_POS0", &InputMappings[8]},
  {nullptr, nullptr},
  {"AHCP_LASER_ARM_POS0", &InputMappings[17]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_TGP_POS1", &InputMappings[24]},
  {"AHCP_MASTER_ARM_POS2", &InputMappings[22]},
  {"LANDING_LIGHTS_LAND", &InputMappings[32]},
  {nullptr, nullptr},
  {"DOWNLOCK_OVERRIDE", &InputMappings[27]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_MASTER_ARM_POS1", &InputMappings[21]},
  {"AHCP_IFFCC_POS0", &InputMappings[12]},
  {"ENGINE_TEMS_DATA", &InputMappings[28]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"GEAR_LEVER_POS0", &InputMappings[30]},
  {nullptr, nullptr},
  {"AHCP_CICU_POS1", &InputMappings[4]},
  {"AHCP_MASTER_ARM_POS0", &InputMappings[20]},
  {"AHCP_CICU_POS0", &InputMappings[3]},
  {"AHCP_IFFCC_POS1", &InputMappings[13]},
  {"GEAR_LEVER_POS1", &InputMappings[31]},
  {"AHCP_ALT_SCE_POS1", &InputMappings[1]},
  {nullptr, nullptr},
  {"AHCP_IFFCC_POS2", &InputMappings[14]},
  {"LANDING_LIGHTS_OFF", &InputMappings[33]},
  {nullptr, nullptr},
  {"GEAR_HORN_SILENCE", &InputMappings[29]},
  {nullptr, nullptr},
  {"AHCP_LASER_ARM_POS1", &InputMappings[18]},
  {"AHCP_GUNPAC_POS2", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_GUNPAC_POS1", &InputMappings[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_GUNPAC_POS0", &InputMappings[5]},
  {nullptr, nullptr},
  {"AHCP_HUD_MODE_POS0", &InputMappings[10]},
  {"AHCP_TGP_POS0", &InputMappings[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_JTRS_POS1", &InputMappings[16]},
  {"LANDING_LIGHTS_TAXI", &InputMappings[34]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"AHCP_JTRS_POS0", &InputMappings[15]},
  {"AHCP_ALT_SCE_POS2", &InputMappings[2]},
  {"ANTI_SKID_SWITCH_POS0", &InputMappings[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 71;
  for (uint16_t i = 0; i < 71; ++i) {
    uint16_t idx = (startH + i >= 71) ? (startH + i - 71) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
