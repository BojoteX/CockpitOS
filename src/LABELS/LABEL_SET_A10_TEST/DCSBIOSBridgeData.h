// Auto-generated DCSBIOS Bridge Data (JSON‑only) - DO NOT EDIT
#pragma once

#define DCSBIOS_ACFT_NAME "A-10C"
static constexpr const char* DCSBIOS_AIRCRAFT_NAME = "A-10C";

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
    {0x10E8,0x0600,9,2,"AHCP_ALT_SCE",CT_SELECTOR},
    {0x10E8,0x2000,13,1,"AHCP_CICU",CT_SELECTOR},
    {0x10E8,0x0030,4,2,"AHCP_GUNPAC",CT_SELECTOR},
    {0x10E8,0x0800,11,1,"AHCP_HUD_DAYNIGHT",CT_SELECTOR},
    {0x10E8,0x1000,12,1,"AHCP_HUD_MODE",CT_SELECTOR},
    {0x10EE,0x0003,0,2,"AHCP_IFFCC",CT_SELECTOR},
    {0x10E8,0x4000,14,1,"AHCP_JTRS",CT_SELECTOR},
    {0x10E8,0x00C0,6,2,"AHCP_LASER_ARM",CT_SELECTOR},
    {0x10E8,0x000C,2,2,"AHCP_MASTER_ARM",CT_SELECTOR},
    {0x10E8,0x0100,8,1,"AHCP_TGP",CT_SELECTOR},
    {0x1110,0x0080,7,1,"ANTI_SKID_SWITCH",CT_SELECTOR},
    {0x1110,0x0010,4,1,"DOWNLOCK_OVERRIDE",CT_SELECTOR},
    {0x1108,0x0010,4,1,"ENGINE_TEMS_DATA",CT_SELECTOR},
    {0x10A0,0xFFFF,0,65535,"FLAP_POS",CT_GAUGE},
    {0x10F2,0x0010,4,1,"GEAR_HORN_SILENCE",CT_SELECTOR},
    {0x1110,0x0008,3,1,"GEAR_LEVER",CT_SELECTOR},
    {0x1026,0x1000,12,1,"GEAR_L_SAFE",CT_LED},
    {0x1026,0x0800,11,1,"GEAR_N_SAFE",CT_LED},
    {0x1026,0x2000,13,1,"GEAR_R_SAFE",CT_LED},
    {0x1026,0x4000,14,1,"HANDLE_GEAR_WARNING",CT_LED},
    {0x1144,0x1800,11,2,"LANDING_LIGHTS",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[9]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x10E8, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[4], &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[9] }, 9 },
  { 0x10EE, { &DcsOutputTable[5] }, 1 },
  { 0x1110, { &DcsOutputTable[10], &DcsOutputTable[11], &DcsOutputTable[15] }, 3 },
  { 0x1108, { &DcsOutputTable[12] }, 1 },
  { 0x10A0, { &DcsOutputTable[13] }, 1 },
  { 0x10F2, { &DcsOutputTable[14] }, 1 },
  { 0x1026, { &DcsOutputTable[16], &DcsOutputTable[17], &DcsOutputTable[18], &DcsOutputTable[19] }, 4 },
  { 0x1144, { &DcsOutputTable[20] }, 1 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  { 0x1026, &dcsAddressTable[6] },
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
  { 0x1108, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x10A0, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x1144, &dcsAddressTable[7] },
  { 0x1110, &dcsAddressTable[2] },
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
  { 0x10E8, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x10EE, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x10F2, &dcsAddressTable[5] },
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
    if (entry.addr == 0xFFFF) return nullptr;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "AHCP_ALT_SCE_POS0","AHCP_ALT_SCE",0,"selector",2,"POS0" },
    { "AHCP_ALT_SCE_POS1","AHCP_ALT_SCE",1,"selector",2,"POS1" },
    { "AHCP_ALT_SCE_POS2","AHCP_ALT_SCE",2,"selector",2,"POS2" },
    { "AHCP_CICU_POS0","AHCP_CICU",0,"selector",3,"POS0" },
    { "AHCP_CICU_POS1","AHCP_CICU",1,"selector",3,"POS1" },
    { "AHCP_GUNPAC_POS0","AHCP_GUNPAC",0,"selector",4,"POS0" },
    { "AHCP_GUNPAC_POS1","AHCP_GUNPAC",1,"selector",4,"POS1" },
    { "AHCP_GUNPAC_POS2","AHCP_GUNPAC",2,"selector",4,"POS2" },
    { "AHCP_HUD_DAYNIGHT_POS0","AHCP_HUD_DAYNIGHT",0,"selector",5,"POS0" },
    { "AHCP_HUD_DAYNIGHT_POS1","AHCP_HUD_DAYNIGHT",1,"selector",5,"POS1" },
    { "AHCP_HUD_MODE_POS0","AHCP_HUD_MODE",0,"selector",6,"POS0" },
    { "AHCP_HUD_MODE_POS1","AHCP_HUD_MODE",1,"selector",6,"POS1" },
    { "AHCP_IFFCC_POS0","AHCP_IFFCC",0,"selector",7,"POS0" },
    { "AHCP_IFFCC_POS1","AHCP_IFFCC",1,"selector",7,"POS1" },
    { "AHCP_IFFCC_POS2","AHCP_IFFCC",2,"selector",7,"POS2" },
    { "AHCP_JTRS_POS0","AHCP_JTRS",0,"selector",8,"POS0" },
    { "AHCP_JTRS_POS1","AHCP_JTRS",1,"selector",8,"POS1" },
    { "AHCP_LASER_ARM_POS0","AHCP_LASER_ARM",0,"selector",9,"POS0" },
    { "AHCP_LASER_ARM_POS1","AHCP_LASER_ARM",1,"selector",9,"POS1" },
    { "AHCP_LASER_ARM_POS2","AHCP_LASER_ARM",2,"selector",9,"POS2" },
    { "AHCP_MASTER_ARM_POS0","AHCP_MASTER_ARM",0,"selector",10,"POS0" },
    { "AHCP_MASTER_ARM_POS1","AHCP_MASTER_ARM",1,"selector",10,"POS1" },
    { "AHCP_MASTER_ARM_POS2","AHCP_MASTER_ARM",2,"selector",10,"POS2" },
    { "AHCP_TGP_POS0","AHCP_TGP",0,"selector",11,"POS0" },
    { "AHCP_TGP_POS1","AHCP_TGP",1,"selector",11,"POS1" },
    { "ANTI_SKID_SWITCH_POS0","ANTI_SKID_SWITCH",0,"selector",12,"POS0" },
    { "ANTI_SKID_SWITCH_POS1","ANTI_SKID_SWITCH",1,"selector",12,"POS1" },
    { "DOWNLOCK_OVERRIDE","DOWNLOCK_OVERRIDE",1,"momentary",0,"PRESS" },
    { "ENGINE_TEMS_DATA","ENGINE_TEMS_DATA",1,"momentary",0,"PRESS" },
    { "GEAR_HORN_SILENCE","GEAR_HORN_SILENCE",1,"momentary",0,"PRESS" },
    { "GEAR_LEVER_POS0","GEAR_LEVER",0,"selector",13,"POS0" },
    { "GEAR_LEVER_POS1","GEAR_LEVER",1,"selector",13,"POS1" },
    { "LANDING_LIGHTS_LAND","LANDING_LIGHTS",0,"selector",1,"LAND" },
    { "LANDING_LIGHTS_OFF","LANDING_LIGHTS",1,"selector",1,"OFF" },
    { "LANDING_LIGHTS_TAXI","LANDING_LIGHTS",2,"selector",1,"TAXI" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[71] = {
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"AHCP_JTRS", 0, &SelectorMap[15]},
  {"AHCP_TGP", 0, &SelectorMap[23]},
  {"AHCP_ALT_SCE", 1, &SelectorMap[1]},
  {"AHCP_LASER_ARM", 1, &SelectorMap[18]},
  {nullptr, 0, nullptr},
  {"ANTI_SKID_SWITCH", 0, &SelectorMap[25]},
  {"AHCP_MASTER_ARM", 1, &SelectorMap[21]},
  {"AHCP_JTRS", 1, &SelectorMap[16]},
  {nullptr, 0, nullptr},
  {"AHCP_LASER_ARM", 0, &SelectorMap[17]},
  {nullptr, 0, nullptr},
  {"AHCP_LASER_ARM", 2, &SelectorMap[19]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"AHCP_IFFCC", 1, &SelectorMap[13]},
  {nullptr, 0, nullptr},
  {"ANTI_SKID_SWITCH", 1, &SelectorMap[26]},
  {"AHCP_GUNPAC", 0, &SelectorMap[5]},
  {"AHCP_HUD_DAYNIGHT", 1, &SelectorMap[9]},
  {"AHCP_MASTER_ARM", 2, &SelectorMap[22]},
  {"AHCP_HUD_MODE", 0, &SelectorMap[10]},
  {"AHCP_HUD_MODE", 1, &SelectorMap[11]},
  {"GEAR_HORN_SILENCE", 1, &SelectorMap[29]},
  {"AHCP_IFFCC", 0, &SelectorMap[12]},
  {"AHCP_ALT_SCE", 0, &SelectorMap[0]},
  {nullptr, 0, nullptr},
  {"AHCP_TGP", 1, &SelectorMap[24]},
  {"ENGINE_TEMS_DATA", 1, &SelectorMap[28]},
  {"AHCP_HUD_DAYNIGHT", 0, &SelectorMap[8]},
  {"GEAR_LEVER", 0, &SelectorMap[30]},
  {"GEAR_LEVER", 1, &SelectorMap[31]},
  {"LANDING_LIGHTS", 0, &SelectorMap[32]},
  {"AHCP_ALT_SCE", 2, &SelectorMap[2]},
  {"LANDING_LIGHTS", 1, &SelectorMap[33]},
  {"LANDING_LIGHTS", 2, &SelectorMap[34]},
  {nullptr, 0, nullptr},
  {"AHCP_CICU", 0, &SelectorMap[3]},
  {"AHCP_IFFCC", 2, &SelectorMap[14]},
  {"AHCP_CICU", 1, &SelectorMap[4]},
  {"AHCP_GUNPAC", 1, &SelectorMap[6]},
  {"AHCP_MASTER_ARM", 0, &SelectorMap[20]},
  {nullptr, 0, nullptr},
  {"DOWNLOCK_OVERRIDE", 1, &SelectorMap[27]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"AHCP_GUNPAC", 2, &SelectorMap[7]},
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
};

// Composite hash: labelHash(dcsCommand) ^ (value * 7919)
inline const SelectorEntry* findSelectorByDcsAndValue(const char* dcsCommand, uint16_t value) {
  uint16_t startH = (labelHash(dcsCommand) ^ (value * 7919u)) % 71;
  for (uint16_t i = 0; i < 71; ++i) {
    uint16_t idx = (startH + i >= 71) ? (startH + i - 71) : (startH + i);
    const auto& entry = selectorHashTable[idx];
    if (!entry.dcsCommand) return nullptr;
    if (entry.value == value && strcmp(entry.dcsCommand, dcsCommand) == 0) return entry.entry;
  }
  return nullptr;
}


// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "AHCP_ALT_SCE", 0, 0, true, 2, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_CICU", 0, 0, true, 3, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_GUNPAC", 0, 0, true, 4, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_HUD_DAYNIGHT", 0, 0, true, 5, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_HUD_MODE", 0, 0, true, 6, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_IFFCC", 0, 0, true, 7, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_JTRS", 0, 0, true, 8, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_LASER_ARM", 0, 0, true, 9, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_MASTER_ARM", 0, 0, true, 10, 0,   0, false, 0, {0}, {0}, 0 },
    { "AHCP_TGP", 0, 0, true, 11, 0,   0, false, 0, {0}, {0}, 0 },
    { "ANTI_SKID_SWITCH", 0, 0, true, 12, 0,   0, false, 0, {0}, {0}, 0 },
    { "DOWNLOCK_OVERRIDE", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "ENGINE_TEMS_DATA", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "GEAR_HORN_SILENCE", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "GEAR_LEVER", 0, 0, true, 13, 0,   0, false, 0, {0}, {0}, 0 },
    { "LANDING_LIGHTS", 0, 0, true, 1, 0,   0, false, 0, {0}, {0}, 0 },
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
