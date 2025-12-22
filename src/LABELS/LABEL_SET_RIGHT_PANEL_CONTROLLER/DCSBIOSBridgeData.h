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
    {0x7550,0xFFFF,0,65535,"DEFOG_HANDLE",CT_ANALOG},
    {0x74CC,0x6000,13,2,"WSHIELD_ANTI_ICE_SW",CT_SELECTOR},
    {0x74C4,0x1800,11,2,"BATTERY_SW",CT_SELECTOR},
    {0x74C4,0x2000,13,1,"L_GEN_SW",CT_SELECTOR},
    {0x74C4,0x4000,14,1,"R_GEN_SW",CT_SELECTOR},
    {0x753E,0xFFFF,0,65535,"VOLT_E",CT_GAUGE},
    {0x753C,0xFFFF,0,65535,"VOLT_U",CT_GAUGE},
    {0x74C6,0x0300,8,3,"BLEED_AIR_KNOB",CT_SELECTOR},
    {0x74C4,0x8000,15,1,"BLEED_AIR_PULL",CT_SELECTOR},
    {0x74C6,0x3000,12,2,"CABIN_PRESS_SW",CT_SELECTOR},
    {0x7540,0xFFFF,0,65535,"CABIN_TEMP",CT_ANALOG},
    {0x74C6,0x0C00,10,2,"ECS_MODE_SW",CT_SELECTOR},
    {0x74C6,0xC000,14,2,"ENG_ANTIICE_SW",CT_SELECTOR},
    {0x74C8,0x0100,8,1,"PITOT_HEAT_SW",CT_SELECTOR},
    {0x7542,0xFFFF,0,65535,"SUIT_TEMP",CT_ANALOG},
    {0x7576,0xFFFF,0,65535,"EXT_FORMATION_LIGHTS",CT_METADATA},
    {0x7586,0xFFFF,0,65535,"EXT_HOOK",CT_METADATA},
    {0x757A,0xFFFF,0,65535,"EXT_NOZZLE_POS_L",CT_METADATA},
    {0x7578,0xFFFF,0,65535,"EXT_NOZZLE_POS_R",CT_METADATA},
    {0x74D6,0x0400,10,1,"EXT_POSITION_LIGHT_LEFT",CT_METADATA},
    {0x74D6,0x0800,11,1,"EXT_POSITION_LIGHT_RIGHT",CT_METADATA},
    {0x7574,0xFFFF,0,65535,"EXT_REFUEL_PROBE",CT_METADATA},
    {0x74D6,0x0200,9,1,"EXT_REFUEL_PROBE_LIGHT",CT_METADATA},
    {0x756E,0xFFFF,0,65535,"EXT_SPEED_BRAKE",CT_METADATA},
    {0x7572,0xFFFF,0,65535,"EXT_STAIR",CT_METADATA},
    {0x74D6,0x2000,13,1,"EXT_STROBE_LIGHTS",CT_METADATA},
    {0x74D6,0x1000,12,1,"EXT_TAIL_LIGHT",CT_METADATA},
    {0x7570,0xFFFF,0,65535,"EXT_WING_FOLDING",CT_METADATA},
    {0x74D8,0x0100,8,1,"EXT_WOW_LEFT",CT_METADATA},
    {0x74D6,0x4000,14,1,"EXT_WOW_NOSE",CT_METADATA},
    {0x74D6,0x8000,15,1,"EXT_WOW_RIGHT",CT_METADATA},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x7552,0xFFFF,0,65535,"CANOPY_POS",CT_GAUGE},
    {0x74CE,0x0300,8,2,"CANOPY_SW",CT_SELECTOR},
    {0x74CC,0x8000,15,1,"CB_FCS_CHAN3",CT_SELECTOR},
    {0x74CE,0x0400,10,1,"CB_FCS_CHAN4",CT_SELECTOR},
    {0x74CE,0x0800,11,1,"CB_HOOOK",CT_SELECTOR},
    {0x74CE,0x1000,12,1,"CB_LG",CT_SELECTOR},
    {0x74CE,0x2000,13,1,"FCS_BIT_SW",CT_SELECTOR},
    {0x74C8,0x3000,12,2,"FLIR_SW",CT_SELECTOR},
    {0x74CA,0x3800,11,7,"INS_SW",CT_SELECTOR},
    {0x74C8,0x8000,15,1,"LST_NFLR_SW",CT_SELECTOR},
    {0x74C8,0x4000,14,1,"LTD_R_SW",CT_SELECTOR},
    {0x74CA,0x0300,8,3,"RADAR_SW",CT_SELECTOR},
    {0x74CA,0x0400,10,1,"RADAR_SW_PULL",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[7]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x7550, { &DcsOutputTable[0] }, 1 },
  { 0x74CC, { &DcsOutputTable[1], &DcsOutputTable[40] }, 2 },
  { 0x74C4, { &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[4], &DcsOutputTable[8] }, 4 },
  { 0x753E, { &DcsOutputTable[5] }, 1 },
  { 0x753C, { &DcsOutputTable[6] }, 1 },
  { 0x74C6, { &DcsOutputTable[7], &DcsOutputTable[9], &DcsOutputTable[11], &DcsOutputTable[12] }, 4 },
  { 0x7540, { &DcsOutputTable[10] }, 1 },
  { 0x74C8, { &DcsOutputTable[13], &DcsOutputTable[32], &DcsOutputTable[36], &DcsOutputTable[45], &DcsOutputTable[47], &DcsOutputTable[48] }, 6 },
  { 0x7542, { &DcsOutputTable[14] }, 1 },
  { 0x7576, { &DcsOutputTable[15] }, 1 },
  { 0x7586, { &DcsOutputTable[16] }, 1 },
  { 0x757A, { &DcsOutputTable[17] }, 1 },
  { 0x7578, { &DcsOutputTable[18] }, 1 },
  { 0x74D6, { &DcsOutputTable[19], &DcsOutputTable[20], &DcsOutputTable[22], &DcsOutputTable[25], &DcsOutputTable[26], &DcsOutputTable[29], &DcsOutputTable[30] }, 7 },
  { 0x7574, { &DcsOutputTable[21] }, 1 },
  { 0x756E, { &DcsOutputTable[23] }, 1 },
  { 0x7572, { &DcsOutputTable[24] }, 1 },
  { 0x7570, { &DcsOutputTable[27] }, 1 },
  { 0x74D8, { &DcsOutputTable[28] }, 1 },
  { 0x754A, { &DcsOutputTable[31] }, 1 },
  { 0x7544, { &DcsOutputTable[33] }, 1 },
  { 0x7548, { &DcsOutputTable[34] }, 1 },
  { 0x7546, { &DcsOutputTable[35] }, 1 },
  { 0x754C, { &DcsOutputTable[37] }, 1 },
  { 0x7552, { &DcsOutputTable[38] }, 1 },
  { 0x74CE, { &DcsOutputTable[39], &DcsOutputTable[41], &DcsOutputTable[42], &DcsOutputTable[43], &DcsOutputTable[44] }, 5 },
  { 0x74CA, { &DcsOutputTable[46], &DcsOutputTable[49], &DcsOutputTable[50] }, 3 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[59] = {
  {0xFFFF, nullptr},
  { 0x7550, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  { 0x7552, &dcsAddressTable[24] },
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
  { 0x756E, &dcsAddressTable[15] },
  {0xFFFF, nullptr},
  { 0x7570, &dcsAddressTable[17] },
  {0xFFFF, nullptr},
  { 0x7572, &dcsAddressTable[16] },
  {0xFFFF, nullptr},
  { 0x7574, &dcsAddressTable[14] },
  { 0x74C4, &dcsAddressTable[2] },
  { 0x7576, &dcsAddressTable[9] },
  { 0x753C, &dcsAddressTable[4] },
  { 0x74C6, &dcsAddressTable[5] },
  { 0x753E, &dcsAddressTable[3] },
  { 0x74C8, &dcsAddressTable[7] },
  { 0x7540, &dcsAddressTable[6] },
  { 0x757A, &dcsAddressTable[11] },
  { 0x74CC, &dcsAddressTable[1] },
  { 0x7542, &dcsAddressTable[8] },
  { 0x7578, &dcsAddressTable[12] },
  { 0x7544, &dcsAddressTable[20] },
  { 0x7546, &dcsAddressTable[22] },
  { 0x74CE, &dcsAddressTable[25] },
  { 0x7548, &dcsAddressTable[21] },
  { 0x74CA, &dcsAddressTable[26] },
  { 0x754A, &dcsAddressTable[19] },
  { 0x7586, &dcsAddressTable[10] },
  { 0x74D6, &dcsAddressTable[13] },
  { 0x754C, &dcsAddressTable[23] },
  { 0x74D8, &dcsAddressTable[18] },
};

// Simple address hash (modulo)
constexpr uint16_t addrHash(uint16_t addr) {
  return addr % 59;
}

inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {
  uint16_t startH = addrHash(addr);
  for (uint16_t i = 0; i < 59; ++i) {
    uint16_t idx = (startH + i >= 59) ? (startH + i - 59) : (startH + i);
    const auto& entry = dcsAddressHashTable[idx];
    if (entry.addr == 0xFFFF) continue;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "DEFOG_HANDLE","DEFOG_HANDLE",65535,"analog",0,"LEVEL" },
    { "DEFOG_HANDLE_DEC","DEFOG_HANDLE",0,"variable_step",0,"DEC" },
    { "DEFOG_HANDLE_INC","DEFOG_HANDLE",1,"variable_step",0,"INC" },
    { "WSHIELD_ANTI_ICE_SW_RAIN","WSHIELD_ANTI_ICE_SW",0,"selector",1,"RAIN" },
    { "WSHIELD_ANTI_ICE_SW_OFF","WSHIELD_ANTI_ICE_SW",1,"selector",1,"OFF" },
    { "WSHIELD_ANTI_ICE_SW_ANTI_ICE","WSHIELD_ANTI_ICE_SW",2,"selector",1,"ANTI_ICE" },
    { "BATTERY_SW_ORIDE","BATTERY_SW",0,"selector",2,"ORIDE" },
    { "BATTERY_SW_OFF","BATTERY_SW",1,"selector",2,"OFF" },
    { "BATTERY_SW_ON","BATTERY_SW",2,"selector",2,"ON" },
    { "L_GEN_SW_OFF","L_GEN_SW",0,"selector",3,"OFF" },
    { "L_GEN_SW_NORM","L_GEN_SW",1,"selector",3,"NORM" },
    { "R_GEN_SW_OFF","R_GEN_SW",0,"selector",4,"OFF" },
    { "R_GEN_SW_NORM","R_GEN_SW",1,"selector",4,"NORM" },
    { "BLEED_AIR_KNOB_OFF","BLEED_AIR_KNOB",0,"selector",5,"OFF" },
    { "BLEED_AIR_KNOB_L_OFF","BLEED_AIR_KNOB",1,"selector",5,"L_OFF" },
    { "BLEED_AIR_KNOB_NORM","BLEED_AIR_KNOB",2,"selector",5,"NORM" },
    { "BLEED_AIR_KNOB_R_OFF","BLEED_AIR_KNOB",3,"selector",5,"R_OFF" },
    { "BLEED_AIR_KNOB_DEC","BLEED_AIR_KNOB",0,"fixed_step",0,"DEC" },
    { "BLEED_AIR_KNOB_INC","BLEED_AIR_KNOB",1,"fixed_step",0,"INC" },
    { "BLEED_AIR_PULL_POS0","BLEED_AIR_PULL",0,"selector",16,"POS0" },
    { "BLEED_AIR_PULL_POS1","BLEED_AIR_PULL",1,"selector",16,"POS1" },
    { "CABIN_PRESS_SW_POS0","CABIN_PRESS_SW",0,"selector",17,"POS0" },
    { "CABIN_PRESS_SW_POS1","CABIN_PRESS_SW",1,"selector",17,"POS1" },
    { "CABIN_PRESS_SW_POS2","CABIN_PRESS_SW",2,"selector",17,"POS2" },
    { "CABIN_TEMP","CABIN_TEMP",65535,"analog",0,"LEVEL" },
    { "CABIN_TEMP_DEC","CABIN_TEMP",0,"variable_step",0,"DEC" },
    { "CABIN_TEMP_INC","CABIN_TEMP",1,"variable_step",0,"INC" },
    { "ECS_MODE_SW_POS0","ECS_MODE_SW",0,"selector",18,"POS0" },
    { "ECS_MODE_SW_POS1","ECS_MODE_SW",1,"selector",18,"POS1" },
    { "ECS_MODE_SW_POS2","ECS_MODE_SW",2,"selector",18,"POS2" },
    { "ENG_ANTIICE_SW_TEST","ENG_ANTIICE_SW",0,"selector",6,"TEST" },
    { "ENG_ANTIICE_SW_OFF","ENG_ANTIICE_SW",1,"selector",6,"OFF" },
    { "ENG_ANTIICE_SW_ON","ENG_ANTIICE_SW",2,"selector",6,"ON" },
    { "PITOT_HEAT_SW_AUTO","PITOT_HEAT_SW",0,"selector",7,"AUTO" },
    { "PITOT_HEAT_SW_ON","PITOT_HEAT_SW",1,"selector",7,"ON" },
    { "SUIT_TEMP","SUIT_TEMP",65535,"analog",0,"LEVEL" },
    { "SUIT_TEMP_DEC","SUIT_TEMP",0,"variable_step",0,"DEC" },
    { "SUIT_TEMP_INC","SUIT_TEMP",1,"variable_step",0,"INC" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "CHART_DIMMER_DEC","CHART_DIMMER",0,"variable_step",0,"DEC" },
    { "CHART_DIMMER_INC","CHART_DIMMER",1,"variable_step",0,"INC" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",8,"DAY" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",8,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",8,"NVG" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "CONSOLES_DIMMER_DEC","CONSOLES_DIMMER",0,"variable_step",0,"DEC" },
    { "CONSOLES_DIMMER_INC","CONSOLES_DIMMER",1,"variable_step",0,"INC" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER_DEC","FLOOD_DIMMER",0,"variable_step",0,"DEC" },
    { "FLOOD_DIMMER_INC","FLOOD_DIMMER",1,"variable_step",0,"INC" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER_DEC","INST_PNL_DIMMER",0,"variable_step",0,"DEC" },
    { "INST_PNL_DIMMER_INC","INST_PNL_DIMMER",1,"variable_step",0,"INC" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",9,"OFF" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",9,"TEST" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "WARN_CAUTION_DIMMER_DEC","WARN_CAUTION_DIMMER",0,"variable_step",0,"DEC" },
    { "WARN_CAUTION_DIMMER_INC","WARN_CAUTION_DIMMER",1,"variable_step",0,"INC" },
    { "CANOPY_SW_CLOSE","CANOPY_SW",0,"selector",10,"CLOSE" },
    { "CANOPY_SW_HOLD","CANOPY_SW",1,"selector",10,"HOLD" },
    { "CANOPY_SW_OPEN","CANOPY_SW",2,"selector",10,"OPEN" },
    { "CB_FCS_CHAN3","CB_FCS_CHAN3",1,"momentary",0,"PRESS" },
    { "CB_FCS_CHAN4","CB_FCS_CHAN4",1,"momentary",0,"PRESS" },
    { "CB_HOOOK","CB_HOOOK",1,"momentary",0,"PRESS" },
    { "CB_LG","CB_LG",1,"momentary",0,"PRESS" },
    { "FCS_BIT_SW_PRESS","FCS_BIT_SW",0,"selector",19,"PRESS" },
    { "FCS_BIT_SW_RELEASE","FCS_BIT_SW",1,"selector",19,"RELEASE" },
    { "FLIR_SW_OFF","FLIR_SW",0,"selector",11,"OFF" },
    { "FLIR_SW_STBY","FLIR_SW",1,"selector",11,"STBY" },
    { "FLIR_SW_ON","FLIR_SW",2,"selector",11,"ON" },
    { "INS_SW_TEST","INS_SW",0,"selector",12,"TEST" },
    { "INS_SW_GB","INS_SW",1,"selector",12,"GB" },
    { "INS_SW_GYRO","INS_SW",2,"selector",12,"GYRO" },
    { "INS_SW_IFA","INS_SW",3,"selector",12,"IFA" },
    { "INS_SW_NAV","INS_SW",4,"selector",12,"NAV" },
    { "INS_SW_GND","INS_SW",5,"selector",12,"GND" },
    { "INS_SW_CV","INS_SW",6,"selector",12,"CV" },
    { "INS_SW_OFF","INS_SW",7,"selector",12,"OFF" },
    { "INS_SW_DEC","INS_SW",0,"fixed_step",0,"DEC" },
    { "INS_SW_INC","INS_SW",1,"fixed_step",0,"INC" },
    { "LST_NFLR_SW_OFF","LST_NFLR_SW",0,"selector",13,"OFF" },
    { "LST_NFLR_SW_ON","LST_NFLR_SW",1,"selector",13,"ON" },
    { "LTD_R_SW_SAFE","LTD_R_SW",0,"selector",14,"SAFE" },
    { "LTD_R_SW_ARM","LTD_R_SW",1,"selector",14,"ARM" },
    { "RADAR_SW_EMERG(PULL)","RADAR_SW",0,"selector",15,"EMERG(PULL)" },
    { "RADAR_SW_OPR","RADAR_SW",1,"selector",15,"OPR" },
    { "RADAR_SW_STBY","RADAR_SW",2,"selector",15,"STBY" },
    { "RADAR_SW_OFF","RADAR_SW",3,"selector",15,"OFF" },
    { "RADAR_SW_DEC","RADAR_SW",0,"fixed_step",0,"DEC" },
    { "RADAR_SW_INC","RADAR_SW",1,"fixed_step",0,"INC" },
    { "RADAR_SW_PULL","RADAR_SW_PULL",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "BATTERY_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "BLEED_AIR_KNOB", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "BLEED_AIR_PULL", 0, 0, true, 16, 0,   0, false, {0}, {0}, 0 },
    { "CABIN_PRESS_SW", 0, 0, true, 17, 0,   0, false, {0}, {0}, 0 },
    { "CABIN_TEMP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CANOPY_SW", 0, 0, true, 10, 0,   0, false, {0}, {0}, 0 },
    { "CB_FCS_CHAN3", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CB_FCS_CHAN4", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CB_HOOOK", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CB_LG", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "DEFOG_HANDLE", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ECS_MODE_SW", 0, 0, true, 18, 0,   0, false, {0}, {0}, 0 },
    { "ENG_ANTIICE_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "FCS_BIT_SW", 0, 0, true, 19, 0,   0, false, {0}, {0}, 0 },
    { "FLIR_SW", 0, 0, true, 11, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INS_SW", 0, 0, true, 12, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 9, 0,   0, false, {0}, {0}, 0 },
    { "LST_NFLR_SW", 0, 0, true, 13, 0,   0, false, {0}, {0}, 0 },
    { "LTD_R_SW", 0, 0, true, 14, 0,   0, false, {0}, {0}, 0 },
    { "L_GEN_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "PITOT_HEAT_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
    { "RADAR_SW", 0, 0, true, 15, 0,   0, false, {0}, {0}, 0 },
    { "RADAR_SW_PULL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "R_GEN_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "SUIT_TEMP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "WARN_CAUTION_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "WSHIELD_ANTI_ICE_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
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


// --- Auto-generated tracked metadata fields ---
struct MetadataState {
    const char* label;
    uint16_t    value;
};
static MetadataState metadataStates[] = {
    { "EXT_FORMATION_LIGHTS", 0 }, // External Aircraft Model: Formation Lights (light green)
    { "EXT_HOOK", 0 }, // External Aircraft Model: Hook
    { "EXT_NOZZLE_POS_L", 0 }, // External Aircraft Model: Left Nozzle Position
    { "EXT_NOZZLE_POS_R", 0 }, // External Aircraft Model: Right Nozzle Position
    { "EXT_POSITION_LIGHT_LEFT", 0 }, // External Aircraft Model: Left Position Light (red)
    { "EXT_POSITION_LIGHT_RIGHT", 0 }, // External Aircraft Model: Right Position Light (green)
    { "EXT_REFUEL_PROBE", 0 }, // External Aircraft Model: Refuel Probe
    { "EXT_REFUEL_PROBE_LIGHT", 0 }, // External Aircraft Model: Refuel Probe Light (white)
    { "EXT_SPEED_BRAKE", 0 }, // External Aircraft Model: Speed Brake
    { "EXT_STAIR", 0 }, // External Aircraft Model: Stair
    { "EXT_STROBE_LIGHTS", 0 }, // External Aircraft Model: Strobe Lights (red)
    { "EXT_TAIL_LIGHT", 0 }, // External Aircraft Model: Tail Light (white)
    { "EXT_WING_FOLDING", 0 }, // External Aircraft Model: Wing Folding
    { "EXT_WOW_LEFT", 0 }, // External Aircraft Model: Weight ON Wheels Left Gear
    { "EXT_WOW_NOSE", 0 }, // External Aircraft Model: Weight ON Wheels Nose Gear
    { "EXT_WOW_RIGHT", 0 }, // External Aircraft Model: Weight ON Wheels Right Gear
};
static const size_t numMetadataStates = sizeof(metadataStates)/sizeof(metadataStates[0]);

struct MetadataHashEntry { const char* label; MetadataState* state; };
static MetadataHashEntry metadataHashTable[37] = {
  {"EXT_REFUEL_PROBE_LIGHT", &metadataStates[7]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_WING_FOLDING", &metadataStates[12]},
  {"EXT_STAIR", &metadataStates[9]},
  {nullptr, nullptr},
  {"EXT_FORMATION_LIGHTS", &metadataStates[0]},
  {nullptr, nullptr},
  {"EXT_WOW_RIGHT", &metadataStates[15]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_TAIL_LIGHT", &metadataStates[11]},
  {"EXT_REFUEL_PROBE", &metadataStates[6]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_POSITION_LIGHT_LEFT", &metadataStates[4]},
  {"EXT_HOOK", &metadataStates[1]},
  {"EXT_STROBE_LIGHTS", &metadataStates[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_SPEED_BRAKE", &metadataStates[8]},
  {"EXT_WOW_LEFT", &metadataStates[13]},
  {nullptr, nullptr},
  {"EXT_NOZZLE_POS_L", &metadataStates[2]},
  {"EXT_WOW_NOSE", &metadataStates[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_NOZZLE_POS_R", &metadataStates[3]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"EXT_POSITION_LIGHT_RIGHT", &metadataStates[5]},
};

constexpr uint16_t metadataHash(const char* s) { return labelHash(s); }

inline MetadataState* findMetadataState(const char* label) {
    uint16_t startH = metadataHash(label) % 37;
    for (uint16_t i = 0; i < 37; ++i) {
        uint16_t idx = (startH + i >= 37) ? (startH + i - 37) : (startH + i);
        const auto& entry = metadataHashTable[idx];
        if (!entry.label) continue;
        if (strcmp(entry.label, label) == 0) return entry.state;
    }
    return nullptr;
}
