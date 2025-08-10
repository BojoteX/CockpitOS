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
    {0x74C0,0x3000,12,2,"COMM1_ANT_SELECT_SW",CT_SELECTOR},
    {0x74C0,0xC000,14,2,"IFF_ANT_SELECT_SW",CT_SELECTOR},
    {0x74C2,0x0100,8,1,"APU_CONTROL_SW",CT_SELECTOR},
    {0x74C2,0x0800,11,1,"APU_READY_LT",CT_LED},
    {0x74C2,0x0600,9,2,"ENGINE_CRANK_SW",CT_SELECTOR},
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
    {0x74B4,0x4000,14,1,"FCS_RESET_BTN",CT_SELECTOR},
    {0x74BC,0x0100,8,1,"GAIN_SWITCH",CT_SELECTOR},
    {0x74B4,0x8000,15,1,"GAIN_SWITCH_COVER",CT_SELECTOR},
    {0x7528,0xFFFF,0,65535,"RUD_TRIM",CT_ANALOG},
    {0x74B4,0x2000,13,1,"TO_TRIM_BTN",CT_SELECTOR},
    {0x74B4,0x0600,9,2,"EXT_CNT_TANK_SW",CT_SELECTOR},
    {0x74B4,0x1800,11,2,"EXT_WNG_TANK_SW",CT_SELECTOR},
    {0x74B4,0x0100,8,1,"FUEL_DUMP_SW",CT_SELECTOR},
    {0x74B0,0xC000,14,2,"PROBE_SW",CT_SELECTOR},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x74C0,0x0100,8,1,"OBOGS_SW",CT_SELECTOR},
    {0x753A,0xFFFF,0,65535,"OXY_FLOW",CT_ANALOG},
    {0x74D0,0x2000,13,1,"LEFT_VIDEO_BIT",CT_SELECTOR},
    {0x74D0,0x8000,15,1,"NUC_WPN_SW",CT_SELECTOR},
    {0x74D0,0x4000,14,1,"RIGHT_VIDEO_BIT",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[6]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x74C0, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[36] }, 3 },
  { 0x74C2, { &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[4] }, 3 },
  { 0x7538, { &DcsOutputTable[5] }, 1 },
  { 0x74BC, { &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[10], &DcsOutputTable[11], &DcsOutputTable[21] }, 5 },
  { 0x74BE, { &DcsOutputTable[8], &DcsOutputTable[12], &DcsOutputTable[13] }, 3 },
  { 0x752C, { &DcsOutputTable[9] }, 1 },
  { 0x7532, { &DcsOutputTable[14] }, 1 },
  { 0x7534, { &DcsOutputTable[15] }, 1 },
  { 0x752E, { &DcsOutputTable[16] }, 1 },
  { 0x7536, { &DcsOutputTable[17] }, 1 },
  { 0x752A, { &DcsOutputTable[18] }, 1 },
  { 0x7530, { &DcsOutputTable[19] }, 1 },
  { 0x74B4, { &DcsOutputTable[20], &DcsOutputTable[22], &DcsOutputTable[24], &DcsOutputTable[25], &DcsOutputTable[26], &DcsOutputTable[27] }, 6 },
  { 0x7528, { &DcsOutputTable[23] }, 1 },
  { 0x74B0, { &DcsOutputTable[28] }, 1 },
  { 0x754A, { &DcsOutputTable[29] }, 1 },
  { 0x74C8, { &DcsOutputTable[30], &DcsOutputTable[34] }, 2 },
  { 0x7544, { &DcsOutputTable[31] }, 1 },
  { 0x7548, { &DcsOutputTable[32] }, 1 },
  { 0x7546, { &DcsOutputTable[33] }, 1 },
  { 0x754C, { &DcsOutputTable[35] }, 1 },
  { 0x753A, { &DcsOutputTable[37] }, 1 },
  { 0x74D0, { &DcsOutputTable[38], &DcsOutputTable[39], &DcsOutputTable[40] }, 3 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  { 0x752E, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x7530, &dcsAddressTable[11] },
  {0xFFFF, nullptr},
  { 0x7532, &dcsAddressTable[6] },
  { 0x74C8, &dcsAddressTable[16] },
  { 0x7534, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x7536, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x7538, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x753A, &dcsAddressTable[21] },
  { 0x74D0, &dcsAddressTable[22] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7544, &dcsAddressTable[17] },
  {0xFFFF, nullptr},
  { 0x7546, &dcsAddressTable[19] },
  {0xFFFF, nullptr},
  { 0x7548, &dcsAddressTable[18] },
  {0xFFFF, nullptr},
  { 0x754A, &dcsAddressTable[15] },
  {0xFFFF, nullptr},
  { 0x754C, &dcsAddressTable[20] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74B0, &dcsAddressTable[14] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74B4, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74BC, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x74BE, &dcsAddressTable[4] },
  { 0x7528, &dcsAddressTable[13] },
  { 0x74C0, &dcsAddressTable[0] },
  { 0x752A, &dcsAddressTable[10] },
  { 0x74C2, &dcsAddressTable[1] },
  { 0x752C, &dcsAddressTable[5] },
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
    { "COMM1_ANT_SELECT_SW_UPPER","COMM1_ANT_SELECT_SW",2,"selector",1,"UPPER" },
    { "COMM1_ANT_SELECT_SW_AUTO","COMM1_ANT_SELECT_SW",1,"selector",1,"AUTO" },
    { "COMM1_ANT_SELECT_SW_LOWER","COMM1_ANT_SELECT_SW",0,"selector",1,"LOWER" },
    { "IFF_ANT_SELECT_SW_UPPER","IFF_ANT_SELECT_SW",2,"selector",2,"UPPER" },
    { "IFF_ANT_SELECT_SW_BOTH","IFF_ANT_SELECT_SW",1,"selector",2,"BOTH" },
    { "IFF_ANT_SELECT_SW_LOWER","IFF_ANT_SELECT_SW",0,"selector",2,"LOWER" },
    { "APU_CONTROL_SW_ON","APU_CONTROL_SW",1,"action",3,"ON" },
    { "APU_CONTROL_SW_OFF","APU_CONTROL_SW",0,"action",3,"OFF" },
    { "ENGINE_CRANK_SW_LEFT","ENGINE_CRANK_SW",2,"selector",4,"LEFT" },
    { "ENGINE_CRANK_SW_OFF","ENGINE_CRANK_SW",1,"selector",4,"OFF" },
    { "ENGINE_CRANK_SW_RIGHT","ENGINE_CRANK_SW",0,"selector",4,"RIGHT" },
    { "COM_AUX","COM_AUX",65535,"analog",0,"LEVEL" },
    { "COM_COMM_G_XMT_SW_COMM_1","COM_COMM_G_XMT_SW",2,"selector",5,"COMM_1" },
    { "COM_COMM_G_XMT_SW_OFF","COM_COMM_G_XMT_SW",1,"selector",5,"OFF" },
    { "COM_COMM_G_XMT_SW_COMM_2","COM_COMM_G_XMT_SW",0,"selector",5,"COMM_2" },
    { "COM_COMM_RELAY_SW_CIPHER","COM_COMM_RELAY_SW",2,"selector",6,"CIPHER" },
    { "COM_COMM_RELAY_SW_OFF","COM_COMM_RELAY_SW",1,"selector",6,"OFF" },
    { "COM_COMM_RELAY_SW_PLAIN","COM_COMM_RELAY_SW",0,"selector",6,"PLAIN" },
    { "COM_CRYPTO_SW_HOLD","COM_CRYPTO_SW",2,"selector",7,"HOLD" },
    { "COM_CRYPTO_SW_NORM","COM_CRYPTO_SW",1,"selector",7,"NORM" },
    { "COM_CRYPTO_SW_ZERO","COM_CRYPTO_SW",0,"selector",7,"ZERO" },
    { "COM_ICS","COM_ICS",65535,"analog",0,"LEVEL" },
    { "COM_IFF_MASTER_SW_EMER","COM_IFF_MASTER_SW",1,"selector",8,"EMER" },
    { "COM_IFF_MASTER_SW_NORM","COM_IFF_MASTER_SW",0,"selector",8,"NORM" },
    { "COM_IFF_MODE4_SW_POS0","COM_IFF_MODE4_SW",0,"selector",18,"POS0" },
    { "COM_IFF_MODE4_SW_POS1","COM_IFF_MODE4_SW",1,"selector",18,"POS1" },
    { "COM_IFF_MODE4_SW_POS2","COM_IFF_MODE4_SW",2,"selector",18,"POS2" },
    { "COM_ILS_CHANNEL_SW_POS0","COM_ILS_CHANNEL_SW",0,"selector",19,"POS0" },
    { "COM_ILS_CHANNEL_SW_POS1","COM_ILS_CHANNEL_SW",1,"selector",19,"POS1" },
    { "COM_ILS_CHANNEL_SW_POS2","COM_ILS_CHANNEL_SW",2,"selector",19,"POS2" },
    { "COM_ILS_CHANNEL_SW_POS3","COM_ILS_CHANNEL_SW",3,"selector",19,"POS3" },
    { "COM_ILS_CHANNEL_SW_POS4","COM_ILS_CHANNEL_SW",4,"selector",19,"POS4" },
    { "COM_ILS_CHANNEL_SW_POS5","COM_ILS_CHANNEL_SW",5,"selector",19,"POS5" },
    { "COM_ILS_CHANNEL_SW_POS6","COM_ILS_CHANNEL_SW",6,"selector",19,"POS6" },
    { "COM_ILS_CHANNEL_SW_POS7","COM_ILS_CHANNEL_SW",7,"selector",19,"POS7" },
    { "COM_ILS_CHANNEL_SW_POS8","COM_ILS_CHANNEL_SW",8,"selector",19,"POS8" },
    { "COM_ILS_CHANNEL_SW_POS9","COM_ILS_CHANNEL_SW",9,"selector",19,"POS9" },
    { "COM_ILS_CHANNEL_SW_POS10","COM_ILS_CHANNEL_SW",10,"selector",19,"POS10" },
    { "COM_ILS_CHANNEL_SW_POS11","COM_ILS_CHANNEL_SW",11,"selector",19,"POS11" },
    { "COM_ILS_CHANNEL_SW_POS12","COM_ILS_CHANNEL_SW",12,"selector",19,"POS12" },
    { "COM_ILS_CHANNEL_SW_POS13","COM_ILS_CHANNEL_SW",13,"selector",19,"POS13" },
    { "COM_ILS_CHANNEL_SW_POS14","COM_ILS_CHANNEL_SW",14,"selector",19,"POS14" },
    { "COM_ILS_CHANNEL_SW_POS15","COM_ILS_CHANNEL_SW",15,"selector",19,"POS15" },
    { "COM_ILS_CHANNEL_SW_POS16","COM_ILS_CHANNEL_SW",16,"selector",19,"POS16" },
    { "COM_ILS_CHANNEL_SW_POS17","COM_ILS_CHANNEL_SW",17,"selector",19,"POS17" },
    { "COM_ILS_CHANNEL_SW_POS18","COM_ILS_CHANNEL_SW",18,"selector",19,"POS18" },
    { "COM_ILS_CHANNEL_SW_POS19","COM_ILS_CHANNEL_SW",19,"selector",19,"POS19" },
    { "COM_ILS_UFC_MAN_SW_UFC","COM_ILS_UFC_MAN_SW",1,"selector",9,"UFC" },
    { "COM_ILS_UFC_MAN_SW_MAN","COM_ILS_UFC_MAN_SW",0,"selector",9,"MAN" },
    { "COM_MIDS_A","COM_MIDS_A",65535,"analog",0,"LEVEL" },
    { "COM_MIDS_B","COM_MIDS_B",65535,"analog",0,"LEVEL" },
    { "COM_RWR","COM_RWR",65535,"analog",0,"LEVEL" },
    { "COM_TACAN","COM_TACAN",65535,"analog",0,"LEVEL" },
    { "COM_VOX","COM_VOX",65535,"analog",0,"LEVEL" },
    { "COM_WPN","COM_WPN",65535,"analog",0,"LEVEL" },
    { "FCS_RESET_BTN","FCS_RESET_BTN",1,"momentary",0,"PRESS" },
    { "GAIN_SWITCH_POS0","GAIN_SWITCH",0,"selector",20,"POS0" },
    { "GAIN_SWITCH_POS1","GAIN_SWITCH",1,"selector",20,"POS1" },
    { "GAIN_SWITCH_COVER","GAIN_SWITCH_COVER",1,"momentary",0,"OPEN" },
    { "RUD_TRIM","RUD_TRIM",65535,"analog",0,"LEVEL" },
    { "TO_TRIM_BTN","TO_TRIM_BTN",1,"momentary",0,"PRESS" },
    { "EXT_CNT_TANK_SW_STOP","EXT_CNT_TANK_SW",2,"selector",10,"STOP" },
    { "EXT_CNT_TANK_SW_NORM","EXT_CNT_TANK_SW",1,"selector",10,"NORM" },
    { "EXT_CNT_TANK_SW_ORIDE","EXT_CNT_TANK_SW",0,"selector",10,"ORIDE" },
    { "EXT_WNG_TANK_SW_STOP","EXT_WNG_TANK_SW",2,"selector",11,"STOP" },
    { "EXT_WNG_TANK_SW_NORM","EXT_WNG_TANK_SW",1,"selector",11,"NORM" },
    { "EXT_WNG_TANK_SW_ORIDE","EXT_WNG_TANK_SW",0,"selector",11,"ORIDE" },
    { "FUEL_DUMP_SW_ON","FUEL_DUMP_SW",1,"action",12,"ON" },
    { "FUEL_DUMP_SW_OFF","FUEL_DUMP_SW",0,"action",12,"OFF" },
    { "PROBE_SW_EXTEND","PROBE_SW",2,"selector",13,"EXTEND" },
    { "PROBE_SW_RETRACT","PROBE_SW",1,"selector",13,"RETRACT" },
    { "PROBE_SW_EMERG_EXTD","PROBE_SW",0,"selector",13,"EMERG_EXTD" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",14,"NVG" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",14,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",14,"DAY" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",15,"TEST" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",15,"OFF" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "OBOGS_SW_ON","OBOGS_SW",1,"selector",16,"ON" },
    { "OBOGS_SW_OFF","OBOGS_SW",0,"selector",16,"OFF" },
    { "OXY_FLOW","OXY_FLOW",65535,"analog",0,"LEVEL" },
    { "LEFT_VIDEO_BIT","LEFT_VIDEO_BIT",1,"momentary",0,"PRESS" },
    { "NUC_WPN_SW_ENABLE","NUC_WPN_SW",1,"selector",17,"ENABLE" },
    { "NUC_WPN_SW_DISABLE_(NO_FUNCTION)","NUC_WPN_SW",0,"selector",17,"DISABLE_(NO_FUNCTION)" },
    { "RIGHT_VIDEO_BIT","RIGHT_VIDEO_BIT",1,"momentary",0,"PRESS" },
    { "COM_ILS_CHANNEL_CUSTOM_POS0","COM_ILS_CHANNEL_CUSTOM",0,"momentary",0,"POS0" },
    { "COM_ILS_CHANNEL_CUSTOM_POS1","COM_ILS_CHANNEL_CUSTOM",1,"momentary",0,"POS1" },
    { "RUD_TRIM_CUSTOM_POS0","RUD_TRIM_CUSTOM",0,"momentary",0,"POS0" },
    { "RUD_TRIM_CUSTOM_POS1","RUD_TRIM_CUSTOM",1,"momentary",0,"POS1" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "APU_CONTROL_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 14, 0,   0, false, {0}, {0}, 0 },
    { "COMM1_ANT_SELECT_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "COM_AUX", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_COMM_G_XMT_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "COM_COMM_RELAY_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "COM_CRYPTO_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
    { "COM_ICS", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_IFF_MASTER_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "COM_IFF_MODE4_SW", 0, 0, true, 18, 0,   0, false, {0}, {0}, 0 },
    { "COM_ILS_CHANNEL_CUSTOM", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_ILS_CHANNEL_SW", 0, 0, true, 19, 0,   0, false, {0}, {0}, 0 },
    { "COM_ILS_UFC_MAN_SW", 0, 0, true, 9, 0,   0, false, {0}, {0}, 0 },
    { "COM_MIDS_A", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_MIDS_B", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_RWR", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_TACAN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_VOX", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COM_WPN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ENGINE_CRANK_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "EXT_CNT_TANK_SW", 0, 0, true, 10, 0,   0, false, {0}, {0}, 0 },
    { "EXT_WNG_TANK_SW", 0, 0, true, 11, 0,   0, false, {0}, {0}, 0 },
    { "FCS_RESET_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FUEL_DUMP_SW", 0, 0, true, 12, 0,   0, false, {0}, {0}, 0 },
    { "GAIN_SWITCH", 0, 0, true, 20, 0,   0, false, {0}, {0}, 0 },
    { "GAIN_SWITCH_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFF_ANT_SELECT_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_VIDEO_BIT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 15, 0,   0, false, {0}, {0}, 0 },
    { "NUC_WPN_SW", 0, 0, true, 17, 0,   0, false, {0}, {0}, 0 },
    { "OBOGS_SW", 0, 0, true, 16, 0,   0, false, {0}, {0}, 0 },
    { "OXY_FLOW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "PROBE_SW", 0, 0, true, 13, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_VIDEO_BIT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RUD_TRIM", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RUD_TRIM_CUSTOM", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "TO_TRIM_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "WARN_CAUTION_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
