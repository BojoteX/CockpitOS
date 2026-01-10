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
    uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
    uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
    unsigned long   lastHidSendTime;
};

struct DcsOutputEntry { uint16_t addr, mask; uint8_t shift; uint16_t max_value; const char* label; ControlType controlType; };
static const DcsOutputEntry DcsOutputTable[] = {
    {0x7466,0x0001,0,1,"FLP_LG_FLAPS_LT",CT_LED},
    {0x7430,0x8000,15,1,"FLP_LG_FULL_FLAPS_LT",CT_LED},
    {0x7430,0x4000,14,1,"FLP_LG_HALF_FLAPS_LT",CT_LED},
    {0x7430,0x1000,12,1,"FLP_LG_LEFT_GEAR_LT",CT_LED},
    {0x7430,0x0800,11,1,"FLP_LG_NOSE_GEAR_LT",CT_LED},
    {0x7430,0x2000,13,1,"FLP_LG_RIGHT_GEAR_LT",CT_LED},
    {0x742C,0x8000,15,1,"SJ_CTR",CT_SELECTOR},
    {0x742E,0x4000,14,1,"SJ_CTR_LT",CT_LED},
    {0x742E,0x0400,10,1,"SJ_LI",CT_SELECTOR},
    {0x742E,0x8000,15,1,"SJ_LI_LT",CT_LED},
    {0x742E,0x0800,11,1,"SJ_LO",CT_SELECTOR},
    {0x7430,0x0100,8,1,"SJ_LO_LT",CT_LED},
    {0x742E,0x1000,12,1,"SJ_RI",CT_SELECTOR},
    {0x7430,0x0200,9,1,"SJ_RI_LT",CT_LED},
    {0x742E,0x2000,13,1,"SJ_RO",CT_SELECTOR},
    {0x7430,0x0400,10,1,"SJ_RO_LT",CT_LED},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[8]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x7466, { &DcsOutputTable[0] }, 1 },
  { 0x7430, { &DcsOutputTable[1], &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[4], &DcsOutputTable[5], &DcsOutputTable[11], &DcsOutputTable[13], &DcsOutputTable[15] }, 8 },
  { 0x742C, { &DcsOutputTable[6] }, 1 },
  { 0x742E, { &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[9], &DcsOutputTable[10], &DcsOutputTable[12], &DcsOutputTable[14] }, 6 },
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x742C, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x742E, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x7430, &dcsAddressTable[1] },
  { 0x7466, &dcsAddressTable[0] },
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
    if (entry.addr == 0xFFFF) continue;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "SJ_CTR","SJ_CTR",1,"momentary",0,"PRESS" },
    { "SJ_LI","SJ_LI",1,"momentary",0,"PRESS" },
    { "SJ_LO","SJ_LO",1,"momentary",0,"PRESS" },
    { "SJ_RI","SJ_RI",1,"momentary",0,"PRESS" },
    { "SJ_RO","SJ_RO",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "SJ_CTR", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SJ_LI", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SJ_LO", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SJ_RI", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SJ_RO", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.def;
  }
  return nullptr;
}

// No tracked metadata fields found
struct MetadataState { const char* label; uint16_t value; };
static MetadataState metadataStates[] = {};
static const size_t numMetadataStates = 0;
inline MetadataState* findMetadataState(const char*) { return nullptr; }
