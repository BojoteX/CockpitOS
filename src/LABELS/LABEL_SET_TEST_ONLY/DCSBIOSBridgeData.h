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
    {0x740C,0x0008,3,1,"APU_FIRE_BTN",CT_SELECTOR},
    {0x740C,0x0004,2,1,"FIRE_APU_LT",CT_LED},
    {0x740A,0x0008,3,1,"LH_ADV_ASPJ_OH",CT_LED},
    {0x740A,0x0010,4,1,"LH_ADV_GO",CT_LED},
    {0x740A,0x0002,1,1,"LH_ADV_L_BAR_GREEN",CT_LED},
    {0x7408,0x8000,15,1,"LH_ADV_L_BAR_RED",CT_LED},
    {0x7408,0x0800,11,1,"LH_ADV_L_BLEED",CT_LED},
    {0x740A,0x0020,5,1,"LH_ADV_NO_GO",CT_LED},
    {0x740A,0x0001,0,1,"LH_ADV_REC",CT_LED},
    {0x7408,0x1000,12,1,"LH_ADV_R_BLEED",CT_LED},
    {0x7408,0x2000,13,1,"LH_ADV_SPD_BRK",CT_LED},
    {0x7408,0x4000,14,1,"LH_ADV_STBY",CT_LED},
    {0x740A,0x0004,2,1,"LH_ADV_XMIT",CT_LED},
    {0x7408,0x0040,6,1,"FIRE_LEFT_LT",CT_LED},
    {0x7408,0x0080,7,1,"LEFT_FIRE_BTN",CT_SELECTOR},
    {0x7408,0x0100,8,1,"LEFT_FIRE_BTN_COVER",CT_SELECTOR},
    {0x7408,0x0200,9,1,"MASTER_CAUTION_LT",CT_LED},
    {0x7408,0x0400,10,1,"MASTER_CAUTION_RESET_SW",CT_SELECTOR},
    {0x740A,0x0800,11,1,"RH_ADV_AAA",CT_LED},
    {0x740A,0x0400,10,1,"RH_ADV_AI",CT_LED},
    {0x740A,0x1000,12,1,"RH_ADV_CW",CT_LED},
    {0x740A,0x0100,8,1,"RH_ADV_DISP",CT_LED},
    {0x740A,0x0080,7,1,"RH_ADV_RCDR_ON",CT_LED},
    {0x740A,0x0200,9,1,"RH_ADV_SAM",CT_LED},
    {0x740A,0x2000,13,1,"RH_ADV_SPARE_RH1",CT_LED},
    {0x740A,0x4000,14,1,"RH_ADV_SPARE_RH2",CT_LED},
    {0x740A,0x8000,15,1,"RH_ADV_SPARE_RH3",CT_LED},
    {0x740C,0x0001,0,1,"RH_ADV_SPARE_RH4",CT_LED},
    {0x740C,0x0002,1,1,"RH_ADV_SPARE_RH5",CT_LED},
    {0x7554,0xFFFF,0,65535,"RWR_AUDIO_CTRL",CT_ANALOG},
    {0x7498,0x0100,8,1,"RWR_BIT_BTN",CT_SELECTOR},
    {0x749C,0x1000,12,1,"RWR_BIT_LT",CT_LED},
    {0x7488,0x2000,13,1,"RWR_DISPLAY_BTN",CT_SELECTOR},
    {0x7498,0x4000,14,1,"RWR_DISPLAY_LT",CT_LED},
    {0x7498,0x0E00,9,4,"RWR_DIS_TYPE_SW",CT_SELECTOR},
    {0x7508,0xFFFF,0,65535,"RWR_DMR_CTRL",CT_ANALOG},
    {0x749C,0x0200,9,1,"RWR_ENABLE_LT",CT_LED},
    {0x749C,0x0800,11,1,"RWR_FAIL_LT",CT_LED},
    {0x7498,0x2000,13,1,"RWR_LIMIT_LT",CT_LED},
    {0x7498,0x1000,12,1,"RWR_LOWER_LT",CT_LED},
    {0x7568,0xFFFF,0,65535,"RWR_LT_BRIGHT",CT_GAUGE},
    {0x7488,0x8000,15,1,"RWR_OFFSET_BTN",CT_SELECTOR},
    {0x749C,0x0400,10,1,"RWR_OFFSET_LT",CT_LED},
    {0x7488,0x1000,12,1,"RWR_POWER_BTN",CT_SELECTOR},
    {0x750A,0xFFFF,0,65535,"RWR_RWR_INTESITY",CT_ANALOG},
    {0x7488,0x4000,14,1,"RWR_SPECIAL_BTN",CT_SELECTOR},
    {0x7498,0x8000,15,1,"RWR_SPECIAL_EN_LT",CT_LED},
    {0x749C,0x0100,8,1,"RWR_SPECIAL_LT",CT_LED},
    {0x740C,0x0010,4,1,"FIRE_RIGHT_LT",CT_LED},
    {0x740C,0x0020,5,1,"RIGHT_FIRE_BTN",CT_SELECTOR},
    {0x740C,0x0040,6,1,"RIGHT_FIRE_BTN_COVER",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[15]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x740C, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[27], &DcsOutputTable[28], &DcsOutputTable[48], &DcsOutputTable[49], &DcsOutputTable[50] }, 7 },
  { 0x740A, { &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[4], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[12], &DcsOutputTable[18], &DcsOutputTable[19], &DcsOutputTable[20], &DcsOutputTable[21], &DcsOutputTable[22], &DcsOutputTable[23], &DcsOutputTable[24], &DcsOutputTable[25], &DcsOutputTable[26] }, 15 },
  { 0x7408, { &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[9], &DcsOutputTable[10], &DcsOutputTable[11], &DcsOutputTable[13], &DcsOutputTable[14], &DcsOutputTable[15], &DcsOutputTable[16], &DcsOutputTable[17] }, 10 },
  { 0x7554, { &DcsOutputTable[29] }, 1 },
  { 0x7498, { &DcsOutputTable[30], &DcsOutputTable[33], &DcsOutputTable[34], &DcsOutputTable[38], &DcsOutputTable[39], &DcsOutputTable[46] }, 6 },
  { 0x749C, { &DcsOutputTable[31], &DcsOutputTable[36], &DcsOutputTable[37], &DcsOutputTable[42], &DcsOutputTable[47] }, 5 },
  { 0x7488, { &DcsOutputTable[32], &DcsOutputTable[41], &DcsOutputTable[43], &DcsOutputTable[45] }, 4 },
  { 0x7508, { &DcsOutputTable[35] }, 1 },
  { 0x7568, { &DcsOutputTable[40] }, 1 },
  { 0x750A, { &DcsOutputTable[44] }, 1 },
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
  { 0x7568, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7498, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x749C, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x7508, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x750A, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7408, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x740A, &dcsAddressTable[1] },
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
  { 0x7554, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7488, &dcsAddressTable[6] },
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
    { "APU_FIRE_BTN","APU_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "LEFT_FIRE_BTN","LEFT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "LEFT_FIRE_BTN_COVER","LEFT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
    { "MASTER_CAUTION_RESET_SW","MASTER_CAUTION_RESET_SW",1,"momentary",0,"PRESS" },
    { "RWR_AUDIO_CTRL","RWR_AUDIO_CTRL",65535,"analog",0,"LEVEL" },
    { "RWR_AUDIO_CTRL_DEC","RWR_AUDIO_CTRL",0,"variable_step",0,"DEC" },
    { "RWR_AUDIO_CTRL_INC","RWR_AUDIO_CTRL",1,"variable_step",0,"INC" },
    { "RWR_BIT_BTN","RWR_BIT_BTN",1,"momentary",0,"PRESS" },
    { "RWR_DISPLAY_BTN","RWR_DISPLAY_BTN",1,"momentary",0,"PRESS" },
    { "RWR_DIS_TYPE_SW_N","RWR_DIS_TYPE_SW",0,"selector",1,"N" },
    { "RWR_DIS_TYPE_SW_I","RWR_DIS_TYPE_SW",1,"selector",1,"I" },
    { "RWR_DIS_TYPE_SW_A","RWR_DIS_TYPE_SW",2,"selector",1,"A" },
    { "RWR_DIS_TYPE_SW_U","RWR_DIS_TYPE_SW",3,"selector",1,"U" },
    { "RWR_DIS_TYPE_SW_F","RWR_DIS_TYPE_SW",4,"selector",1,"F" },
    { "RWR_DMR_CTRL","RWR_DMR_CTRL",65535,"analog",0,"LEVEL" },
    { "RWR_DMR_CTRL_DEC","RWR_DMR_CTRL",0,"variable_step",0,"DEC" },
    { "RWR_DMR_CTRL_INC","RWR_DMR_CTRL",1,"variable_step",0,"INC" },
    { "RWR_OFFSET_BTN","RWR_OFFSET_BTN",1,"momentary",0,"PRESS" },
    { "RWR_POWER_BTN","RWR_POWER_BTN",1,"momentary",0,"PRESS" },
    { "RWR_RWR_INTESITY","RWR_RWR_INTESITY",65535,"analog",0,"LEVEL" },
    { "RWR_RWR_INTESITY_DEC","RWR_RWR_INTESITY",0,"variable_step",0,"DEC" },
    { "RWR_RWR_INTESITY_INC","RWR_RWR_INTESITY",1,"variable_step",0,"INC" },
    { "RWR_SPECIAL_BTN","RWR_SPECIAL_BTN",1,"momentary",0,"PRESS" },
    { "RIGHT_FIRE_BTN","RIGHT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "RIGHT_FIRE_BTN_COVER","RIGHT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "APU_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_CAUTION_RESET_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_AUDIO_CTRL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_BIT_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_DISPLAY_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_DIS_TYPE_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "RWR_DMR_CTRL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_OFFSET_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_POWER_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_RWR_INTESITY", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_SPECIAL_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
