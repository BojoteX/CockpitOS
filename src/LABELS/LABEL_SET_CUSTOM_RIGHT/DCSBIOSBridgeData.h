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
    {0x74A0,0x0400,10,1,"ARRESTING_HOOK_LT",CT_LED},
    {0x74A0,0x0200,9,1,"HOOK_LEVER",CT_SELECTOR},
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
    {0x74A0,0x4000,14,1,"AV_COOL_SW",CT_SELECTOR},
    {0x751E,0xFFFF,0,65535,"HYD_IND_LEFT",CT_GAUGE},
    {0x7520,0xFFFF,0,65535,"HYD_IND_RIGHT",CT_GAUGE},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x749C,0x8000,15,1,"LOW_ALT_WARN_LT",CT_LED},
    {0x751A,0xFFFF,0,65535,"RADALT_ALT_PTR",CT_GAUGE},
    {0x74A0,0x0100,8,1,"RADALT_GREEN_LAMP",CT_LED},
    {0x7516,0xFFFF,0,65535,"RADALT_HEIGHT",CT_ANALOG},
    {0x7518,0xFFFF,0,65535,"RADALT_MIN_HEIGHT_PTR",CT_GAUGE},
    {0x751C,0xFFFF,0,65535,"RADALT_OFF_FLAG",CT_GAUGE},
    {0x749C,0x4000,14,1,"RADALT_TEST_SW",CT_SELECTOR},
    {0x74A0,0x0800,11,1,"WING_FOLD_PULL",CT_SELECTOR},
    {0x74A0,0x3000,12,2,"WING_FOLD_ROTATE",CT_SELECTOR},
    {0x74A0,0x0800,11,1,"WING_FOLD_PULL",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[8]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x74A0, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[4], &DcsOutputTable[14], &DcsOutputTable[26], &DcsOutputTable[31], &DcsOutputTable[32], &DcsOutputTable[33] }, 8 },
  { 0x74A4, { &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[11], &DcsOutputTable[12] }, 8 },
  { 0x74A8, { &DcsOutputTable[9], &DcsOutputTable[10], &DcsOutputTable[13] }, 3 },
  { 0x751E, { &DcsOutputTable[15] }, 1 },
  { 0x7520, { &DcsOutputTable[16] }, 1 },
  { 0x754A, { &DcsOutputTable[17] }, 1 },
  { 0x74C8, { &DcsOutputTable[18], &DcsOutputTable[22] }, 2 },
  { 0x7544, { &DcsOutputTable[19] }, 1 },
  { 0x7548, { &DcsOutputTable[20] }, 1 },
  { 0x7546, { &DcsOutputTable[21] }, 1 },
  { 0x754C, { &DcsOutputTable[23] }, 1 },
  { 0x749C, { &DcsOutputTable[24], &DcsOutputTable[30] }, 2 },
  { 0x751A, { &DcsOutputTable[25] }, 1 },
  { 0x7516, { &DcsOutputTable[27] }, 1 },
  { 0x7518, { &DcsOutputTable[28] }, 1 },
  { 0x751C, { &DcsOutputTable[29] }, 1 },
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
  { 0x74C8, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x749C, &dcsAddressTable[11] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74A0, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74A4, &dcsAddressTable[1] },
  { 0x7544, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x7546, &dcsAddressTable[9] },
  { 0x74A8, &dcsAddressTable[2] },
  { 0x7548, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x754A, &dcsAddressTable[5] },
  { 0x7516, &dcsAddressTable[13] },
  { 0x754C, &dcsAddressTable[10] },
  { 0x7518, &dcsAddressTable[14] },
  {0xFFFF, nullptr},
  { 0x751A, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  { 0x751C, &dcsAddressTable[15] },
  {0xFFFF, nullptr},
  { 0x751E, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x7520, &dcsAddressTable[4] },
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
    if (entry.addr == 0xFFFF) return nullptr;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "HOOK_LEVER_POS0","HOOK_LEVER",0,"selector",5,"POS0" },
    { "HOOK_LEVER_POS1","HOOK_LEVER",1,"selector",5,"POS1" },
    { "AV_COOL_SW_EMERG","AV_COOL_SW",0,"selector",1,"EMERG" },
    { "AV_COOL_SW_NORM","AV_COOL_SW",1,"selector",1,"NORM" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "CHART_DIMMER_DEC","CHART_DIMMER",0,"variable_step",0,"DEC" },
    { "CHART_DIMMER_INC","CHART_DIMMER",1,"variable_step",0,"INC" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",2,"DAY" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",2,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",2,"NVG" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "CONSOLES_DIMMER_DEC","CONSOLES_DIMMER",0,"variable_step",0,"DEC" },
    { "CONSOLES_DIMMER_INC","CONSOLES_DIMMER",1,"variable_step",0,"INC" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER_DEC","FLOOD_DIMMER",0,"variable_step",0,"DEC" },
    { "FLOOD_DIMMER_INC","FLOOD_DIMMER",1,"variable_step",0,"INC" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER_DEC","INST_PNL_DIMMER",0,"variable_step",0,"DEC" },
    { "INST_PNL_DIMMER_INC","INST_PNL_DIMMER",1,"variable_step",0,"INC" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",3,"OFF" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",3,"TEST" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "WARN_CAUTION_DIMMER_DEC","WARN_CAUTION_DIMMER",0,"variable_step",0,"DEC" },
    { "WARN_CAUTION_DIMMER_INC","WARN_CAUTION_DIMMER",1,"variable_step",0,"INC" },
    { "RADALT_HEIGHT_POS0","RADALT_HEIGHT",0,"variable_step",0,"POS0" },
    { "RADALT_HEIGHT_POS1","RADALT_HEIGHT",1,"variable_step",0,"POS1" },
    { "RADALT_TEST_SW","RADALT_TEST_SW",1,"momentary",0,"PRESS" },
    { "WING_FOLD_PULL_POS0","WING_FOLD_PULL",0,"selector",6,"POS0" },
    { "WING_FOLD_PULL_POS1","WING_FOLD_PULL",1,"selector",6,"POS1" },
    { "WING_FOLD_ROTATE_UNFOLD","WING_FOLD_ROTATE",0,"selector",4,"UNFOLD" },
    { "WING_FOLD_ROTATE_HOLD","WING_FOLD_ROTATE",1,"selector",4,"HOLD" },
    { "WING_FOLD_ROTATE_FOLD","WING_FOLD_ROTATE",2,"selector",4,"FOLD" },
    { "WING_FOLD_CUSTOM_PULL_POS0","WING_FOLD_PULL",0,"selector",6,"POS0" },
    { "WING_FOLD_CUSTOM_PULL_POS1","WING_FOLD_PULL",1,"selector",6,"POS1" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "AV_COOL_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "HOOK_LEVER", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "RADALT_HEIGHT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RADALT_TEST_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "WARN_CAUTION_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "WING_FOLD_PULL", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "WING_FOLD_ROTATE", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
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
