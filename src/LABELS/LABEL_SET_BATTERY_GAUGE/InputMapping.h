// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE
#pragma once

struct InputMapping {
    const char* label;        // Unique selector label
    const char* source;       // Hardware source identifier
    int8_t     port;         // Port index
    int8_t     bit;          // Bit position
    int8_t      hidId;        // HID usage ID
    const char* oride_label;  // Override command label (dcsCommand)
    uint16_t    oride_value;  // Override command value (value)
    const char* controlType;  // Control type, e.g., "selector"
    uint16_t    group;        // Group ID for exclusive selectors
};

//  label                       source     port bit hidId  DCSCommand           value   Type        group
static const InputMapping InputMappings[] = {
    { "BATTERY_SW_ON"          , "PCA_0x27" ,  1 ,  6 ,   1 , "BATTERY_SW"             ,     2 , "selector" ,  1 },
    { "BATTERY_SW_OFF"         , "PCA_0x27" ,  1 , -1 ,   2 , "BATTERY_SW"             ,     1 , "selector" ,  1 },
    { "BATTERY_SW_ORIDE"       , "PCA_0x27" ,  1 ,  7 ,   3 , "BATTERY_SW"             ,     0 , "selector" ,  1 },
    { "L_GEN_SW_NORM"          , "PCA_0x00" ,  0 ,  0 ,  -1 , "L_GEN_SW"               ,     1 , "selector" ,  2 },
    { "L_GEN_SW_OFF"           , "PCA_0x00" ,  0 , -1 ,  -1 , "L_GEN_SW"               ,     0 , "selector" ,  2 },
    { "R_GEN_SW_NORM"          , "PCA_0x27" ,  0 ,  0 ,  -1 , "R_GEN_SW"               ,     1 , "selector" ,  3 },
    { "R_GEN_SW_OFF"           , "PCA_0x27" ,  0 , -1 ,  -1 , "R_GEN_SW"               ,     0 , "selector" ,  3 },
    { "MASTER_CAUTION_RESET_SW", "PCA_0x00" ,  0 ,  0 ,  -1 , "MASTER_CAUTION_RESET_SW",     1 , "momentary",  0 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// Auto-generated: selector DCS labels with group > 0 (panel sync)
static const char* const TrackedSelectorLabels[] = {
    "BATTERY_SW",
    "L_GEN_SW",
    "R_GEN_SW",
};
static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);


// Static hash lookup table for InputMappings[]
struct InputHashEntry { const char* label; const InputMapping* mapping; };
static const InputHashEntry inputHashTable[53] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"MASTER_CAUTION_RESET_SW", &InputMappings[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"L_GEN_SW_OFF", &InputMappings[4]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"BATTERY_SW_OFF", &InputMappings[1]},
  {"BATTERY_SW_ON", &InputMappings[0]},
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
  {"BATTERY_SW_ORIDE", &InputMappings[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"L_GEN_SW_NORM", &InputMappings[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"R_GEN_SW_NORM", &InputMappings[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"R_GEN_SW_OFF", &InputMappings[6]},
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
