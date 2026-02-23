// Auto-generated DCSBIOS Bridge Data (JSON‑only) - DO NOT EDIT
#pragma once

#define DCSBIOS_ACFT_NAME "FA-18C_hornet"
static constexpr const char* DCSBIOS_AIRCRAFT_NAME = "FA-18C_hornet";

enum ControlType : uint8_t {
  CT_LED,
  CT_ANALOG,
  CT_GAUGE,
  CT_SELECTOR,
  CT_DISPLAY,
  CT_METADATA
};

// --- Command History Tracking Struct ---
struct CommandHistoryEntry {
    const char*     label;
    uint16_t        lastValue;
    unsigned long   lastSendTime;
    bool            isSelector;
    uint16_t        group;
    uint16_t        pendingValue;
    unsigned long   lastChangeTime;
    bool            hasPending;
    uint16_t        maxPositions;
    uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
    uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
    unsigned long   lastHidSendTime;
};

struct DcsOutputEntry { uint16_t addr, mask; uint8_t shift; uint16_t max_value; const char* label; ControlType controlType; };
static const DcsOutputEntry DcsOutputTable[] = {
    {0x7576,0xFFFF,0,65535,"EXT_FORMATION_LIGHTS",CT_METADATA},
    {0x7586,0xFFFF,0,65535,"EXT_HOOK",CT_METADATA},
    {0x75AE,0xFFFF,0,65535,"EXT_LAUNCH_BAR",CT_METADATA},
    {0x757A,0xFFFF,0,65535,"EXT_NOZZLE_POS_L",CT_METADATA},
    {0x7578,0xFFFF,0,65535,"EXT_NOZZLE_POS_R",CT_METADATA},
    {0x74D6,0x0400,10,1,"EXT_POSITION_LIGHT_LEFT",CT_METADATA},
    {0x74D6,0x0800,11,1,"EXT_POSITION_LIGHT_RIGHT",CT_METADATA},
    {0x7574,0xFFFF,0,65535,"EXT_REFUEL_PROBE",CT_METADATA},
    {0x74D6,0x0200,9,1,"EXT_REFUEL_PROBE_LIGHT",CT_METADATA},
    {0x756E,0xFFFF,0,65535,"EXT_SPEED_BRAKE",CT_METADATA},
    {0x7572,0xFFFF,0,65535,"EXT_STAIR",CT_METADATA},
    {0x74D6,0x2000,13,1,"EXT_STROBE_LIGHTS",CT_METADATA},
    {0x74D6,0x1000,12,1,"EXT_TAIL_LIGHT",CT_METADATA},
    {0x7570,0xFFFF,0,65535,"EXT_WING_FOLDING",CT_METADATA},
    {0x74D8,0x0100,8,1,"EXT_WOW_LEFT",CT_METADATA},
    {0x74D6,0x4000,14,1,"EXT_WOW_NOSE",CT_METADATA},
    {0x74D6,0x8000,15,1,"EXT_WOW_RIGHT",CT_METADATA},
    {0x7456,0xFFFF,0,65535,"HMD_OFF_BRT",CT_ANALOG},
    {0x742A,0xC000,14,2,"IR_COOL_SW",CT_SELECTOR},
    {0x742A,0x0800,11,1,"SPIN_LT",CT_LED},
    {0x742A,0x1000,12,1,"SPIN_RECOVERY_COVER",CT_SELECTOR},
    {0x742A,0x2000,13,1,"SPIN_RECOVERY_SW",CT_SELECTOR},
    {0x740C,0x2000,13,1,"MASTER_ARM_SW",CT_SELECTOR},
    {0x740C,0x0800,11,1,"MASTER_MODE_AA",CT_SELECTOR},
    {0x740C,0x0200,9,1,"MASTER_MODE_AA_LT",CT_LED},
    {0x740C,0x1000,12,1,"MASTER_MODE_AG",CT_SELECTOR},
    {0x740C,0x0400,10,1,"MASTER_MODE_AG_LT",CT_LED},
    {0x740C,0x4000,14,1,"MC_DISCH",CT_LED},
    {0x740C,0x8000,15,1,"MC_READY",CT_LED},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[7]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x7576, { &DcsOutputTable[0] }, 1 },
  { 0x7586, { &DcsOutputTable[1] }, 1 },
  { 0x75AE, { &DcsOutputTable[2] }, 1 },
  { 0x757A, { &DcsOutputTable[3] }, 1 },
  { 0x7578, { &DcsOutputTable[4] }, 1 },
  { 0x74D6, { &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[8], &DcsOutputTable[11], &DcsOutputTable[12], &DcsOutputTable[15], &DcsOutputTable[16] }, 7 },
  { 0x7574, { &DcsOutputTable[7] }, 1 },
  { 0x756E, { &DcsOutputTable[9] }, 1 },
  { 0x7572, { &DcsOutputTable[10] }, 1 },
  { 0x7570, { &DcsOutputTable[13] }, 1 },
  { 0x74D8, { &DcsOutputTable[14] }, 1 },
  { 0x7456, { &DcsOutputTable[17] }, 1 },
  { 0x742A, { &DcsOutputTable[18], &DcsOutputTable[19], &DcsOutputTable[20], &DcsOutputTable[21] }, 4 },
  { 0x740C, { &DcsOutputTable[22], &DcsOutputTable[23], &DcsOutputTable[24], &DcsOutputTable[25], &DcsOutputTable[26], &DcsOutputTable[27], &DcsOutputTable[28] }, 7 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x742A, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x756E, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x7570, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x7572, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x7574, &dcsAddressTable[6] },
  { 0x74D6, &dcsAddressTable[5] },
  { 0x7576, &dcsAddressTable[0] },
  { 0x74D8, &dcsAddressTable[10] },
  { 0x7578, &dcsAddressTable[4] },
  { 0x75AE, &dcsAddressTable[2] },
  { 0x757A, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x740C, &dcsAddressTable[13] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7586, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7456, &dcsAddressTable[11] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
};

// Simple address hash (modulo)
constexpr uint16_t addrHash(uint16_t addr) {
  return addr % 53;
}

inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {
  uint16_t startH = addrHash(addr);
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = dcsAddressHashTable[idx];
    if (entry.addr == 0xFFFF) return nullptr;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "HMD_OFF_BRT","HMD_OFF_BRT",65535,"analog",0,"LEVEL" },
    { "HMD_OFF_BRT_DEC","HMD_OFF_BRT",0,"variable_step",0,"DEC" },
    { "HMD_OFF_BRT_INC","HMD_OFF_BRT",1,"variable_step",0,"INC" },
    { "IR_COOL_SW_OFF","IR_COOL_SW",0,"selector",1,"OFF" },
    { "IR_COOL_SW_NORM","IR_COOL_SW",1,"selector",1,"NORM" },
    { "IR_COOL_SW_ORIDE","IR_COOL_SW",2,"selector",1,"ORIDE" },
    { "SPIN_RECOVERY_COVER","SPIN_RECOVERY_COVER",1,"momentary",0,"OPEN" },
    { "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_SW",0,"selector",2,"NORM" },
    { "SPIN_RECOVERY_SW_RCVY","SPIN_RECOVERY_SW",1,"selector",2,"RCVY" },
    { "MASTER_ARM_SW_SAFE","MASTER_ARM_SW",0,"selector",3,"SAFE" },
    { "MASTER_ARM_SW_ARM","MASTER_ARM_SW",1,"selector",3,"ARM" },
    { "MASTER_MODE_AA","MASTER_MODE_AA",1,"momentary",0,"PRESS" },
    { "MASTER_MODE_AG","MASTER_MODE_AG",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[53] = {
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"HMD_OFF_BRT", 65535, &SelectorMap[0]},
  {nullptr, 0, nullptr},
  {"SPIN_RECOVERY_SW", 0, &SelectorMap[7]},
  {"MASTER_ARM_SW", 1, &SelectorMap[10]},
  {"MASTER_MODE_AA", 1, &SelectorMap[11]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"IR_COOL_SW", 1, &SelectorMap[4]},
  {"MASTER_ARM_SW", 0, &SelectorMap[9]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"SPIN_RECOVERY_COVER", 1, &SelectorMap[6]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"IR_COOL_SW", 2, &SelectorMap[5]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"HMD_OFF_BRT", 0, &SelectorMap[1]},
  {"IR_COOL_SW", 0, &SelectorMap[3]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"HMD_OFF_BRT", 1, &SelectorMap[2]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"MASTER_MODE_AG", 1, &SelectorMap[12]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"SPIN_RECOVERY_SW", 1, &SelectorMap[8]},
  {nullptr, 0, nullptr},
};

// Composite hash: labelHash(dcsCommand) ^ (value * 7919)
inline const SelectorEntry* findSelectorByDcsAndValue(const char* dcsCommand, uint16_t value) {
  uint16_t startH = (labelHash(dcsCommand) ^ (value * 7919u)) % 53;
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = selectorHashTable[idx];
    if (!entry.dcsCommand) return nullptr;
    if (entry.value == value && strcmp(entry.dcsCommand, dcsCommand) == 0) return entry.entry;
  }
  return nullptr;
}


// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "HMD_OFF_BRT", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "IR_COOL_SW", 0, 0, true, 1, 0,   0, false, 0, {0}, {0}, 0 },
    { "MASTER_ARM_SW", 0, 0, true, 3, 0,   0, false, 0, {0}, {0}, 0 },
    { "MASTER_MODE_AA", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "MASTER_MODE_AG", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "SPIN_RECOVERY_COVER", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "SPIN_RECOVERY_SW", 0, 0, true, 2, 0,   0, false, 0, {0}, {0}, 0 },
};
static const size_t commandHistorySize = sizeof(commandHistory)/sizeof(CommandHistoryEntry);

// --- Auto-generated display string field grouping ---
struct DisplayFieldDef {
    const char* panel;
    const char* label;
    uint16_t base_addr;
    uint8_t  length;
};
static const DisplayFieldDef displayFields[] = {
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[2] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 2;
  for (uint16_t i = 0; i < 2; ++i) {
    uint16_t idx = (startH + i >= 2) ? (startH + i - 2) : (startH + i);
    const auto& entry = displayFieldsByLabel[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.def;
  }
  return nullptr;
}


// --- Auto-generated tracked metadata fields ---
struct MetadataState {
    const char* label;
    uint16_t    value;
};
static MetadataState metadataStates[] = {
    { "EXT_FORMATION_LIGHTS", 0 }, // External Aircraft Model: Formation Lights (light green)
    { "EXT_HOOK", 0 }, // External Aircraft Model: Hook Position
    { "EXT_LAUNCH_BAR", 0 }, // External Aircraft Model: Launch Bar position
    { "EXT_NOZZLE_POS_L", 0 }, // External Aircraft Model: Left Nozzle Position
    { "EXT_NOZZLE_POS_R", 0 }, // External Aircraft Model: Right Nozzle Position
    { "EXT_POSITION_LIGHT_LEFT", 0 }, // External Aircraft Model: Left Position Light (red)
    { "EXT_POSITION_LIGHT_RIGHT", 0 }, // External Aircraft Model: Right Position Light (green)
    { "EXT_REFUEL_PROBE", 0 }, // External Aircraft Model: Refuel Probe
    { "EXT_REFUEL_PROBE_LIGHT", 0 }, // External Aircraft Model: Refuel Probe Light (white)
    { "EXT_SPEED_BRAKE", 0 }, // External Aircraft Model: Speed Brake
    { "EXT_STAIR", 0 }, // External Aircraft Model: Stair
    { "EXT_STROBE_LIGHTS", 0 }, // External Aircraft Model: Strobe Lights (red)
    { "EXT_TAIL_LIGHT", 0 }, // External Aircraft Model: Tail Light (white)
    { "EXT_WING_FOLDING", 0 }, // External Aircraft Model: Wing Folding
    { "EXT_WOW_LEFT", 0 }, // External Aircraft Model: Weight ON Wheels Left Gear
    { "EXT_WOW_NOSE", 0 }, // External Aircraft Model: Weight ON Wheels Nose Gear
    { "EXT_WOW_RIGHT", 0 }, // External Aircraft Model: Weight ON Wheels Right Gear
};
static const size_t numMetadataStates = sizeof(metadataStates)/sizeof(metadataStates[0]);

struct MetadataHashEntry { const char* label; MetadataState* state; };
static MetadataHashEntry metadataHashTable[37] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_WOW_NOSE", &metadataStates[15]},
  {"EXT_POSITION_LIGHT_LEFT", &metadataStates[5]},
  {"EXT_NOZZLE_POS_L", &metadataStates[3]},
  {"EXT_POSITION_LIGHT_RIGHT", &metadataStates[6]},
  {"EXT_WOW_LEFT", &metadataStates[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_SPEED_BRAKE", &metadataStates[9]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_HOOK", &metadataStates[1]},
  {"EXT_WOW_RIGHT", &metadataStates[16]},
  {nullptr, nullptr},
  {"EXT_NOZZLE_POS_R", &metadataStates[4]},
  {nullptr, nullptr},
  {"EXT_REFUEL_PROBE", &metadataStates[7]},
  {"EXT_FORMATION_LIGHTS", &metadataStates[0]},
  {"EXT_LAUNCH_BAR", &metadataStates[2]},
  {nullptr, nullptr},
  {"EXT_WING_FOLDING", &metadataStates[13]},
  {nullptr, nullptr},
  {"EXT_STROBE_LIGHTS", &metadataStates[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_REFUEL_PROBE_LIGHT", &metadataStates[8]},
  {nullptr, nullptr},
  {"EXT_STAIR", &metadataStates[10]},
  {"EXT_TAIL_LIGHT", &metadataStates[12]},
  {nullptr, nullptr},
};

constexpr uint16_t metadataHash(const char* s) { return labelHash(s); }

inline MetadataState* findMetadataState(const char* label) {
    uint16_t startH = metadataHash(label) % 37;
    for (uint16_t i = 0; i < 37; ++i) {
        uint16_t idx = (startH + i >= 37) ? (startH + i - 37) : (startH + i);
        const auto& entry = metadataHashTable[idx];
        if (!entry.label) return nullptr;
        if (strcmp(entry.label, label) == 0) return entry.state;
    }
    return nullptr;
}
