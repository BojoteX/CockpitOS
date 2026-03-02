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
    {0x74C2,0x0100,8,1,"APU_CONTROL_SW",CT_SELECTOR},
    {0x74C2,0x0800,11,1,"APU_READY_LT",CT_LED},
    {0x74C2,0x0600,9,2,"ENGINE_CRANK_SW",CT_SELECTOR},
    {0x740C,0x2000,13,1,"MASTER_ARM_SW",CT_SELECTOR},
    {0x740C,0x0800,11,1,"MASTER_MODE_AA",CT_SELECTOR},
    {0x740C,0x0200,9,1,"MASTER_MODE_AA_LT",CT_LED},
    {0x740C,0x1000,12,1,"MASTER_MODE_AG",CT_SELECTOR},
    {0x740C,0x0400,10,1,"MASTER_MODE_AG_LT",CT_LED},
    {0x740C,0x4000,14,1,"MC_DISCH",CT_LED},
    {0x740C,0x8000,15,1,"MC_READY",CT_LED},
    {0x74C8,0x3000,12,2,"FLIR_SW",CT_SELECTOR},
    {0x74CA,0x3800,11,7,"INS_SW",CT_SELECTOR},
    {0x74C8,0x8000,15,1,"LST_NFLR_SW",CT_SELECTOR},
    {0x74C8,0x4000,14,1,"LTD_R_SW",CT_SELECTOR},
    {0x74CA,0x0300,8,3,"RADAR_SW",CT_SELECTOR},
    {0x74CA,0x0400,10,1,"RADAR_SW_PULL",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[7]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x74C2, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[2] }, 3 },
  { 0x740C, { &DcsOutputTable[3], &DcsOutputTable[4], &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[9] }, 7 },
  { 0x74C8, { &DcsOutputTable[10], &DcsOutputTable[12], &DcsOutputTable[13] }, 3 },
  { 0x74CA, { &DcsOutputTable[11], &DcsOutputTable[14], &DcsOutputTable[15] }, 3 },
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
  { 0x74C8, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x74CA, &dcsAddressTable[3] },
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x740C, &dcsAddressTable[1] },
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74C2, &dcsAddressTable[0] },
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
    { "APU_CONTROL_SW_OFF","APU_CONTROL_SW",0,"selector",1,"OFF" },
    { "APU_CONTROL_SW_ON","APU_CONTROL_SW",1,"selector",1,"ON" },
    { "ENGINE_CRANK_SW_RIGHT","ENGINE_CRANK_SW",0,"selector",2,"RIGHT" },
    { "ENGINE_CRANK_SW_OFF","ENGINE_CRANK_SW",1,"selector",2,"OFF" },
    { "ENGINE_CRANK_SW_LEFT","ENGINE_CRANK_SW",2,"selector",2,"LEFT" },
    { "MASTER_ARM_SW_SAFE","MASTER_ARM_SW",0,"selector",3,"SAFE" },
    { "MASTER_ARM_SW_ARM","MASTER_ARM_SW",1,"selector",3,"ARM" },
    { "MASTER_MODE_AA","MASTER_MODE_AA",1,"momentary",0,"PRESS" },
    { "MASTER_MODE_AG","MASTER_MODE_AG",1,"momentary",0,"PRESS" },
    { "FLIR_SW_OFF","FLIR_SW",0,"selector",4,"OFF" },
    { "FLIR_SW_STBY","FLIR_SW",1,"selector",4,"STBY" },
    { "FLIR_SW_ON","FLIR_SW",2,"selector",4,"ON" },
    { "INS_SW_TEST","INS_SW",0,"selector",5,"TEST" },
    { "INS_SW_GB","INS_SW",1,"selector",5,"GB" },
    { "INS_SW_GYRO","INS_SW",2,"selector",5,"GYRO" },
    { "INS_SW_IFA","INS_SW",3,"selector",5,"IFA" },
    { "INS_SW_NAV","INS_SW",4,"selector",5,"NAV" },
    { "INS_SW_GND","INS_SW",5,"selector",5,"GND" },
    { "INS_SW_CV","INS_SW",6,"selector",5,"CV" },
    { "INS_SW_OFF","INS_SW",7,"selector",5,"OFF" },
    { "INS_SW_DEC","INS_SW",0,"fixed_step",0,"DEC" },
    { "INS_SW_INC","INS_SW",1,"fixed_step",0,"INC" },
    { "LST_NFLR_SW_OFF","LST_NFLR_SW",0,"selector",6,"OFF" },
    { "LST_NFLR_SW_ON","LST_NFLR_SW",1,"selector",6,"ON" },
    { "LTD_R_SW_SAFE","LTD_R_SW",0,"selector",7,"SAFE" },
    { "LTD_R_SW_ARM","LTD_R_SW",1,"selector",7,"ARM" },
    { "RADAR_SW_EMERG(PULL)","RADAR_SW",0,"selector",8,"EMERG(PULL)" },
    { "RADAR_SW_OPR","RADAR_SW",1,"selector",8,"OPR" },
    { "RADAR_SW_STBY","RADAR_SW",2,"selector",8,"STBY" },
    { "RADAR_SW_OFF","RADAR_SW",3,"selector",8,"OFF" },
    { "RADAR_SW_DEC","RADAR_SW",0,"fixed_step",0,"DEC" },
    { "RADAR_SW_INC","RADAR_SW",1,"fixed_step",0,"INC" },
    { "RADAR_SW_PULL","RADAR_SW_PULL",1,"momentary",0,"PRESS" },
    { "ENGINE_CRANK_SW_CUSTOM_PRESS","ENGINE_CRANK_SW",2,"momentary",0,"PRESS" },
    { "ENGINE_CRANK_SW_CUSTOM_2_PRESS","ENGINE_CRANK_SW",0,"momentary",0,"PRESS" },
    { "LTD_R_SW_CUSTOM_PRESS","LTD_R_SW",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[73] = {
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"LTD_R_SW", 1, &SelectorMap[25]},
  {"LTD_R_SW", 1, &SelectorMap[35]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"INS_SW", 0, &SelectorMap[12]},
  {"INS_SW", 0, &SelectorMap[20]},
  {"LTD_R_SW", 0, &SelectorMap[24]},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 0, &SelectorMap[2]},
  {"ENGINE_CRANK_SW", 0, &SelectorMap[34]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"FLIR_SW", 1, &SelectorMap[10]},
  {"LST_NFLR_SW", 0, &SelectorMap[22]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"INS_SW", 7, &SelectorMap[19]},
  {"INS_SW", 4, &SelectorMap[16]},
  {"APU_CONTROL_SW", 1, &SelectorMap[1]},
  {"INS_SW", 3, &SelectorMap[15]},
  {"MASTER_ARM_SW", 1, &SelectorMap[6]},
  {"INS_SW", 2, &SelectorMap[14]},
  {"INS_SW", 6, &SelectorMap[18]},
  {nullptr, 0, nullptr},
  {"INS_SW", 5, &SelectorMap[17]},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 2, &SelectorMap[4]},
  {"INS_SW", 1, &SelectorMap[13]},
  {"INS_SW", 1, &SelectorMap[21]},
  {"ENGINE_CRANK_SW", 2, &SelectorMap[33]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"MASTER_MODE_AA", 1, &SelectorMap[7]},
  {"MASTER_MODE_AG", 1, &SelectorMap[8]},
  {nullptr, 0, nullptr},
  {"APU_CONTROL_SW", 0, &SelectorMap[0]},
  {"RADAR_SW_PULL", 1, &SelectorMap[32]},
  {nullptr, 0, nullptr},
  {"RADAR_SW", 1, &SelectorMap[27]},
  {"RADAR_SW", 1, &SelectorMap[31]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"FLIR_SW", 2, &SelectorMap[11]},
  {"RADAR_SW", 2, &SelectorMap[28]},
  {nullptr, 0, nullptr},
  {"RADAR_SW", 0, &SelectorMap[26]},
  {"RADAR_SW", 0, &SelectorMap[30]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"RADAR_SW", 3, &SelectorMap[29]},
  {"ENGINE_CRANK_SW", 1, &SelectorMap[3]},
  {"MASTER_ARM_SW", 0, &SelectorMap[5]},
  {"FLIR_SW", 0, &SelectorMap[9]},
  {"LST_NFLR_SW", 1, &SelectorMap[23]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
};

// Composite hash: labelHash(dcsCommand) ^ (value * 7919)
inline const SelectorEntry* findSelectorByDcsAndValue(const char* dcsCommand, uint16_t value) {
  uint16_t startH = (labelHash(dcsCommand) ^ (value * 7919u)) % 73;
  for (uint16_t i = 0; i < 73; ++i) {
    uint16_t idx = (startH + i >= 73) ? (startH + i - 73) : (startH + i);
    const auto& entry = selectorHashTable[idx];
    if (!entry.dcsCommand) return nullptr;
    if (entry.value == value && strcmp(entry.dcsCommand, dcsCommand) == 0) return entry.entry;
  }
  return nullptr;
}


// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "APU_CONTROL_SW", 0, 0, true, 1, 0,   0, false, 0, {0}, {0}, 0 },
    { "ENGINE_CRANK_SW", 0, 0, true, 2, 0,   0, false, 0, {0}, {0}, 0 },
    { "FLIR_SW", 0, 0, true, 4, 0,   0, false, 0, {0}, {0}, 0 },
    { "INS_SW", 0, 0, true, 5, 0,   0, false, 0, {0}, {0}, 0 },
    { "LST_NFLR_SW", 0, 0, true, 6, 0,   0, false, 0, {0}, {0}, 0 },
    { "LTD_R_SW", 0, 0, true, 7, 0,   0, false, 0, {0}, {0}, 0 },
    { "MASTER_ARM_SW", 0, 0, true, 3, 0,   0, false, 0, {0}, {0}, 0 },
    { "MASTER_MODE_AA", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "MASTER_MODE_AG", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "RADAR_SW", 0, 0, true, 8, 0,   0, false, 0, {0}, {0}, 0 },
    { "RADAR_SW_PULL", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
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

// No tracked metadata fields found
struct MetadataState { const char* label; uint16_t value; };
static MetadataState metadataStates[] __attribute__((unused)) = {};
static const size_t numMetadataStates __attribute__((unused)) = 0;
inline MetadataState* findMetadataState(const char*) { return nullptr; }
