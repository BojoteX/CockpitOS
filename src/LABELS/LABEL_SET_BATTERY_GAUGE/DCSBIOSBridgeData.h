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
    {0x74C4,0x1800,11,2,"BATTERY_SW",CT_SELECTOR},
    {0x74C4,0x2000,13,1,"L_GEN_SW",CT_SELECTOR},
    {0x74C4,0x4000,14,1,"R_GEN_SW",CT_SELECTOR},
    {0x753E,0xFFFF,0,65535,"VOLT_E",CT_GAUGE},
    {0x753C,0xFFFF,0,65535,"VOLT_U",CT_GAUGE},
    {0x7456,0xFFFF,0,65535,"HMD_OFF_BRT",CT_ANALOG},
    {0x742A,0xC000,14,2,"IR_COOL_SW",CT_SELECTOR},
    {0x742A,0x0800,11,1,"SPIN_LT",CT_LED},
    {0x742A,0x1000,12,1,"SPIN_RECOVERY_COVER",CT_SELECTOR},
    {0x742A,0x2000,13,1,"SPIN_RECOVERY_SW",CT_SELECTOR},
    {0x7408,0x0200,9,1,"MASTER_CAUTION_LT",CT_LED},
    {0x7408,0x0400,10,1,"MASTER_CAUTION_RESET_SW",CT_SELECTOR},
    {0x0400,0x00FF,0,6,"DCS_BIOS",CT_DISPLAY},
    {0x0401,0x00FF,0,6,"DCS_BIOS",CT_DISPLAY},
    {0x0402,0x00FF,0,6,"DCS_BIOS",CT_DISPLAY},
    {0x0403,0x00FF,0,6,"DCS_BIOS",CT_DISPLAY},
    {0x0404,0x00FF,0,6,"DCS_BIOS",CT_DISPLAY},
    {0x0405,0x00FF,0,6,"DCS_BIOS",CT_DISPLAY},
    {0x0406,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0407,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0408,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0409,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x040A,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x040B,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x040C,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x040D,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x040E,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x040F,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0410,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0411,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0412,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0413,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0414,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0415,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0416,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0417,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0418,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x0419,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x041A,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x041B,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x041C,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0x041D,0x00FF,0,24,"PILOTNAME",CT_DISPLAY},
    {0xFFFE,0x00FF,0,255,"_UPDATE_COUNTER",CT_METADATA},
    {0xFFFE,0xFF00,8,255,"_UPDATE_SKIP_COUNTER",CT_METADATA},
    {0x0000,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0001,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0002,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0003,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0004,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0005,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0006,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0007,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0008,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0009,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x000A,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x000B,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x000C,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x000D,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x000E,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x000F,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0010,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0011,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0012,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0013,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0014,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0015,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0016,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
    {0x0017,0x00FF,0,24,"_ACFT_NAME",CT_DISPLAY},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[4]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x74C4, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[2] }, 3 },
  { 0x753E, { &DcsOutputTable[3] }, 1 },
  { 0x753C, { &DcsOutputTable[4] }, 1 },
  { 0x7456, { &DcsOutputTable[5] }, 1 },
  { 0x742A, { &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[9] }, 4 },
  { 0x7408, { &DcsOutputTable[10], &DcsOutputTable[11] }, 2 },
  { 0x0400, { &DcsOutputTable[12] }, 1 },
  { 0x0401, { &DcsOutputTable[13] }, 1 },
  { 0x0402, { &DcsOutputTable[14] }, 1 },
  { 0x0403, { &DcsOutputTable[15] }, 1 },
  { 0x0404, { &DcsOutputTable[16] }, 1 },
  { 0x0405, { &DcsOutputTable[17] }, 1 },
  { 0x0406, { &DcsOutputTable[18] }, 1 },
  { 0x0407, { &DcsOutputTable[19] }, 1 },
  { 0x0408, { &DcsOutputTable[20] }, 1 },
  { 0x0409, { &DcsOutputTable[21] }, 1 },
  { 0x040A, { &DcsOutputTable[22] }, 1 },
  { 0x040B, { &DcsOutputTable[23] }, 1 },
  { 0x040C, { &DcsOutputTable[24] }, 1 },
  { 0x040D, { &DcsOutputTable[25] }, 1 },
  { 0x040E, { &DcsOutputTable[26] }, 1 },
  { 0x040F, { &DcsOutputTable[27] }, 1 },
  { 0x0410, { &DcsOutputTable[28] }, 1 },
  { 0x0411, { &DcsOutputTable[29] }, 1 },
  { 0x0412, { &DcsOutputTable[30] }, 1 },
  { 0x0413, { &DcsOutputTable[31] }, 1 },
  { 0x0414, { &DcsOutputTable[32] }, 1 },
  { 0x0415, { &DcsOutputTable[33] }, 1 },
  { 0x0416, { &DcsOutputTable[34] }, 1 },
  { 0x0417, { &DcsOutputTable[35] }, 1 },
  { 0x0418, { &DcsOutputTable[36] }, 1 },
  { 0x0419, { &DcsOutputTable[37] }, 1 },
  { 0x041A, { &DcsOutputTable[38] }, 1 },
  { 0x041B, { &DcsOutputTable[39] }, 1 },
  { 0x041C, { &DcsOutputTable[40] }, 1 },
  { 0x041D, { &DcsOutputTable[41] }, 1 },
  { 0xFFFE, { &DcsOutputTable[42], &DcsOutputTable[43] }, 2 },
  { 0x0000, { &DcsOutputTable[44] }, 1 },
  { 0x0001, { &DcsOutputTable[45] }, 1 },
  { 0x0002, { &DcsOutputTable[46] }, 1 },
  { 0x0003, { &DcsOutputTable[47] }, 1 },
  { 0x0004, { &DcsOutputTable[48] }, 1 },
  { 0x0005, { &DcsOutputTable[49] }, 1 },
  { 0x0006, { &DcsOutputTable[50] }, 1 },
  { 0x0007, { &DcsOutputTable[51] }, 1 },
  { 0x0008, { &DcsOutputTable[52] }, 1 },
  { 0x0009, { &DcsOutputTable[53] }, 1 },
  { 0x000A, { &DcsOutputTable[54] }, 1 },
  { 0x000B, { &DcsOutputTable[55] }, 1 },
  { 0x000C, { &DcsOutputTable[56] }, 1 },
  { 0x000D, { &DcsOutputTable[57] }, 1 },
  { 0x000E, { &DcsOutputTable[58] }, 1 },
  { 0x000F, { &DcsOutputTable[59] }, 1 },
  { 0x0010, { &DcsOutputTable[60] }, 1 },
  { 0x0011, { &DcsOutputTable[61] }, 1 },
  { 0x0012, { &DcsOutputTable[62] }, 1 },
  { 0x0013, { &DcsOutputTable[63] }, 1 },
  { 0x0014, { &DcsOutputTable[64] }, 1 },
  { 0x0015, { &DcsOutputTable[65] }, 1 },
  { 0x0016, { &DcsOutputTable[66] }, 1 },
  { 0x0017, { &DcsOutputTable[67] }, 1 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[127] = {
  { 0x0000, &dcsAddressTable[37] },
  { 0x0001, &dcsAddressTable[38] },
  { 0xFFFE, &dcsAddressTable[36] },
  { 0x0002, &dcsAddressTable[39] },
  { 0x0003, &dcsAddressTable[40] },
  { 0x0004, &dcsAddressTable[41] },
  { 0x0005, &dcsAddressTable[42] },
  { 0x0006, &dcsAddressTable[43] },
  { 0x0400, &dcsAddressTable[6] },
  { 0x0401, &dcsAddressTable[7] },
  { 0x0402, &dcsAddressTable[8] },
  { 0x0403, &dcsAddressTable[9] },
  { 0x0404, &dcsAddressTable[10] },
  { 0x0405, &dcsAddressTable[11] },
  { 0x0406, &dcsAddressTable[12] },
  { 0x0407, &dcsAddressTable[13] },
  { 0x0408, &dcsAddressTable[14] },
  { 0x0409, &dcsAddressTable[15] },
  { 0x040A, &dcsAddressTable[16] },
  { 0x040B, &dcsAddressTable[17] },
  { 0x742A, &dcsAddressTable[4] },
  { 0x040C, &dcsAddressTable[18] },
  { 0x040D, &dcsAddressTable[19] },
  { 0x040E, &dcsAddressTable[20] },
  { 0x040F, &dcsAddressTable[21] },
  { 0x0410, &dcsAddressTable[22] },
  { 0x0411, &dcsAddressTable[23] },
  { 0x0412, &dcsAddressTable[24] },
  { 0x0413, &dcsAddressTable[25] },
  { 0x0414, &dcsAddressTable[26] },
  { 0x0415, &dcsAddressTable[27] },
  { 0x0416, &dcsAddressTable[28] },
  { 0x0417, &dcsAddressTable[29] },
  { 0x0418, &dcsAddressTable[30] },
  { 0x0419, &dcsAddressTable[31] },
  { 0x041A, &dcsAddressTable[32] },
  { 0x041B, &dcsAddressTable[33] },
  { 0x041C, &dcsAddressTable[34] },
  { 0x041D, &dcsAddressTable[35] },
  { 0x0007, &dcsAddressTable[44] },
  { 0x753C, &dcsAddressTable[2] },
  { 0x0008, &dcsAddressTable[45] },
  { 0x753E, &dcsAddressTable[1] },
  { 0x0009, &dcsAddressTable[46] },
  { 0x000A, &dcsAddressTable[47] },
  { 0x000B, &dcsAddressTable[48] },
  { 0x000C, &dcsAddressTable[49] },
  { 0x74C4, &dcsAddressTable[0] },
  { 0x000D, &dcsAddressTable[50] },
  { 0x000E, &dcsAddressTable[51] },
  { 0x000F, &dcsAddressTable[52] },
  { 0x0010, &dcsAddressTable[53] },
  { 0x0011, &dcsAddressTable[54] },
  { 0x0012, &dcsAddressTable[55] },
  { 0x0013, &dcsAddressTable[56] },
  { 0x0014, &dcsAddressTable[57] },
  { 0x0015, &dcsAddressTable[58] },
  { 0x0016, &dcsAddressTable[59] },
  { 0x0017, &dcsAddressTable[60] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7456, &dcsAddressTable[3] },
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7408, &dcsAddressTable[5] },
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
  return addr % 127;
}

inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {
  uint16_t startH = addrHash(addr);
  for (uint16_t i = 0; i < 127; ++i) {
    uint16_t idx = (startH + i >= 127) ? (startH + i - 127) : (startH + i);
    const auto& entry = dcsAddressHashTable[idx];
    if (entry.addr == 0xFFFF) continue;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "BATTERY_SW_ON","BATTERY_SW",2,"selector",1,"ON" },
    { "BATTERY_SW_OFF","BATTERY_SW",1,"selector",1,"OFF" },
    { "BATTERY_SW_ORIDE","BATTERY_SW",0,"selector",1,"ORIDE" },
    { "L_GEN_SW_NORM","L_GEN_SW",1,"selector",2,"NORM" },
    { "L_GEN_SW_OFF","L_GEN_SW",0,"selector",2,"OFF" },
    { "R_GEN_SW_NORM","R_GEN_SW",1,"selector",3,"NORM" },
    { "R_GEN_SW_OFF","R_GEN_SW",0,"selector",3,"OFF" },
    { "HMD_OFF_BRT","HMD_OFF_BRT",65535,"analog",0,"LEVEL" },
    { "IR_COOL_SW_ORIDE","IR_COOL_SW",2,"selector",4,"ORIDE" },
    { "IR_COOL_SW_NORM","IR_COOL_SW",1,"selector",4,"NORM" },
    { "IR_COOL_SW_OFF","IR_COOL_SW",0,"selector",4,"OFF" },
    { "SPIN_RECOVERY_COVER","SPIN_RECOVERY_COVER",1,"momentary",0,"OPEN" },
    { "SPIN_RECOVERY_SW_RCVY","SPIN_RECOVERY_SW",1,"selector",5,"RCVY" },
    { "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_SW",0,"selector",5,"NORM" },
    { "MASTER_CAUTION_RESET_SW","MASTER_CAUTION_RESET_SW",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "BATTERY_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "HMD_OFF_BRT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IR_COOL_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "L_GEN_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_CAUTION_RESET_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "R_GEN_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
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
    { "Metadata", "DCS_BIOS", 0x0400, 6 },
    { "Metadata", "PILOTNAME", 0x0406, 24 },
    { "Metadata", "_ACFT_NAME", 0x0000, 24 },
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[7] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"DCS_BIOS", &displayFields[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PILOTNAME", &displayFields[1]},
  {"_ACFT_NAME", &displayFields[2]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 7;
  for (uint16_t i = 0; i < 7; ++i) {
    uint16_t idx = (startH + i >= 7) ? (startH + i - 7) : (startH + i);
    const auto& entry = displayFieldsByLabel[idx];
    if (!entry.label) continue;
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
    { "_UPDATE_COUNTER", 0 }, // Metadata: Update Counter
    { "_UPDATE_SKIP_COUNTER", 0 }, // Metadata: Update Skip Counter
};
static const size_t numMetadataStates = sizeof(metadataStates)/sizeof(metadataStates[0]);

struct MetadataHashEntry { const char* label; MetadataState* state; };
static MetadataHashEntry metadataHashTable[5] = {
  {"_UPDATE_COUNTER", &metadataStates[0]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"_UPDATE_SKIP_COUNTER", &metadataStates[1]},
  {nullptr, nullptr},
};

constexpr uint16_t metadataHash(const char* s) { return labelHash(s); }

inline MetadataState* findMetadataState(const char* label) {
    uint16_t startH = metadataHash(label) % 5;
    for (uint16_t i = 0; i < 5; ++i) {
        uint16_t idx = (startH + i >= 5) ? (startH + i - 5) : (startH + i);
        const auto& entry = metadataHashTable[idx];
        if (!entry.label) continue;
        if (strcmp(entry.label, label) == 0) return entry.state;
    }
    return nullptr;
}
