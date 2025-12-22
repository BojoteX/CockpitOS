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
    { "UFC_0"            , "GPIO" , 14 ,  0 ,   1 , "UFC_0"         ,     1 , "momentary"    ,  0 },
    { "UFC_1"            , "GPIO" ,  8 ,  0 ,   2 , "UFC_1"         ,     1 , "momentary"    ,  0 },
    { "UFC_2"            , "GPIO" , 11 ,  0 ,   3 , "UFC_2"         ,     1 , "momentary"    ,  0 },
    { "UFC_3"            , "GPIO" , 13 ,  0 ,   4 , "UFC_3"         ,     1 , "momentary"    ,  0 },
    { "UFC_4"            , "GPIO" , 23 ,  0 ,   5 , "UFC_4"         ,     1 , "momentary"    ,  0 },
    { "UFC_5"            , "GPIO" , 22 ,  0 ,   6 , "UFC_5"         ,     1 , "momentary"    ,  0 },
    { "UFC_6"            , "GPIO" , 12 ,  0 ,   7 , "UFC_6"         ,     1 , "momentary"    ,  0 },
    { "UFC_7"            , "GPIO" , 25 ,  0 ,   8 , "UFC_7"         ,     1 , "momentary"    ,  0 },
    { "UFC_8"            , "GPIO" ,  0 ,  0 ,  11 , "UFC_8"         ,     1 , "momentary"    ,  0 },
    { "UFC_9"            , "GPIO" ,  9 ,  0 ,   9 , "UFC_9"         ,     1 , "momentary"    ,  0 },
    { "UFC_ADF_2"        , "NONE" ,  0 ,  0 ,  -1 , "UFC_ADF"       ,     0 , "selector"     ,  8 },
    { "UFC_ADF_OFF"      , "NONE" ,  0 ,  0 ,  -1 , "UFC_ADF"       ,     1 , "selector"     ,  8 },
    { "UFC_ADF_1"        , "NONE" ,  0 ,  0 ,  -1 , "UFC_ADF"       ,     2 , "selector"     ,  8 },
    { "UFC_AP"           , "NONE" ,  0 ,  0 ,  -1 , "UFC_AP"        ,     1 , "momentary"    ,  0 },
    { "UFC_BCN"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_BCN"       ,     1 , "momentary"    ,  0 },
    { "UFC_BRT"          , "GPIO" ,  5 ,  0 ,  -1 , "UFC_BRT"       , 65535 , "analog"       ,  0 },
    { "UFC_BRT_DEC"      , "NONE" ,  0 ,  0 ,  -1 , "UFC_BRT"       ,     0 , "variable_step",  0 },
    { "UFC_BRT_INC"      , "NONE" ,  0 ,  0 ,  -1 , "UFC_BRT"       ,     1 , "variable_step",  0 },
    { "UFC_CLR"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_CLR"       ,     1 , "momentary"    ,  0 },
    { "UFC_COMM1_PULL"   , "NONE" ,  0 ,  0 ,  -1 , "UFC_COMM1_PULL",     1 , "momentary"    ,  0 },
    { "UFC_COMM1_VOL"    , "GPIO" ,  2 ,  0 ,  -1 , "UFC_COMM1_VOL" , 65535 , "analog"       ,  0 },
    { "UFC_COMM1_VOL_DEC", "NONE" ,  0 ,  0 ,  -1 , "UFC_COMM1_VOL" ,     0 , "variable_step",  0 },
    { "UFC_COMM1_VOL_INC", "NONE" ,  0 ,  0 ,  -1 , "UFC_COMM1_VOL" ,     1 , "variable_step",  0 },
    { "UFC_COMM2_PULL"   , "GPIO" , 10 ,  0 ,  10 , "UFC_COMM2_PULL",     1 , "momentary"    ,  0 },
    { "UFC_COMM2_VOL"    , "NONE" ,  0 ,  0 ,  -1 , "UFC_COMM2_VOL" , 65535 , "analog"       ,  0 },
    { "UFC_COMM2_VOL_DEC", "NONE" ,  0 ,  0 ,  -1 , "UFC_COMM2_VOL" ,     0 , "variable_step",  0 },
    { "UFC_COMM2_VOL_INC", "NONE" ,  0 ,  0 ,  -1 , "UFC_COMM2_VOL" ,     1 , "variable_step",  0 },
    { "UFC_DL"           , "NONE" ,  0 ,  0 ,  -1 , "UFC_DL"        ,     1 , "momentary"    ,  0 },
    { "UFC_EMCON"        , "NONE" ,  0 ,  0 ,  -1 , "UFC_EMCON"     ,     1 , "momentary"    ,  0 },
    { "UFC_ENT"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_ENT"       ,     1 , "momentary"    ,  0 },
    { "UFC_IFF"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_IFF"       ,     1 , "momentary"    ,  0 },
    { "UFC_ILS"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_ILS"       ,     1 , "momentary"    ,  0 },
    { "UFC_IP"           , "NONE" ,  0 ,  0 ,  -1 , "UFC_IP"        ,     1 , "momentary"    ,  0 },
    { "UFC_ONOFF"        , "NONE" ,  0 ,  0 ,  -1 , "UFC_ONOFF"     ,     1 , "momentary"    ,  0 },
    { "UFC_OS1"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_OS1"       ,     1 , "momentary"    ,  0 },
    { "UFC_OS2"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_OS2"       ,     1 , "momentary"    ,  0 },
    { "UFC_OS3"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_OS3"       ,     1 , "momentary"    ,  0 },
    { "UFC_OS4"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_OS4"       ,     1 , "momentary"    ,  0 },
    { "UFC_OS5"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_OS5"       ,     1 , "momentary"    ,  0 },
    { "UFC_TCN"          , "NONE" ,  0 ,  0 ,  -1 , "UFC_TCN"       ,     1 , "momentary"    ,  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "UFC_ADF",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[83] = {
  {"UFC_IP", &InputMappings[32]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_TCN", &InputMappings[39]},
  {"UFC_COMM1_VOL", &InputMappings[20]},
  {"UFC_COMM2_PULL", &InputMappings[23]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_COMM1_VOL_INC", &InputMappings[22]},
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
  {"UFC_COMM2_VOL", &InputMappings[24]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_COMM1_PULL", &InputMappings[19]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_ILS", &InputMappings[31]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_ADF_1", &InputMappings[12]},
  {"UFC_ADF_2", &InputMappings[10]},
  {"UFC_COMM2_VOL_DEC", &InputMappings[25]},
  {"UFC_ONOFF", &InputMappings[33]},
  {nullptr, nullptr},
  {"UFC_OS1", &InputMappings[34]},
  {"UFC_0", &InputMappings[0]},
  {"UFC_1", &InputMappings[1]},
  {"UFC_2", &InputMappings[2]},
  {"UFC_3", &InputMappings[3]},
  {"UFC_4", &InputMappings[4]},
  {"UFC_5", &InputMappings[5]},
  {"UFC_6", &InputMappings[6]},
  {"UFC_7", &InputMappings[7]},
  {"UFC_8", &InputMappings[8]},
  {"UFC_9", &InputMappings[9]},
  {"UFC_BRT_DEC", &InputMappings[16]},
  {"UFC_COMM2_VOL_INC", &InputMappings[26]},
  {"UFC_OS2", &InputMappings[35]},
  {"UFC_CLR", &InputMappings[18]},
  {"UFC_EMCON", &InputMappings[28]},
  {"UFC_OS3", &InputMappings[36]},
  {"UFC_BRT_INC", &InputMappings[17]},
  {"UFC_OS4", &InputMappings[37]},
  {"UFC_ENT", &InputMappings[29]},
  {"UFC_ADF_OFF", &InputMappings[11]},
  {"UFC_OS5", &InputMappings[38]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_AP", &InputMappings[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_IFF", &InputMappings[30]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_BCN", &InputMappings[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_BRT", &InputMappings[15]},
  {"UFC_DL", &InputMappings[27]},
  {"UFC_COMM1_VOL_DEC", &InputMappings[21]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);


// Preserve existing signature
constexpr uint16_t inputHash(const char* s) { return labelHash(s); }

inline const InputMapping* findInputByLabel(const char* label) {
  uint16_t startH = inputHash(label) % 83;
  for (uint16_t i = 0; i < 83; ++i) {
    uint16_t idx = (startH + i >= 83) ? (startH + i - 83) : (startH + i);
    const auto& entry = inputHashTable[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.mapping;
  }
  return nullptr;
}
