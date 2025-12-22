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
    {0x7416,0x0200,9,1,"UFC_0",CT_SELECTOR},
    {0x7416,0x0400,10,1,"UFC_1",CT_SELECTOR},
    {0x7416,0x0800,11,1,"UFC_2",CT_SELECTOR},
    {0x7416,0x1000,12,1,"UFC_3",CT_SELECTOR},
    {0x7416,0x2000,13,1,"UFC_4",CT_SELECTOR},
    {0x7416,0x4000,14,1,"UFC_5",CT_SELECTOR},
    {0x7416,0x8000,15,1,"UFC_6",CT_SELECTOR},
    {0x7418,0x0001,0,1,"UFC_7",CT_SELECTOR},
    {0x7418,0x0002,1,1,"UFC_8",CT_SELECTOR},
    {0x7418,0x0004,2,1,"UFC_9",CT_SELECTOR},
    {0x7416,0x00C0,6,2,"UFC_ADF",CT_SELECTOR},
    {0x7414,0x0080,7,1,"UFC_AP",CT_SELECTOR},
    {0x7414,0x1000,12,1,"UFC_BCN",CT_SELECTOR},
    {0x741E,0xFFFF,0,65535,"UFC_BRT",CT_ANALOG},
    {0x7418,0x0008,3,1,"UFC_CLR",CT_SELECTOR},
    {0x7424,0x00FF,0,2,"UFC_COMM1_DISPLAY",CT_DISPLAY},
    {0x7425,0x00FF,0,2,"UFC_COMM1_DISPLAY",CT_DISPLAY},
    {0x7414,0x4000,14,1,"UFC_COMM1_PULL",CT_SELECTOR},
    {0x741A,0xFFFF,0,65535,"UFC_COMM1_VOL",CT_ANALOG},
    {0x7426,0x00FF,0,2,"UFC_COMM2_DISPLAY",CT_DISPLAY},
    {0x7427,0x00FF,0,2,"UFC_COMM2_DISPLAY",CT_DISPLAY},
    {0x7414,0x8000,15,1,"UFC_COMM2_PULL",CT_SELECTOR},
    {0x741C,0xFFFF,0,65535,"UFC_COMM2_VOL",CT_ANALOG},
    {0x7414,0x0800,11,1,"UFC_DL",CT_SELECTOR},
    {0x7416,0x0100,8,1,"UFC_EMCON",CT_SELECTOR},
    {0x7418,0x0010,4,1,"UFC_ENT",CT_SELECTOR},
    {0x7414,0x0100,8,1,"UFC_IFF",CT_SELECTOR},
    {0x7414,0x0400,10,1,"UFC_ILS",CT_SELECTOR},
    {0x7416,0x0020,5,1,"UFC_IP",CT_SELECTOR},
    {0x7414,0x2000,13,1,"UFC_ONOFF",CT_SELECTOR},
    {0x7428,0x00FF,0,1,"UFC_OPTION_CUEING_1",CT_DISPLAY},
    {0x742A,0x00FF,0,1,"UFC_OPTION_CUEING_2",CT_DISPLAY},
    {0x742C,0x00FF,0,1,"UFC_OPTION_CUEING_3",CT_DISPLAY},
    {0x742E,0x00FF,0,1,"UFC_OPTION_CUEING_4",CT_DISPLAY},
    {0x7430,0x00FF,0,1,"UFC_OPTION_CUEING_5",CT_DISPLAY},
    {0x7432,0x00FF,0,4,"UFC_OPTION_DISPLAY_1",CT_DISPLAY},
    {0x7433,0x00FF,0,4,"UFC_OPTION_DISPLAY_1",CT_DISPLAY},
    {0x7434,0x00FF,0,4,"UFC_OPTION_DISPLAY_1",CT_DISPLAY},
    {0x7435,0x00FF,0,4,"UFC_OPTION_DISPLAY_1",CT_DISPLAY},
    {0x7436,0x00FF,0,4,"UFC_OPTION_DISPLAY_2",CT_DISPLAY},
    {0x7437,0x00FF,0,4,"UFC_OPTION_DISPLAY_2",CT_DISPLAY},
    {0x7438,0x00FF,0,4,"UFC_OPTION_DISPLAY_2",CT_DISPLAY},
    {0x7439,0x00FF,0,4,"UFC_OPTION_DISPLAY_2",CT_DISPLAY},
    {0x743A,0x00FF,0,4,"UFC_OPTION_DISPLAY_3",CT_DISPLAY},
    {0x743B,0x00FF,0,4,"UFC_OPTION_DISPLAY_3",CT_DISPLAY},
    {0x743C,0x00FF,0,4,"UFC_OPTION_DISPLAY_3",CT_DISPLAY},
    {0x743D,0x00FF,0,4,"UFC_OPTION_DISPLAY_3",CT_DISPLAY},
    {0x743E,0x00FF,0,4,"UFC_OPTION_DISPLAY_4",CT_DISPLAY},
    {0x743F,0x00FF,0,4,"UFC_OPTION_DISPLAY_4",CT_DISPLAY},
    {0x7440,0x00FF,0,4,"UFC_OPTION_DISPLAY_4",CT_DISPLAY},
    {0x7441,0x00FF,0,4,"UFC_OPTION_DISPLAY_4",CT_DISPLAY},
    {0x7442,0x00FF,0,4,"UFC_OPTION_DISPLAY_5",CT_DISPLAY},
    {0x7443,0x00FF,0,4,"UFC_OPTION_DISPLAY_5",CT_DISPLAY},
    {0x7444,0x00FF,0,4,"UFC_OPTION_DISPLAY_5",CT_DISPLAY},
    {0x7445,0x00FF,0,4,"UFC_OPTION_DISPLAY_5",CT_DISPLAY},
    {0x7416,0x0001,0,1,"UFC_OS1",CT_SELECTOR},
    {0x7416,0x0002,1,1,"UFC_OS2",CT_SELECTOR},
    {0x7416,0x0004,2,1,"UFC_OS3",CT_SELECTOR},
    {0x7416,0x0008,3,1,"UFC_OS4",CT_SELECTOR},
    {0x7416,0x0010,4,1,"UFC_OS5",CT_SELECTOR},
    {0x7446,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x7447,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x7448,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x7449,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x744A,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x744B,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x744C,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x744D,0x00FF,0,8,"UFC_SCRATCHPAD_NUMBER_DISPLAY",CT_DISPLAY},
    {0x744E,0x00FF,0,2,"UFC_SCRATCHPAD_STRING_1_DISPLAY",CT_DISPLAY},
    {0x744F,0x00FF,0,2,"UFC_SCRATCHPAD_STRING_1_DISPLAY",CT_DISPLAY},
    {0x7450,0x00FF,0,2,"UFC_SCRATCHPAD_STRING_2_DISPLAY",CT_DISPLAY},
    {0x7451,0x00FF,0,2,"UFC_SCRATCHPAD_STRING_2_DISPLAY",CT_DISPLAY},
    {0x7414,0x0200,9,1,"UFC_TCN",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[15]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x7416, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[4], &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[10], &DcsOutputTable[24], &DcsOutputTable[28], &DcsOutputTable[55], &DcsOutputTable[56], &DcsOutputTable[57], &DcsOutputTable[58], &DcsOutputTable[59] }, 15 },
  { 0x7418, { &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[9], &DcsOutputTable[14], &DcsOutputTable[25] }, 5 },
  { 0x7414, { &DcsOutputTable[11], &DcsOutputTable[12], &DcsOutputTable[17], &DcsOutputTable[21], &DcsOutputTable[23], &DcsOutputTable[26], &DcsOutputTable[27], &DcsOutputTable[29], &DcsOutputTable[72] }, 9 },
  { 0x741E, { &DcsOutputTable[13] }, 1 },
  { 0x7424, { &DcsOutputTable[15] }, 1 },
  { 0x7425, { &DcsOutputTable[16] }, 1 },
  { 0x741A, { &DcsOutputTable[18] }, 1 },
  { 0x7426, { &DcsOutputTable[19] }, 1 },
  { 0x7427, { &DcsOutputTable[20] }, 1 },
  { 0x741C, { &DcsOutputTable[22] }, 1 },
  { 0x7428, { &DcsOutputTable[30] }, 1 },
  { 0x742A, { &DcsOutputTable[31] }, 1 },
  { 0x742C, { &DcsOutputTable[32] }, 1 },
  { 0x742E, { &DcsOutputTable[33] }, 1 },
  { 0x7430, { &DcsOutputTable[34] }, 1 },
  { 0x7432, { &DcsOutputTable[35] }, 1 },
  { 0x7433, { &DcsOutputTable[36] }, 1 },
  { 0x7434, { &DcsOutputTable[37] }, 1 },
  { 0x7435, { &DcsOutputTable[38] }, 1 },
  { 0x7436, { &DcsOutputTable[39] }, 1 },
  { 0x7437, { &DcsOutputTable[40] }, 1 },
  { 0x7438, { &DcsOutputTable[41] }, 1 },
  { 0x7439, { &DcsOutputTable[42] }, 1 },
  { 0x743A, { &DcsOutputTable[43] }, 1 },
  { 0x743B, { &DcsOutputTable[44] }, 1 },
  { 0x743C, { &DcsOutputTable[45] }, 1 },
  { 0x743D, { &DcsOutputTable[46] }, 1 },
  { 0x743E, { &DcsOutputTable[47] }, 1 },
  { 0x743F, { &DcsOutputTable[48] }, 1 },
  { 0x7440, { &DcsOutputTable[49] }, 1 },
  { 0x7441, { &DcsOutputTable[50] }, 1 },
  { 0x7442, { &DcsOutputTable[51] }, 1 },
  { 0x7443, { &DcsOutputTable[52] }, 1 },
  { 0x7444, { &DcsOutputTable[53] }, 1 },
  { 0x7445, { &DcsOutputTable[54] }, 1 },
  { 0x7446, { &DcsOutputTable[60] }, 1 },
  { 0x7447, { &DcsOutputTable[61] }, 1 },
  { 0x7448, { &DcsOutputTable[62] }, 1 },
  { 0x7449, { &DcsOutputTable[63] }, 1 },
  { 0x744A, { &DcsOutputTable[64] }, 1 },
  { 0x744B, { &DcsOutputTable[65] }, 1 },
  { 0x744C, { &DcsOutputTable[66] }, 1 },
  { 0x744D, { &DcsOutputTable[67] }, 1 },
  { 0x744E, { &DcsOutputTable[68] }, 1 },
  { 0x744F, { &DcsOutputTable[69] }, 1 },
  { 0x7450, { &DcsOutputTable[70] }, 1 },
  { 0x7451, { &DcsOutputTable[71] }, 1 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[97] = {
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
  { 0x7414, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x7416, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  { 0x7418, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  { 0x741A, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x741C, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x741E, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7424, &dcsAddressTable[4] },
  { 0x7425, &dcsAddressTable[5] },
  { 0x7426, &dcsAddressTable[7] },
  { 0x7427, &dcsAddressTable[8] },
  { 0x7428, &dcsAddressTable[10] },
  {0xFFFF, nullptr},
  { 0x742A, &dcsAddressTable[11] },
  {0xFFFF, nullptr},
  { 0x742C, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  { 0x742E, &dcsAddressTable[13] },
  {0xFFFF, nullptr},
  { 0x7430, &dcsAddressTable[14] },
  {0xFFFF, nullptr},
  { 0x7432, &dcsAddressTable[15] },
  { 0x7433, &dcsAddressTable[16] },
  { 0x7434, &dcsAddressTable[17] },
  { 0x7435, &dcsAddressTable[18] },
  { 0x7436, &dcsAddressTable[19] },
  { 0x7437, &dcsAddressTable[20] },
  { 0x7438, &dcsAddressTable[21] },
  { 0x7439, &dcsAddressTable[22] },
  { 0x743A, &dcsAddressTable[23] },
  { 0x743B, &dcsAddressTable[24] },
  { 0x743C, &dcsAddressTable[25] },
  { 0x743D, &dcsAddressTable[26] },
  { 0x743E, &dcsAddressTable[27] },
  { 0x743F, &dcsAddressTable[28] },
  { 0x7440, &dcsAddressTable[29] },
  { 0x7441, &dcsAddressTable[30] },
  { 0x7442, &dcsAddressTable[31] },
  { 0x7443, &dcsAddressTable[32] },
  { 0x7444, &dcsAddressTable[33] },
  { 0x7445, &dcsAddressTable[34] },
  { 0x7446, &dcsAddressTable[35] },
  { 0x7447, &dcsAddressTable[36] },
  { 0x7448, &dcsAddressTable[37] },
  { 0x7449, &dcsAddressTable[38] },
  { 0x744A, &dcsAddressTable[39] },
  { 0x744B, &dcsAddressTable[40] },
  { 0x744C, &dcsAddressTable[41] },
  { 0x744D, &dcsAddressTable[42] },
  { 0x744E, &dcsAddressTable[43] },
  { 0x744F, &dcsAddressTable[44] },
  { 0x7450, &dcsAddressTable[45] },
  { 0x7451, &dcsAddressTable[46] },
  {0xFFFF, nullptr},
};

// Simple address hash (modulo)
constexpr uint16_t addrHash(uint16_t addr) {
  return addr % 97;
}

inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {
  uint16_t startH = addrHash(addr);
  for (uint16_t i = 0; i < 97; ++i) {
    uint16_t idx = (startH + i >= 97) ? (startH + i - 97) : (startH + i);
    const auto& entry = dcsAddressHashTable[idx];
    if (entry.addr == 0xFFFF) continue;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "UFC_0","UFC_0",1,"momentary",0,"PRESS" },
    { "UFC_1","UFC_1",1,"momentary",0,"PRESS" },
    { "UFC_2","UFC_2",1,"momentary",0,"PRESS" },
    { "UFC_3","UFC_3",1,"momentary",0,"PRESS" },
    { "UFC_4","UFC_4",1,"momentary",0,"PRESS" },
    { "UFC_5","UFC_5",1,"momentary",0,"PRESS" },
    { "UFC_6","UFC_6",1,"momentary",0,"PRESS" },
    { "UFC_7","UFC_7",1,"momentary",0,"PRESS" },
    { "UFC_8","UFC_8",1,"momentary",0,"PRESS" },
    { "UFC_9","UFC_9",1,"momentary",0,"PRESS" },
    { "UFC_ADF_2","UFC_ADF",0,"selector",1,"2" },
    { "UFC_ADF_OFF","UFC_ADF",1,"selector",1,"OFF" },
    { "UFC_ADF_1","UFC_ADF",2,"selector",1,"1" },
    { "UFC_AP","UFC_AP",1,"momentary",0,"PRESS" },
    { "UFC_BCN","UFC_BCN",1,"momentary",0,"PRESS" },
    { "UFC_BRT","UFC_BRT",65535,"analog",0,"LEVEL" },
    { "UFC_BRT_DEC","UFC_BRT",0,"variable_step",0,"DEC" },
    { "UFC_BRT_INC","UFC_BRT",1,"variable_step",0,"INC" },
    { "UFC_CLR","UFC_CLR",1,"momentary",0,"PRESS" },
    { "UFC_COMM1_PULL","UFC_COMM1_PULL",1,"momentary",0,"PRESS" },
    { "UFC_COMM1_VOL","UFC_COMM1_VOL",65535,"analog",0,"LEVEL" },
    { "UFC_COMM1_VOL_DEC","UFC_COMM1_VOL",0,"variable_step",0,"DEC" },
    { "UFC_COMM1_VOL_INC","UFC_COMM1_VOL",1,"variable_step",0,"INC" },
    { "UFC_COMM2_PULL","UFC_COMM2_PULL",1,"momentary",0,"PRESS" },
    { "UFC_COMM2_VOL","UFC_COMM2_VOL",65535,"analog",0,"LEVEL" },
    { "UFC_COMM2_VOL_DEC","UFC_COMM2_VOL",0,"variable_step",0,"DEC" },
    { "UFC_COMM2_VOL_INC","UFC_COMM2_VOL",1,"variable_step",0,"INC" },
    { "UFC_DL","UFC_DL",1,"momentary",0,"PRESS" },
    { "UFC_EMCON","UFC_EMCON",1,"momentary",0,"PRESS" },
    { "UFC_ENT","UFC_ENT",1,"momentary",0,"PRESS" },
    { "UFC_IFF","UFC_IFF",1,"momentary",0,"PRESS" },
    { "UFC_ILS","UFC_ILS",1,"momentary",0,"PRESS" },
    { "UFC_IP","UFC_IP",1,"momentary",0,"PRESS" },
    { "UFC_ONOFF","UFC_ONOFF",1,"momentary",0,"PRESS" },
    { "UFC_OS1","UFC_OS1",1,"momentary",0,"PRESS" },
    { "UFC_OS2","UFC_OS2",1,"momentary",0,"PRESS" },
    { "UFC_OS3","UFC_OS3",1,"momentary",0,"PRESS" },
    { "UFC_OS4","UFC_OS4",1,"momentary",0,"PRESS" },
    { "UFC_OS5","UFC_OS5",1,"momentary",0,"PRESS" },
    { "UFC_TCN","UFC_TCN",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "UFC_0", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_1", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_2", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_3", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_4", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_5", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_6", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_7", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_8", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_9", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_ADF", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "UFC_AP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_BCN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_BRT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_CLR", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_COMM1_PULL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_COMM1_VOL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_COMM2_PULL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_COMM2_VOL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_DL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_EMCON", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_ENT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_IFF", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_ILS", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_IP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_ONOFF", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_OS1", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_OS2", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_OS3", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_OS4", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_OS5", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "UFC_TCN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
    { "Up Front Controller (UFC)", "UFC_COMM1_DISPLAY", 0x7424, 2 },
    { "Up Front Controller (UFC)", "UFC_COMM2_DISPLAY", 0x7426, 2 },
    { "Up Front Controller (UFC)", "UFC_OPTION_CUEING_1", 0x7428, 1 },
    { "Up Front Controller (UFC)", "UFC_OPTION_CUEING_2", 0x742A, 1 },
    { "Up Front Controller (UFC)", "UFC_OPTION_CUEING_3", 0x742C, 1 },
    { "Up Front Controller (UFC)", "UFC_OPTION_CUEING_4", 0x742E, 1 },
    { "Up Front Controller (UFC)", "UFC_OPTION_CUEING_5", 0x7430, 1 },
    { "Up Front Controller (UFC)", "UFC_OPTION_DISPLAY_1", 0x7432, 4 },
    { "Up Front Controller (UFC)", "UFC_OPTION_DISPLAY_2", 0x7436, 4 },
    { "Up Front Controller (UFC)", "UFC_OPTION_DISPLAY_3", 0x743A, 4 },
    { "Up Front Controller (UFC)", "UFC_OPTION_DISPLAY_4", 0x743E, 4 },
    { "Up Front Controller (UFC)", "UFC_OPTION_DISPLAY_5", 0x7442, 4 },
    { "Up Front Controller (UFC)", "UFC_SCRATCHPAD_NUMBER_DISPLAY", 0x7446, 8 },
    { "Up Front Controller (UFC)", "UFC_SCRATCHPAD_STRING_1_DISPLAY", 0x744E, 2 },
    { "Up Front Controller (UFC)", "UFC_SCRATCHPAD_STRING_2_DISPLAY", 0x7450, 2 },
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[31] = {
  {"UFC_OPTION_CUEING_2", &displayFields[3]},
  {"UFC_OPTION_CUEING_3", &displayFields[4]},
  {"UFC_OPTION_CUEING_4", &displayFields[5]},
  {"UFC_OPTION_CUEING_5", &displayFields[6]},
  {"UFC_COMM1_DISPLAY", &displayFields[0]},
  {"UFC_OPTION_DISPLAY_1", &displayFields[7]},
  {"UFC_OPTION_DISPLAY_2", &displayFields[8]},
  {"UFC_OPTION_DISPLAY_3", &displayFields[9]},
  {"UFC_OPTION_DISPLAY_4", &displayFields[10]},
  {"UFC_OPTION_DISPLAY_5", &displayFields[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_COMM2_DISPLAY", &displayFields[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_SCRATCHPAD_NUMBER_DISPLAY", &displayFields[12]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_SCRATCHPAD_STRING_1_DISPLAY", &displayFields[13]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_SCRATCHPAD_STRING_2_DISPLAY", &displayFields[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"UFC_OPTION_CUEING_1", &displayFields[2]},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 31;
  for (uint16_t i = 0; i < 31; ++i) {
    uint16_t idx = (startH + i >= 31) ? (startH + i - 31) : (startH + i);
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
