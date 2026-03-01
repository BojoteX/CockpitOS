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
    {0x755E,0xFFFF,0,65535,"CHART_INT_LT",CT_GAUGE},
    {0x7558,0xFFFF,0,65535,"CONSOLE_INT_LT",CT_GAUGE},
    {0x74D4,0x2000,13,1,"EMERG_INSTR_INT_LT",CT_LED},
    {0x74D4,0x4000,14,1,"ENG_INSTR_INT_LT",CT_LED},
    {0x755A,0xFFFF,0,65535,"FLOOD_INT_LT",CT_GAUGE},
    {0x7566,0xFFFF,0,65535,"IFEI_BTN_INT_LT",CT_GAUGE},
    {0x7564,0xFFFF,0,65535,"IFEI_DISP_INT_LT",CT_GAUGE},
    {0x7560,0xFFFF,0,65535,"INSTR_INT_LT",CT_GAUGE},
    {0x755C,0xFFFF,0,65535,"NVG_FLOOD_INT_LT",CT_GAUGE},
    {0x7562,0xFFFF,0,65535,"STBY_COMPASS_INT_LT",CT_GAUGE},
    {0x74CC,0x8000,15,1,"CB_FCS_CHAN3",CT_SELECTOR},
    {0x74CE,0x0400,10,1,"CB_FCS_CHAN4",CT_SELECTOR},
    {0x74CE,0x0800,11,1,"CB_HOOOK",CT_SELECTOR},
    {0x74CE,0x1000,12,1,"CB_LG",CT_SELECTOR},
    {0x74CE,0x2000,13,1,"FCS_BIT_SW",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[4]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x755E, { &DcsOutputTable[0] }, 1 },
  { 0x7558, { &DcsOutputTable[1] }, 1 },
  { 0x74D4, { &DcsOutputTable[2], &DcsOutputTable[3] }, 2 },
  { 0x755A, { &DcsOutputTable[4] }, 1 },
  { 0x7566, { &DcsOutputTable[5] }, 1 },
  { 0x7564, { &DcsOutputTable[6] }, 1 },
  { 0x7560, { &DcsOutputTable[7] }, 1 },
  { 0x755C, { &DcsOutputTable[8] }, 1 },
  { 0x7562, { &DcsOutputTable[9] }, 1 },
  { 0x74CC, { &DcsOutputTable[10] }, 1 },
  { 0x74CE, { &DcsOutputTable[11], &DcsOutputTable[12], &DcsOutputTable[13], &DcsOutputTable[14] }, 4 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  {0xFFFF, nullptr},
  { 0x7564, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x7566, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74CC, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x74CE, &dcsAddressTable[10] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74D4, &dcsAddressTable[2] },
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7558, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  { 0x755A, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x755C, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x755E, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  { 0x7560, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x7562, &dcsAddressTable[8] },
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
    { "CB_FCS_CHAN3","CB_FCS_CHAN3",1,"momentary",0,"PRESS" },
    { "CB_FCS_CHAN4","CB_FCS_CHAN4",1,"momentary",0,"PRESS" },
    { "CB_HOOOK","CB_HOOOK",1,"momentary",0,"PRESS" },
    { "CB_LG","CB_LG",1,"momentary",0,"PRESS" },
    { "FCS_BIT_SW_PRESS","FCS_BIT_SW",0,"selector",1,"PRESS" },
    { "FCS_BIT_SW_RELEASE","FCS_BIT_SW",1,"selector",1,"RELEASE" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[53] = {
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"CB_LG", 1, &SelectorMap[3]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"FCS_BIT_SW", 0, &SelectorMap[4]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"CB_FCS_CHAN3", 1, &SelectorMap[0]},
  {"CB_FCS_CHAN4", 1, &SelectorMap[1]},
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
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"FCS_BIT_SW", 1, &SelectorMap[5]},
  {nullptr, 0, nullptr},
  {"CB_HOOOK", 1, &SelectorMap[2]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
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
    { "CB_FCS_CHAN3", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "CB_FCS_CHAN4", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "CB_HOOOK", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "CB_LG", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "FCS_BIT_SW", 0, 0, true, 1, 0,   0, false, 0, {0}, {0}, 0 },
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
