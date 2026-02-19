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
    {0x740C,0x0100,8,1,"CANOPY_JETT_HANDLE_PULL",CT_SELECTOR},
    {0x740C,0x0080,7,1,"CANOPY_JETT_HANDLE_UNLOCK",CT_SELECTOR},
    {0x74A4,0x0100,8,1,"CLIP_APU_ACC_LT",CT_LED},
    {0x74A4,0x0200,9,1,"CLIP_BATT_SW_LT",CT_LED},
    {0x74A0,0x8000,15,1,"CLIP_CK_SEAT_LT",CT_LED},
    {0x74A4,0x4000,14,1,"CLIP_FCES_LT",CT_LED},
    {0x74A4,0x0400,10,1,"CLIP_FCS_HOT_LT",CT_LED},
    {0x74A4,0x2000,13,1,"CLIP_FUEL_LO_LT",CT_LED},
    {0x74A4,0x0800,11,1,"CLIP_GEN_TIE_LT",CT_LED},
    {0x74A8,0x0100,8,1,"CLIP_L_GEN_LT",CT_LED},
    {0x74A8,0x0200,9,1,"CLIP_R_GEN_LT",CT_LED},
    {0x74A4,0x1000,12,1,"CLIP_SPARE_CTN1_LT",CT_LED},
    {0x74A4,0x8000,15,1,"CLIP_SPARE_CTN2_LT",CT_LED},
    {0x74A8,0x0400,10,1,"CLIP_SPARE_CTN3_LT",CT_LED},
    {0x7510,0xFFFF,0,65535,"CLOCK_ELAPSED_MINUTES",CT_GAUGE},
    {0x7512,0xFFFF,0,65535,"CLOCK_ELAPSED_SECONDS",CT_GAUGE},
    {0x750C,0xFFFF,0,65535,"CLOCK_HOURS",CT_GAUGE},
    {0x750E,0xFFFF,0,65535,"CLOCK_MINUTES",CT_GAUGE},
    {0x7488,0x0800,11,1,"AUX_REL_SW",CT_SELECTOR},
    {0x7484,0x6000,13,2,"CMSD_DISPENSE_SW",CT_SELECTOR},
    {0x7484,0x8000,15,1,"CMSD_JET_SEL_BTN",CT_SELECTOR},
    {0x74D4,0x8000,15,1,"CMSD_JET_SEL_L",CT_LED},
    {0x7488,0x0700,8,4,"ECM_MODE_SW",CT_SELECTOR},
    {0x7408,0x0040,6,1,"FIRE_LEFT_LT",CT_LED},
    {0x7408,0x0080,7,1,"LEFT_FIRE_BTN",CT_SELECTOR},
    {0x7408,0x0100,8,1,"LEFT_FIRE_BTN_COVER",CT_SELECTOR},
    {0x7456,0xFFFF,0,65535,"HMD_OFF_BRT",CT_ANALOG},
    {0x742A,0xC000,14,2,"IR_COOL_SW",CT_SELECTOR},
    {0x742A,0x0800,11,1,"SPIN_LT",CT_LED},
    {0x742A,0x1000,12,1,"SPIN_RECOVERY_COVER",CT_SELECTOR},
    {0x742A,0x2000,13,1,"SPIN_RECOVERY_SW",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[8]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x740C, { &DcsOutputTable[0], &DcsOutputTable[1] }, 2 },
  { 0x74A4, { &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[11], &DcsOutputTable[12] }, 8 },
  { 0x74A0, { &DcsOutputTable[4] }, 1 },
  { 0x74A8, { &DcsOutputTable[9], &DcsOutputTable[10], &DcsOutputTable[13] }, 3 },
  { 0x7510, { &DcsOutputTable[14] }, 1 },
  { 0x7512, { &DcsOutputTable[15] }, 1 },
  { 0x750C, { &DcsOutputTable[16] }, 1 },
  { 0x750E, { &DcsOutputTable[17] }, 1 },
  { 0x7488, { &DcsOutputTable[18], &DcsOutputTable[22] }, 2 },
  { 0x7484, { &DcsOutputTable[19], &DcsOutputTable[20] }, 2 },
  { 0x74D4, { &DcsOutputTable[21] }, 1 },
  { 0x7408, { &DcsOutputTable[23], &DcsOutputTable[24], &DcsOutputTable[25] }, 3 },
  { 0x7456, { &DcsOutputTable[26] }, 1 },
  { 0x742A, { &DcsOutputTable[27], &DcsOutputTable[28], &DcsOutputTable[29], &DcsOutputTable[30] }, 4 },
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
  { 0x742A, &dcsAddressTable[13] },
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
  { 0x74D4, &dcsAddressTable[10] },
  { 0x74A0, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x750C, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x74A4, &dcsAddressTable[1] },
  { 0x750E, &dcsAddressTable[7] },
  { 0x7510, &dcsAddressTable[4] },
  { 0x7408, &dcsAddressTable[11] },
  { 0x74A8, &dcsAddressTable[3] },
  { 0x7512, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x740C, &dcsAddressTable[0] },
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
  { 0x7484, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7488, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7456, &dcsAddressTable[12] },
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
    { "CANOPY_JETT_HANDLE_PULL","CANOPY_JETT_HANDLE_PULL",1,"momentary",0,"PRESS" },
    { "CANOPY_JETT_HANDLE_UNLOCK","CANOPY_JETT_HANDLE_UNLOCK",1,"momentary",0,"PRESS" },
    { "AUX_REL_SW_NORM","AUX_REL_SW",0,"selector",1,"NORM" },
    { "AUX_REL_SW_ENABLE","AUX_REL_SW",1,"selector",1,"ENABLE" },
    { "CMSD_DISPENSE_SW_OFF","CMSD_DISPENSE_SW",0,"selector",2,"OFF" },
    { "CMSD_DISPENSE_SW_ON","CMSD_DISPENSE_SW",1,"selector",2,"ON" },
    { "CMSD_DISPENSE_SW_BYPASS","CMSD_DISPENSE_SW",2,"selector",2,"BYPASS" },
    { "CMSD_JET_SEL_BTN","CMSD_JET_SEL_BTN",1,"momentary",0,"PRESS" },
    { "ECM_MODE_SW_OFF","ECM_MODE_SW",0,"selector",3,"OFF" },
    { "ECM_MODE_SW_STBY","ECM_MODE_SW",1,"selector",3,"STBY" },
    { "ECM_MODE_SW_BIT","ECM_MODE_SW",2,"selector",3,"BIT" },
    { "ECM_MODE_SW_REC","ECM_MODE_SW",3,"selector",3,"REC" },
    { "ECM_MODE_SW_XMIT","ECM_MODE_SW",4,"selector",3,"XMIT" },
    { "ECM_MODE_SW_DEC","ECM_MODE_SW",0,"fixed_step",0,"DEC" },
    { "ECM_MODE_SW_INC","ECM_MODE_SW",1,"fixed_step",0,"INC" },
    { "LEFT_FIRE_BTN","LEFT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "LEFT_FIRE_BTN_COVER","LEFT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
    { "HMD_OFF_BRT","HMD_OFF_BRT",65535,"analog",0,"LEVEL" },
    { "HMD_OFF_BRT_DEC","HMD_OFF_BRT",0,"variable_step",0,"DEC" },
    { "HMD_OFF_BRT_INC","HMD_OFF_BRT",1,"variable_step",0,"INC" },
    { "IR_COOL_SW_OFF","IR_COOL_SW",0,"selector",4,"OFF" },
    { "IR_COOL_SW_NORM","IR_COOL_SW",1,"selector",4,"NORM" },
    { "IR_COOL_SW_ORIDE","IR_COOL_SW",2,"selector",4,"ORIDE" },
    { "SPIN_RECOVERY_COVER","SPIN_RECOVERY_COVER",1,"momentary",0,"OPEN" },
    { "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_SW",0,"selector",5,"NORM" },
    { "SPIN_RECOVERY_SW_RCVY","SPIN_RECOVERY_SW",1,"selector",5,"RCVY" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[53] = {
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"CMSD_DISPENSE_SW", 0, &SelectorMap[4]},
  {"ECM_MODE_SW", 1, &SelectorMap[9]},
  {"CMSD_JET_SEL_BTN", 1, &SelectorMap[7]},
  {"ECM_MODE_SW", 1, &SelectorMap[14]},
  {"HMD_OFF_BRT", 65535, &SelectorMap[17]},
  {"CANOPY_JETT_HANDLE_PULL", 1, &SelectorMap[0]},
  {"AUX_REL_SW", 1, &SelectorMap[3]},
  {"SPIN_RECOVERY_SW", 0, &SelectorMap[24]},
  {"IR_COOL_SW", 1, &SelectorMap[21]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"ECM_MODE_SW", 2, &SelectorMap[10]},
  {"SPIN_RECOVERY_COVER", 1, &SelectorMap[23]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"LEFT_FIRE_BTN", 1, &SelectorMap[15]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"ECM_MODE_SW", 3, &SelectorMap[11]},
  {"IR_COOL_SW", 2, &SelectorMap[22]},
  {nullptr, 0, nullptr},
  {"HMD_OFF_BRT", 0, &SelectorMap[18]},
  {"CMSD_DISPENSE_SW", 1, &SelectorMap[5]},
  {"IR_COOL_SW", 0, &SelectorMap[20]},
  {"AUX_REL_SW", 0, &SelectorMap[2]},
  {"HMD_OFF_BRT", 1, &SelectorMap[19]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"ECM_MODE_SW", 0, &SelectorMap[8]},
  {"ECM_MODE_SW", 4, &SelectorMap[12]},
  {"ECM_MODE_SW", 0, &SelectorMap[13]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"LEFT_FIRE_BTN_COVER", 1, &SelectorMap[16]},
  {"CMSD_DISPENSE_SW", 2, &SelectorMap[6]},
  {"CANOPY_JETT_HANDLE_UNLOCK", 1, &SelectorMap[1]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"SPIN_RECOVERY_SW", 1, &SelectorMap[25]},
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
    { "AUX_REL_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "CANOPY_JETT_HANDLE_PULL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CANOPY_JETT_HANDLE_UNLOCK", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_DISPENSE_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_JET_SEL_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ECM_MODE_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "HMD_OFF_BRT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IR_COOL_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
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
