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
    {0x7538,0xFFFF,0,65535,"COM_AUX",CT_ANALOG},
    {0x74BC,0x1800,11,2,"COM_COMM_G_XMT_SW",CT_SELECTOR},
    {0x74BC,0x0600,9,2,"COM_COMM_RELAY_SW",CT_SELECTOR},
    {0x74BE,0x0300,8,2,"COM_CRYPTO_SW",CT_SELECTOR},
    {0x752C,0xFFFF,0,65535,"COM_ICS",CT_ANALOG},
    {0x74BC,0x2000,13,1,"COM_IFF_MASTER_SW",CT_SELECTOR},
    {0x74BC,0xC000,14,2,"COM_IFF_MODE4_SW",CT_SELECTOR},
    {0x74BE,0xF800,11,19,"COM_ILS_CHANNEL_SW",CT_SELECTOR},
    {0x74BE,0x0400,10,1,"COM_ILS_UFC_MAN_SW",CT_SELECTOR},
    {0x7532,0xFFFF,0,65535,"COM_MIDS_A",CT_ANALOG},
    {0x7534,0xFFFF,0,65535,"COM_MIDS_B",CT_ANALOG},
    {0x752E,0xFFFF,0,65535,"COM_RWR",CT_ANALOG},
    {0x7536,0xFFFF,0,65535,"COM_TACAN",CT_ANALOG},
    {0x752A,0xFFFF,0,65535,"COM_VOX",CT_ANALOG},
    {0x7530,0xFFFF,0,65535,"COM_WPN",CT_ANALOG},
    {0x7488,0x0800,11,1,"AUX_REL_SW",CT_SELECTOR},
    {0x7484,0x6000,13,2,"CMSD_DISPENSE_SW",CT_SELECTOR},
    {0x7484,0x8000,15,1,"CMSD_JET_SEL_BTN",CT_SELECTOR},
    {0x74D4,0x8000,15,1,"CMSD_JET_SEL_L",CT_LED},
    {0x7488,0x0700,8,4,"ECM_MODE_SW",CT_SELECTOR},
    {0x74AC,0x0300,8,2,"FIRE_TEST_SW",CT_SELECTOR},
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
  { 0x7538, { &DcsOutputTable[0] }, 1 },
  { 0x74BC, { &DcsOutputTable[1], &DcsOutputTable[2], &DcsOutputTable[5], &DcsOutputTable[6] }, 4 },
  { 0x74BE, { &DcsOutputTable[3], &DcsOutputTable[7], &DcsOutputTable[8] }, 3 },
  { 0x752C, { &DcsOutputTable[4] }, 1 },
  { 0x7532, { &DcsOutputTable[9] }, 1 },
  { 0x7534, { &DcsOutputTable[10] }, 1 },
  { 0x752E, { &DcsOutputTable[11] }, 1 },
  { 0x7536, { &DcsOutputTable[12] }, 1 },
  { 0x752A, { &DcsOutputTable[13] }, 1 },
  { 0x7530, { &DcsOutputTable[14] }, 1 },
  { 0x7488, { &DcsOutputTable[15], &DcsOutputTable[19] }, 2 },
  { 0x7484, { &DcsOutputTable[16], &DcsOutputTable[17] }, 2 },
  { 0x74D4, { &DcsOutputTable[18] }, 1 },
  { 0x74AC, { &DcsOutputTable[20] }, 1 },
  { 0x7456, { &DcsOutputTable[21] }, 1 },
  { 0x742A, { &DcsOutputTable[22], &DcsOutputTable[23], &DcsOutputTable[24], &DcsOutputTable[25] }, 4 },
  { 0x740C, { &DcsOutputTable[26], &DcsOutputTable[27], &DcsOutputTable[28], &DcsOutputTable[29], &DcsOutputTable[30], &DcsOutputTable[31], &DcsOutputTable[32] }, 7 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  { 0x752E, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x7530, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x7532, &dcsAddressTable[4] },
  { 0x742A, &dcsAddressTable[15] },
  { 0x7534, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x7536, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x7538, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74D4, &dcsAddressTable[12] },
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
  { 0x740C, &dcsAddressTable[16] },
  { 0x74AC, &dcsAddressTable[13] },
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
  { 0x7484, &dcsAddressTable[11] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74BC, &dcsAddressTable[1] },
  { 0x7488, &dcsAddressTable[10] },
  { 0x74BE, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x752A, &dcsAddressTable[8] },
  { 0x7456, &dcsAddressTable[14] },
  { 0x752C, &dcsAddressTable[3] },
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
    { "COM_AUX","COM_AUX",65535,"analog",0,"LEVEL" },
    { "COM_AUX_DEC","COM_AUX",0,"variable_step",0,"DEC" },
    { "COM_AUX_INC","COM_AUX",1,"variable_step",0,"INC" },
    { "COM_COMM_G_XMT_SW_COMM_2","COM_COMM_G_XMT_SW",0,"selector",1,"COMM_2" },
    { "COM_COMM_G_XMT_SW_OFF","COM_COMM_G_XMT_SW",1,"selector",1,"OFF" },
    { "COM_COMM_G_XMT_SW_COMM_1","COM_COMM_G_XMT_SW",2,"selector",1,"COMM_1" },
    { "COM_COMM_RELAY_SW_PLAIN","COM_COMM_RELAY_SW",0,"selector",2,"PLAIN" },
    { "COM_COMM_RELAY_SW_OFF","COM_COMM_RELAY_SW",1,"selector",2,"OFF" },
    { "COM_COMM_RELAY_SW_CIPHER","COM_COMM_RELAY_SW",2,"selector",2,"CIPHER" },
    { "COM_CRYPTO_SW_ZERO","COM_CRYPTO_SW",0,"selector",3,"ZERO" },
    { "COM_CRYPTO_SW_NORM","COM_CRYPTO_SW",1,"selector",3,"NORM" },
    { "COM_CRYPTO_SW_HOLD","COM_CRYPTO_SW",2,"selector",3,"HOLD" },
    { "COM_ICS","COM_ICS",65535,"analog",0,"LEVEL" },
    { "COM_ICS_DEC","COM_ICS",0,"variable_step",0,"DEC" },
    { "COM_ICS_INC","COM_ICS",1,"variable_step",0,"INC" },
    { "COM_IFF_MASTER_SW_NORM","COM_IFF_MASTER_SW",0,"selector",4,"NORM" },
    { "COM_IFF_MASTER_SW_EMER","COM_IFF_MASTER_SW",1,"selector",4,"EMER" },
    { "COM_IFF_MODE4_SW_POS0","COM_IFF_MODE4_SW",0,"selector",12,"POS0" },
    { "COM_IFF_MODE4_SW_POS1","COM_IFF_MODE4_SW",1,"selector",12,"POS1" },
    { "COM_IFF_MODE4_SW_POS2","COM_IFF_MODE4_SW",2,"selector",12,"POS2" },
    { "COM_ILS_CHANNEL_SW_POS0","COM_ILS_CHANNEL_SW",0,"selector",13,"POS0" },
    { "COM_ILS_CHANNEL_SW_POS1","COM_ILS_CHANNEL_SW",1,"selector",13,"POS1" },
    { "COM_ILS_CHANNEL_SW_POS2","COM_ILS_CHANNEL_SW",2,"selector",13,"POS2" },
    { "COM_ILS_CHANNEL_SW_POS3","COM_ILS_CHANNEL_SW",3,"selector",13,"POS3" },
    { "COM_ILS_CHANNEL_SW_POS4","COM_ILS_CHANNEL_SW",4,"selector",13,"POS4" },
    { "COM_ILS_CHANNEL_SW_POS5","COM_ILS_CHANNEL_SW",5,"selector",13,"POS5" },
    { "COM_ILS_CHANNEL_SW_POS6","COM_ILS_CHANNEL_SW",6,"selector",13,"POS6" },
    { "COM_ILS_CHANNEL_SW_POS7","COM_ILS_CHANNEL_SW",7,"selector",13,"POS7" },
    { "COM_ILS_CHANNEL_SW_POS8","COM_ILS_CHANNEL_SW",8,"selector",13,"POS8" },
    { "COM_ILS_CHANNEL_SW_POS9","COM_ILS_CHANNEL_SW",9,"selector",13,"POS9" },
    { "COM_ILS_CHANNEL_SW_POS10","COM_ILS_CHANNEL_SW",10,"selector",13,"POS10" },
    { "COM_ILS_CHANNEL_SW_POS11","COM_ILS_CHANNEL_SW",11,"selector",13,"POS11" },
    { "COM_ILS_CHANNEL_SW_POS12","COM_ILS_CHANNEL_SW",12,"selector",13,"POS12" },
    { "COM_ILS_CHANNEL_SW_POS13","COM_ILS_CHANNEL_SW",13,"selector",13,"POS13" },
    { "COM_ILS_CHANNEL_SW_POS14","COM_ILS_CHANNEL_SW",14,"selector",13,"POS14" },
    { "COM_ILS_CHANNEL_SW_POS15","COM_ILS_CHANNEL_SW",15,"selector",13,"POS15" },
    { "COM_ILS_CHANNEL_SW_POS16","COM_ILS_CHANNEL_SW",16,"selector",13,"POS16" },
    { "COM_ILS_CHANNEL_SW_POS17","COM_ILS_CHANNEL_SW",17,"selector",13,"POS17" },
    { "COM_ILS_CHANNEL_SW_POS18","COM_ILS_CHANNEL_SW",18,"selector",13,"POS18" },
    { "COM_ILS_CHANNEL_SW_POS19","COM_ILS_CHANNEL_SW",19,"selector",13,"POS19" },
    { "COM_ILS_CHANNEL_SW_DEC","COM_ILS_CHANNEL_SW",0,"fixed_step",0,"DEC" },
    { "COM_ILS_CHANNEL_SW_INC","COM_ILS_CHANNEL_SW",1,"fixed_step",0,"INC" },
    { "COM_ILS_UFC_MAN_SW_MAN","COM_ILS_UFC_MAN_SW",0,"selector",5,"MAN" },
    { "COM_ILS_UFC_MAN_SW_UFC","COM_ILS_UFC_MAN_SW",1,"selector",5,"UFC" },
    { "COM_MIDS_A","COM_MIDS_A",65535,"analog",0,"LEVEL" },
    { "COM_MIDS_A_DEC","COM_MIDS_A",0,"variable_step",0,"DEC" },
    { "COM_MIDS_A_INC","COM_MIDS_A",1,"variable_step",0,"INC" },
    { "COM_MIDS_B","COM_MIDS_B",65535,"analog",0,"LEVEL" },
    { "COM_MIDS_B_DEC","COM_MIDS_B",0,"variable_step",0,"DEC" },
    { "COM_MIDS_B_INC","COM_MIDS_B",1,"variable_step",0,"INC" },
    { "COM_RWR","COM_RWR",65535,"analog",0,"LEVEL" },
    { "COM_RWR_DEC","COM_RWR",0,"variable_step",0,"DEC" },
    { "COM_RWR_INC","COM_RWR",1,"variable_step",0,"INC" },
    { "COM_TACAN","COM_TACAN",65535,"analog",0,"LEVEL" },
    { "COM_TACAN_DEC","COM_TACAN",0,"variable_step",0,"DEC" },
    { "COM_TACAN_INC","COM_TACAN",1,"variable_step",0,"INC" },
    { "COM_VOX","COM_VOX",65535,"analog",0,"LEVEL" },
    { "COM_VOX_DEC","COM_VOX",0,"variable_step",0,"DEC" },
    { "COM_VOX_INC","COM_VOX",1,"variable_step",0,"INC" },
    { "COM_WPN","COM_WPN",65535,"analog",0,"LEVEL" },
    { "COM_WPN_DEC","COM_WPN",0,"variable_step",0,"DEC" },
    { "COM_WPN_INC","COM_WPN",1,"variable_step",0,"INC" },
    { "AUX_REL_SW_NORM","AUX_REL_SW",0,"selector",6,"NORM" },
    { "AUX_REL_SW_ENABLE","AUX_REL_SW",1,"selector",6,"ENABLE" },
    { "CMSD_DISPENSE_SW_OFF","CMSD_DISPENSE_SW",0,"selector",7,"OFF" },
    { "CMSD_DISPENSE_SW_ON","CMSD_DISPENSE_SW",1,"selector",7,"ON" },
    { "CMSD_DISPENSE_SW_BYPASS","CMSD_DISPENSE_SW",2,"selector",7,"BYPASS" },
    { "CMSD_JET_SEL_BTN","CMSD_JET_SEL_BTN",1,"momentary",0,"PRESS" },
    { "ECM_MODE_SW_OFF","ECM_MODE_SW",0,"selector",8,"OFF" },
    { "ECM_MODE_SW_STBY","ECM_MODE_SW",1,"selector",8,"STBY" },
    { "ECM_MODE_SW_BIT","ECM_MODE_SW",2,"selector",8,"BIT" },
    { "ECM_MODE_SW_REC","ECM_MODE_SW",3,"selector",8,"REC" },
    { "ECM_MODE_SW_XMIT","ECM_MODE_SW",4,"selector",8,"XMIT" },
    { "FIRE_TEST_SW_POS0","FIRE_TEST_SW",0,"selector",14,"POS0" },
    { "FIRE_TEST_SW_POS1","FIRE_TEST_SW",1,"selector",14,"POS1" },
    { "FIRE_TEST_SW_POS2","FIRE_TEST_SW",2,"selector",14,"POS2" },
    { "HMD_OFF_BRT","HMD_OFF_BRT",65535,"analog",0,"LEVEL" },
    { "HMD_OFF_BRT_DEC","HMD_OFF_BRT",0,"variable_step",0,"DEC" },
    { "HMD_OFF_BRT_INC","HMD_OFF_BRT",1,"variable_step",0,"INC" },
    { "IR_COOL_SW_OFF","IR_COOL_SW",0,"selector",9,"OFF" },
    { "IR_COOL_SW_NORM","IR_COOL_SW",1,"selector",9,"NORM" },
    { "IR_COOL_SW_ORIDE","IR_COOL_SW",2,"selector",9,"ORIDE" },
    { "SPIN_RECOVERY_COVER","SPIN_RECOVERY_COVER",1,"momentary",0,"OPEN" },
    { "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_SW",0,"selector",10,"NORM" },
    { "SPIN_RECOVERY_SW_RCVY","SPIN_RECOVERY_SW",1,"selector",10,"RCVY" },
    { "MASTER_ARM_SW_SAFE","MASTER_ARM_SW",0,"selector",11,"SAFE" },
    { "MASTER_ARM_SW_ARM","MASTER_ARM_SW",1,"selector",11,"ARM" },
    { "MASTER_MODE_AA","MASTER_MODE_AA",1,"momentary",0,"PRESS" },
    { "MASTER_MODE_AG","MASTER_MODE_AG",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "AUX_REL_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_DISPENSE_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_JET_SEL_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_AUX", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_COMM_G_XMT_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "COM_COMM_RELAY_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "COM_CRYPTO_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "COM_ICS", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_IFF_MASTER_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "COM_IFF_MODE4_SW", 0, 0, true, 12, 0,   0, false, {0}, {0}, 0 },
    { "COM_ILS_CHANNEL_SW", 0, 0, true, 13, 0,   0, false, {0}, {0}, 0 },
    { "COM_ILS_UFC_MAN_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "COM_MIDS_A", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_MIDS_B", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_RWR", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_TACAN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_VOX", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_WPN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ECM_MODE_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "FIRE_TEST_SW", 0, 0, true, 14, 0,   0, false, {0}, {0}, 0 },
    { "HMD_OFF_BRT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IR_COOL_SW", 0, 0, true, 9, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_ARM_SW", 0, 0, true, 11, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_MODE_AA", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_MODE_AG", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_SW", 0, 0, true, 10, 0,   0, false, {0}, {0}, 0 },
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
