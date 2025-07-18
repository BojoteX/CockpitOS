// Auto-generated DCSBIOS Bridge Data (JSON‑only) - DO NOT EDIT
#pragma once

// #include <stdint.h>
#include "../CT_Display.h"

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
    {0x7514,0xFFFF,0,65535,"PRESSURE_ALT",CT_GAUGE},
    {0x7488,0x0800,11,1,"AUX_REL_SW",CT_SELECTOR},
    {0x7484,0x6000,13,2,"CMSD_DISPENSE_SW",CT_SELECTOR},
    {0x7484,0x8000,15,1,"CMSD_JET_SEL_BTN",CT_SELECTOR},
    {0x74D4,0x8000,15,1,"CMSD_JET_SEL_L",CT_LED},
    {0x7488,0x0700,8,4,"ECM_MODE_SW",CT_SELECTOR},
    {0x740E,0x0001,0,1,"FIRE_EXT_BTN",CT_SELECTOR},
    {0x7468,0x00FF,0,5,"IFEI_BINGO",CT_DISPLAY},
    {0x7469,0x00FF,0,5,"IFEI_BINGO",CT_DISPLAY},
    {0x746A,0x00FF,0,5,"IFEI_BINGO",CT_DISPLAY},
    {0x746B,0x00FF,0,5,"IFEI_BINGO",CT_DISPLAY},
    {0x746C,0x00FF,0,5,"IFEI_BINGO",CT_DISPLAY},
    {0x74C6,0x00FF,0,1,"IFEI_BINGO_TEXTURE",CT_DISPLAY},
    {0x746E,0x00FF,0,2,"IFEI_CLOCK_H",CT_DISPLAY},
    {0x746F,0x00FF,0,2,"IFEI_CLOCK_H",CT_DISPLAY},
    {0x7470,0x00FF,0,2,"IFEI_CLOCK_M",CT_DISPLAY},
    {0x7471,0x00FF,0,2,"IFEI_CLOCK_M",CT_DISPLAY},
    {0x7472,0x00FF,0,2,"IFEI_CLOCK_S",CT_DISPLAY},
    {0x7473,0x00FF,0,2,"IFEI_CLOCK_S",CT_DISPLAY},
    {0x74AE,0x00FF,0,3,"IFEI_CODES",CT_DISPLAY},
    {0x74AF,0x00FF,0,3,"IFEI_CODES",CT_DISPLAY},
    {0x74B0,0x00FF,0,3,"IFEI_CODES",CT_DISPLAY},
    {0x747A,0x00FF,0,1,"IFEI_DD_1",CT_DISPLAY},
    {0x747C,0x00FF,0,1,"IFEI_DD_2",CT_DISPLAY},
    {0x747E,0x00FF,0,1,"IFEI_DD_3",CT_DISPLAY},
    {0x7480,0x00FF,0,1,"IFEI_DD_4",CT_DISPLAY},
    {0x7466,0x0010,4,1,"IFEI_DWN_BTN",CT_SELECTOR},
    {0x7466,0x0040,6,1,"IFEI_ET_BTN",CT_SELECTOR},
    {0x7482,0x00FF,0,3,"IFEI_FF_L",CT_DISPLAY},
    {0x7483,0x00FF,0,3,"IFEI_FF_L",CT_DISPLAY},
    {0x7484,0x00FF,0,3,"IFEI_FF_L",CT_DISPLAY},
    {0x7486,0x00FF,0,3,"IFEI_FF_R",CT_DISPLAY},
    {0x7487,0x00FF,0,3,"IFEI_FF_R",CT_DISPLAY},
    {0x7488,0x00FF,0,3,"IFEI_FF_R",CT_DISPLAY},
    {0x74C0,0x00FF,0,1,"IFEI_FF_TEXTURE",CT_DISPLAY},
    {0x748A,0x00FF,0,6,"IFEI_FUEL_DOWN",CT_DISPLAY},
    {0x748B,0x00FF,0,6,"IFEI_FUEL_DOWN",CT_DISPLAY},
    {0x748C,0x00FF,0,6,"IFEI_FUEL_DOWN",CT_DISPLAY},
    {0x748D,0x00FF,0,6,"IFEI_FUEL_DOWN",CT_DISPLAY},
    {0x748E,0x00FF,0,6,"IFEI_FUEL_DOWN",CT_DISPLAY},
    {0x748F,0x00FF,0,6,"IFEI_FUEL_DOWN",CT_DISPLAY},
    {0x7490,0x00FF,0,6,"IFEI_FUEL_UP",CT_DISPLAY},
    {0x7491,0x00FF,0,6,"IFEI_FUEL_UP",CT_DISPLAY},
    {0x7492,0x00FF,0,6,"IFEI_FUEL_UP",CT_DISPLAY},
    {0x7493,0x00FF,0,6,"IFEI_FUEL_UP",CT_DISPLAY},
    {0x7494,0x00FF,0,6,"IFEI_FUEL_UP",CT_DISPLAY},
    {0x7495,0x00FF,0,6,"IFEI_FUEL_UP",CT_DISPLAY},
    {0x74CC,0x00FF,0,1,"IFEI_L0_TEXTURE",CT_DISPLAY},
    {0x74D4,0x00FF,0,1,"IFEI_L100_TEXTURE",CT_DISPLAY},
    {0x74D0,0x00FF,0,1,"IFEI_L50_TEXTURE",CT_DISPLAY},
    {0x74D8,0x00FF,0,1,"IFEI_LPOINTER_TEXTURE",CT_DISPLAY},
    {0x74C8,0x00FF,0,1,"IFEI_LSCALE_TEXTURE",CT_DISPLAY},
    {0x7582,0x00FF,0,1,"IFEI_L_TEXTURE",CT_DISPLAY},
    {0x7466,0x0002,1,1,"IFEI_MODE_BTN",CT_SELECTOR},
    {0x74C2,0x00FF,0,1,"IFEI_NOZ_TEXTURE",CT_DISPLAY},
    {0x7496,0x00FF,0,3,"IFEI_OIL_PRESS_L",CT_DISPLAY},
    {0x7497,0x00FF,0,3,"IFEI_OIL_PRESS_L",CT_DISPLAY},
    {0x7498,0x00FF,0,3,"IFEI_OIL_PRESS_L",CT_DISPLAY},
    {0x749A,0x00FF,0,3,"IFEI_OIL_PRESS_R",CT_DISPLAY},
    {0x749B,0x00FF,0,3,"IFEI_OIL_PRESS_R",CT_DISPLAY},
    {0x749C,0x00FF,0,3,"IFEI_OIL_PRESS_R",CT_DISPLAY},
    {0x74C4,0x00FF,0,1,"IFEI_OIL_TEXTURE",CT_DISPLAY},
    {0x7466,0x0004,2,1,"IFEI_QTY_BTN",CT_SELECTOR},
    {0x74CE,0x00FF,0,1,"IFEI_R0_TEXTURE",CT_DISPLAY},
    {0x74D6,0x00FF,0,1,"IFEI_R100_TEXTURE",CT_DISPLAY},
    {0x74D2,0x00FF,0,1,"IFEI_R50_TEXTURE",CT_DISPLAY},
    {0x749E,0x00FF,0,3,"IFEI_RPM_L",CT_DISPLAY},
    {0x749F,0x00FF,0,3,"IFEI_RPM_L",CT_DISPLAY},
    {0x74A0,0x00FF,0,3,"IFEI_RPM_L",CT_DISPLAY},
    {0x74A2,0x00FF,0,3,"IFEI_RPM_R",CT_DISPLAY},
    {0x74A3,0x00FF,0,3,"IFEI_RPM_R",CT_DISPLAY},
    {0x74A4,0x00FF,0,3,"IFEI_RPM_R",CT_DISPLAY},
    {0x74BC,0x00FF,0,1,"IFEI_RPM_TEXTURE",CT_DISPLAY},
    {0x74DA,0x00FF,0,1,"IFEI_RPOINTER_TEXTURE",CT_DISPLAY},
    {0x74CA,0x00FF,0,1,"IFEI_RSCALE_TEXTURE",CT_DISPLAY},
    {0x7584,0x00FF,0,1,"IFEI_R_TEXTURE",CT_DISPLAY},
    {0x74B2,0x00FF,0,3,"IFEI_SP",CT_DISPLAY},
    {0x74B3,0x00FF,0,3,"IFEI_SP",CT_DISPLAY},
    {0x74B4,0x00FF,0,3,"IFEI_SP",CT_DISPLAY},
    {0x757C,0x00FF,0,6,"IFEI_T",CT_DISPLAY},
    {0x757D,0x00FF,0,6,"IFEI_T",CT_DISPLAY},
    {0x757E,0x00FF,0,6,"IFEI_T",CT_DISPLAY},
    {0x757F,0x00FF,0,6,"IFEI_T",CT_DISPLAY},
    {0x7580,0x00FF,0,6,"IFEI_T",CT_DISPLAY},
    {0x7581,0x00FF,0,6,"IFEI_T",CT_DISPLAY},
    {0x74A6,0x00FF,0,3,"IFEI_TEMP_L",CT_DISPLAY},
    {0x74A7,0x00FF,0,3,"IFEI_TEMP_L",CT_DISPLAY},
    {0x74A8,0x00FF,0,3,"IFEI_TEMP_L",CT_DISPLAY},
    {0x74AA,0x00FF,0,3,"IFEI_TEMP_R",CT_DISPLAY},
    {0x74AB,0x00FF,0,3,"IFEI_TEMP_R",CT_DISPLAY},
    {0x74AC,0x00FF,0,3,"IFEI_TEMP_R",CT_DISPLAY},
    {0x74BE,0x00FF,0,1,"IFEI_TEMP_TEXTURE",CT_DISPLAY},
    {0x7474,0x00FF,0,2,"IFEI_TIMER_H",CT_DISPLAY},
    {0x7475,0x00FF,0,2,"IFEI_TIMER_H",CT_DISPLAY},
    {0x7476,0x00FF,0,2,"IFEI_TIMER_M",CT_DISPLAY},
    {0x7477,0x00FF,0,2,"IFEI_TIMER_M",CT_DISPLAY},
    {0x7478,0x00FF,0,2,"IFEI_TIMER_S",CT_DISPLAY},
    {0x7479,0x00FF,0,2,"IFEI_TIMER_S",CT_DISPLAY},
    {0x74B6,0x00FF,0,6,"IFEI_TIME_SET_MODE",CT_DISPLAY},
    {0x74B7,0x00FF,0,6,"IFEI_TIME_SET_MODE",CT_DISPLAY},
    {0x74B8,0x00FF,0,6,"IFEI_TIME_SET_MODE",CT_DISPLAY},
    {0x74B9,0x00FF,0,6,"IFEI_TIME_SET_MODE",CT_DISPLAY},
    {0x74BA,0x00FF,0,6,"IFEI_TIME_SET_MODE",CT_DISPLAY},
    {0x74BB,0x00FF,0,6,"IFEI_TIME_SET_MODE",CT_DISPLAY},
    {0x7466,0x0008,3,1,"IFEI_UP_BTN",CT_SELECTOR},
    {0x7466,0x0020,5,1,"IFEI_ZONE_BTN",CT_SELECTOR},
    {0x74DC,0x00FF,0,1,"IFEI_Z_TEXTURE",CT_DISPLAY},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
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
    {0x7408,0x0001,0,1,"LS_LOCK",CT_LED},
    {0x7408,0x0002,1,1,"LS_SHOOT",CT_LED},
    {0x7408,0x0004,2,1,"LS_SHOOT_STROBE",CT_LED},
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
  { 0x740C, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[150], &DcsOutputTable[151], &DcsOutputTable[152], &DcsOutputTable[153], &DcsOutputTable[154], &DcsOutputTable[155], &DcsOutputTable[156], &DcsOutputTable[168], &DcsOutputTable[169], &DcsOutputTable[170], &DcsOutputTable[171], &DcsOutputTable[172] }, 14 },
  { 0x74A4, { &DcsOutputTable[2], &DcsOutputTable[3], &DcsOutputTable[5], &DcsOutputTable[6], &DcsOutputTable[7], &DcsOutputTable[8], &DcsOutputTable[11], &DcsOutputTable[12], &DcsOutputTable[85] }, 9 },
  { 0x74A0, { &DcsOutputTable[4], &DcsOutputTable[82] }, 2 },
  { 0x74A8, { &DcsOutputTable[9], &DcsOutputTable[10], &DcsOutputTable[13], &DcsOutputTable[101] }, 4 },
  { 0x7514, { &DcsOutputTable[14] }, 1 },
  { 0x7488, { &DcsOutputTable[15], &DcsOutputTable[19], &DcsOutputTable[47] }, 3 },
  { 0x7484, { &DcsOutputTable[16], &DcsOutputTable[17], &DcsOutputTable[44] }, 3 },
  { 0x74D4, { &DcsOutputTable[18], &DcsOutputTable[62] }, 2 },
  { 0x740E, { &DcsOutputTable[20] }, 1 },
  { 0x7468, { &DcsOutputTable[21] }, 1 },
  { 0x7469, { &DcsOutputTable[22] }, 1 },
  { 0x746A, { &DcsOutputTable[23] }, 1 },
  { 0x746B, { &DcsOutputTable[24] }, 1 },
  { 0x746C, { &DcsOutputTable[25] }, 1 },
  { 0x74C6, { &DcsOutputTable[26] }, 1 },
  { 0x746E, { &DcsOutputTable[27] }, 1 },
  { 0x746F, { &DcsOutputTable[28] }, 1 },
  { 0x7470, { &DcsOutputTable[29] }, 1 },
  { 0x7471, { &DcsOutputTable[30] }, 1 },
  { 0x7472, { &DcsOutputTable[31] }, 1 },
  { 0x7473, { &DcsOutputTable[32] }, 1 },
  { 0x74AE, { &DcsOutputTable[33] }, 1 },
  { 0x74AF, { &DcsOutputTable[34] }, 1 },
  { 0x74B0, { &DcsOutputTable[35] }, 1 },
  { 0x747A, { &DcsOutputTable[36] }, 1 },
  { 0x747C, { &DcsOutputTable[37] }, 1 },
  { 0x747E, { &DcsOutputTable[38] }, 1 },
  { 0x7480, { &DcsOutputTable[39] }, 1 },
  { 0x7466, { &DcsOutputTable[40], &DcsOutputTable[41], &DcsOutputTable[67], &DcsOutputTable[76], &DcsOutputTable[118], &DcsOutputTable[119] }, 6 },
  { 0x7482, { &DcsOutputTable[42] }, 1 },
  { 0x7483, { &DcsOutputTable[43] }, 1 },
  { 0x7486, { &DcsOutputTable[45] }, 1 },
  { 0x7487, { &DcsOutputTable[46] }, 1 },
  { 0x74C0, { &DcsOutputTable[48] }, 1 },
  { 0x748A, { &DcsOutputTable[49] }, 1 },
  { 0x748B, { &DcsOutputTable[50] }, 1 },
  { 0x748C, { &DcsOutputTable[51] }, 1 },
  { 0x748D, { &DcsOutputTable[52] }, 1 },
  { 0x748E, { &DcsOutputTable[53] }, 1 },
  { 0x748F, { &DcsOutputTable[54] }, 1 },
  { 0x7490, { &DcsOutputTable[55] }, 1 },
  { 0x7491, { &DcsOutputTable[56] }, 1 },
  { 0x7492, { &DcsOutputTable[57] }, 1 },
  { 0x7493, { &DcsOutputTable[58] }, 1 },
  { 0x7494, { &DcsOutputTable[59] }, 1 },
  { 0x7495, { &DcsOutputTable[60] }, 1 },
  { 0x74CC, { &DcsOutputTable[61] }, 1 },
  { 0x74D0, { &DcsOutputTable[63] }, 1 },
  { 0x74D8, { &DcsOutputTable[64] }, 1 },
  { 0x74C8, { &DcsOutputTable[65], &DcsOutputTable[122], &DcsOutputTable[126] }, 3 },
  { 0x7582, { &DcsOutputTable[66] }, 1 },
  { 0x74C2, { &DcsOutputTable[68] }, 1 },
  { 0x7496, { &DcsOutputTable[69] }, 1 },
  { 0x7497, { &DcsOutputTable[70] }, 1 },
  { 0x7498, { &DcsOutputTable[71] }, 1 },
  { 0x749A, { &DcsOutputTable[72] }, 1 },
  { 0x749B, { &DcsOutputTable[73] }, 1 },
  { 0x749C, { &DcsOutputTable[74] }, 1 },
  { 0x74C4, { &DcsOutputTable[75] }, 1 },
  { 0x74CE, { &DcsOutputTable[77] }, 1 },
  { 0x74D6, { &DcsOutputTable[78] }, 1 },
  { 0x74D2, { &DcsOutputTable[79] }, 1 },
  { 0x749E, { &DcsOutputTable[80] }, 1 },
  { 0x749F, { &DcsOutputTable[81] }, 1 },
  { 0x74A2, { &DcsOutputTable[83] }, 1 },
  { 0x74A3, { &DcsOutputTable[84] }, 1 },
  { 0x74BC, { &DcsOutputTable[86] }, 1 },
  { 0x74DA, { &DcsOutputTable[87] }, 1 },
  { 0x74CA, { &DcsOutputTable[88] }, 1 },
  { 0x7584, { &DcsOutputTable[89] }, 1 },
  { 0x74B2, { &DcsOutputTable[90] }, 1 },
  { 0x74B3, { &DcsOutputTable[91] }, 1 },
  { 0x74B4, { &DcsOutputTable[92] }, 1 },
  { 0x757C, { &DcsOutputTable[93] }, 1 },
  { 0x757D, { &DcsOutputTable[94] }, 1 },
  { 0x757E, { &DcsOutputTable[95] }, 1 },
  { 0x757F, { &DcsOutputTable[96] }, 1 },
  { 0x7580, { &DcsOutputTable[97] }, 1 },
  { 0x7581, { &DcsOutputTable[98] }, 1 },
  { 0x74A6, { &DcsOutputTable[99] }, 1 },
  { 0x74A7, { &DcsOutputTable[100] }, 1 },
  { 0x74AA, { &DcsOutputTable[102] }, 1 },
  { 0x74AB, { &DcsOutputTable[103] }, 1 },
  { 0x74AC, { &DcsOutputTable[104] }, 1 },
  { 0x74BE, { &DcsOutputTable[105] }, 1 },
  { 0x7474, { &DcsOutputTable[106] }, 1 },
  { 0x7475, { &DcsOutputTable[107] }, 1 },
  { 0x7476, { &DcsOutputTable[108] }, 1 },
  { 0x7477, { &DcsOutputTable[109] }, 1 },
  { 0x7478, { &DcsOutputTable[110] }, 1 },
  { 0x7479, { &DcsOutputTable[111] }, 1 },
  { 0x74B6, { &DcsOutputTable[112] }, 1 },
  { 0x74B7, { &DcsOutputTable[113] }, 1 },
  { 0x74B8, { &DcsOutputTable[114] }, 1 },
  { 0x74B9, { &DcsOutputTable[115] }, 1 },
  { 0x74BA, { &DcsOutputTable[116] }, 1 },
  { 0x74BB, { &DcsOutputTable[117] }, 1 },
  { 0x74DC, { &DcsOutputTable[120] }, 1 },
  { 0x754A, { &DcsOutputTable[121] }, 1 },
  { 0x7544, { &DcsOutputTable[123] }, 1 },
  { 0x7548, { &DcsOutputTable[124] }, 1 },
  { 0x7546, { &DcsOutputTable[125] }, 1 },
  { 0x754C, { &DcsOutputTable[127] }, 1 },
  { 0x740A, { &DcsOutputTable[128], &DcsOutputTable[129], &DcsOutputTable[130], &DcsOutputTable[133], &DcsOutputTable[134], &DcsOutputTable[138], &DcsOutputTable[159], &DcsOutputTable[160], &DcsOutputTable[161], &DcsOutputTable[162], &DcsOutputTable[163], &DcsOutputTable[164], &DcsOutputTable[165], &DcsOutputTable[166], &DcsOutputTable[167] }, 15 },
  { 0x7408, { &DcsOutputTable[131], &DcsOutputTable[132], &DcsOutputTable[135], &DcsOutputTable[136], &DcsOutputTable[137], &DcsOutputTable[139], &DcsOutputTable[140], &DcsOutputTable[141], &DcsOutputTable[142], &DcsOutputTable[143], &DcsOutputTable[144], &DcsOutputTable[157], &DcsOutputTable[158] }, 13 },
  { 0x7456, { &DcsOutputTable[145] }, 1 },
  { 0x742A, { &DcsOutputTable[146], &DcsOutputTable[147], &DcsOutputTable[148], &DcsOutputTable[149] }, 4 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[223] = {
  { 0x74B9, &dcsAddressTable[94] },
  { 0x74BA, &dcsAddressTable[95] },
  { 0x74BC, &dcsAddressTable[66] },
  { 0x74BB, &dcsAddressTable[96] },
  { 0x74BE, &dcsAddressTable[84] },
  {0xFFFF, nullptr},
  { 0x74C0, &dcsAddressTable[33] },
  {0xFFFF, nullptr},
  { 0x74C2, &dcsAddressTable[51] },
  {0xFFFF, nullptr},
  { 0x74C4, &dcsAddressTable[58] },
  {0xFFFF, nullptr},
  { 0x74C6, &dcsAddressTable[14] },
  {0xFFFF, nullptr},
  { 0x74C8, &dcsAddressTable[49] },
  {0xFFFF, nullptr},
  { 0x74CA, &dcsAddressTable[68] },
  {0xFFFF, nullptr},
  { 0x74CC, &dcsAddressTable[46] },
  {0xFFFF, nullptr},
  { 0x74CE, &dcsAddressTable[59] },
  {0xFFFF, nullptr},
  { 0x74D0, &dcsAddressTable[47] },
  {0xFFFF, nullptr},
  { 0x74D2, &dcsAddressTable[61] },
  {0xFFFF, nullptr},
  { 0x74D4, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x74D6, &dcsAddressTable[60] },
  {0xFFFF, nullptr},
  { 0x74D8, &dcsAddressTable[48] },
  {0xFFFF, nullptr},
  { 0x74DA, &dcsAddressTable[67] },
  {0xFFFF, nullptr},
  { 0x74DC, &dcsAddressTable[97] },
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
  { 0x7408, &dcsAddressTable[104] },
  {0xFFFF, nullptr},
  { 0x740A, &dcsAddressTable[103] },
  {0xFFFF, nullptr},
  { 0x740C, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  { 0x740E, &dcsAddressTable[8] },
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
  { 0x742A, &dcsAddressTable[106] },
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
  { 0x7514, &dcsAddressTable[4] },
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
  { 0x7456, &dcsAddressTable[105] },
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
  { 0x7544, &dcsAddressTable[99] },
  { 0x7466, &dcsAddressTable[28] },
  { 0x7546, &dcsAddressTable[101] },
  { 0x7468, &dcsAddressTable[9] },
  { 0x7469, &dcsAddressTable[10] },
  { 0x746A, &dcsAddressTable[11] },
  { 0x746B, &dcsAddressTable[12] },
  { 0x746C, &dcsAddressTable[13] },
  { 0x754A, &dcsAddressTable[98] },
  { 0x746E, &dcsAddressTable[15] },
  { 0x746F, &dcsAddressTable[16] },
  { 0x7470, &dcsAddressTable[17] },
  { 0x7471, &dcsAddressTable[18] },
  { 0x7472, &dcsAddressTable[19] },
  { 0x7473, &dcsAddressTable[20] },
  { 0x7474, &dcsAddressTable[85] },
  { 0x7475, &dcsAddressTable[86] },
  { 0x7476, &dcsAddressTable[87] },
  { 0x7477, &dcsAddressTable[88] },
  { 0x7478, &dcsAddressTable[89] },
  { 0x7479, &dcsAddressTable[90] },
  { 0x747A, &dcsAddressTable[24] },
  { 0x7548, &dcsAddressTable[100] },
  { 0x747C, &dcsAddressTable[25] },
  { 0x754C, &dcsAddressTable[102] },
  { 0x747E, &dcsAddressTable[26] },
  {0xFFFF, nullptr},
  { 0x7480, &dcsAddressTable[27] },
  {0xFFFF, nullptr},
  { 0x7482, &dcsAddressTable[29] },
  { 0x7483, &dcsAddressTable[30] },
  { 0x7484, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x7486, &dcsAddressTable[31] },
  { 0x7487, &dcsAddressTable[32] },
  { 0x7488, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x748A, &dcsAddressTable[34] },
  { 0x748B, &dcsAddressTable[35] },
  { 0x748C, &dcsAddressTable[36] },
  { 0x748D, &dcsAddressTable[37] },
  { 0x748E, &dcsAddressTable[38] },
  { 0x748F, &dcsAddressTable[39] },
  { 0x7490, &dcsAddressTable[40] },
  { 0x7491, &dcsAddressTable[41] },
  { 0x7492, &dcsAddressTable[42] },
  { 0x7493, &dcsAddressTable[43] },
  { 0x7494, &dcsAddressTable[44] },
  { 0x7495, &dcsAddressTable[45] },
  { 0x7496, &dcsAddressTable[52] },
  { 0x7497, &dcsAddressTable[53] },
  { 0x7498, &dcsAddressTable[54] },
  {0xFFFF, nullptr},
  { 0x749A, &dcsAddressTable[55] },
  { 0x749B, &dcsAddressTable[56] },
  { 0x749C, &dcsAddressTable[57] },
  { 0x757C, &dcsAddressTable[73] },
  { 0x749E, &dcsAddressTable[62] },
  { 0x749F, &dcsAddressTable[63] },
  { 0x74A0, &dcsAddressTable[2] },
  { 0x757D, &dcsAddressTable[74] },
  { 0x74A2, &dcsAddressTable[64] },
  { 0x7582, &dcsAddressTable[50] },
  { 0x74A4, &dcsAddressTable[1] },
  { 0x74A3, &dcsAddressTable[65] },
  { 0x7584, &dcsAddressTable[69] },
  { 0x757E, &dcsAddressTable[75] },
  { 0x74A8, &dcsAddressTable[3] },
  { 0x757F, &dcsAddressTable[76] },
  { 0x7580, &dcsAddressTable[77] },
  { 0x7581, &dcsAddressTable[78] },
  { 0x74A6, &dcsAddressTable[79] },
  { 0x74A7, &dcsAddressTable[80] },
  { 0x74AE, &dcsAddressTable[21] },
  { 0x74AF, &dcsAddressTable[22] },
  { 0x74B0, &dcsAddressTable[23] },
  { 0x74AA, &dcsAddressTable[81] },
  { 0x74B2, &dcsAddressTable[70] },
  { 0x74B3, &dcsAddressTable[71] },
  { 0x74B4, &dcsAddressTable[72] },
  { 0x74AB, &dcsAddressTable[82] },
  { 0x74AC, &dcsAddressTable[83] },
  { 0x74B6, &dcsAddressTable[91] },
  { 0x74B7, &dcsAddressTable[92] },
  { 0x74B8, &dcsAddressTable[93] },
};

// Simple address hash (modulo)
constexpr uint16_t addrHash(uint16_t addr) {
  return addr % 223;
}

inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {
  uint16_t startH = addrHash(addr);
  for (uint16_t i = 0; i < 223; ++i) {
    uint16_t idx = (startH + i >= 223) ? (startH + i - 223) : (startH + i);
    const auto& entry = dcsAddressHashTable[idx];
    if (entry.addr == 0xFFFF) continue;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "APU_FIRE_BTN","APU_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "AUX_REL_SW_ENABLE","AUX_REL_SW",1,"selector",1,"ENABLE" },
    { "AUX_REL_SW_NORM","AUX_REL_SW",0,"selector",1,"NORM" },
    { "CMSD_DISPENSE_SW_BYPASS","CMSD_DISPENSE_SW",2,"selector",2,"BYPASS" },
    { "CMSD_DISPENSE_SW_ON","CMSD_DISPENSE_SW",1,"selector",2,"ON" },
    { "CMSD_DISPENSE_SW_OFF","CMSD_DISPENSE_SW",0,"selector",2,"OFF" },
    { "CMSD_JET_SEL_BTN","CMSD_JET_SEL_BTN",1,"momentary",0,"PRESS" },
    { "ECM_MODE_SW_XMIT","ECM_MODE_SW",4,"selector",3,"XMIT" },
    { "ECM_MODE_SW_REC","ECM_MODE_SW",3,"selector",3,"REC" },
    { "ECM_MODE_SW_BIT","ECM_MODE_SW",2,"selector",3,"BIT" },
    { "ECM_MODE_SW_STBY","ECM_MODE_SW",1,"selector",3,"STBY" },
    { "ECM_MODE_SW_OFF","ECM_MODE_SW",0,"selector",3,"OFF" },
    { "FIRE_EXT_BTN","FIRE_EXT_BTN",1,"momentary",0,"PRESS" },
    { "IFEI_DWN_BTN","IFEI_DWN_BTN",1,"momentary",0,"PRESS" },
    { "IFEI_ET_BTN","IFEI_ET_BTN",1,"momentary",0,"PRESS" },
    { "IFEI_MODE_BTN","IFEI_MODE_BTN",1,"momentary",0,"PRESS" },
    { "IFEI_QTY_BTN","IFEI_QTY_BTN",1,"momentary",0,"PRESS" },
    { "IFEI_UP_BTN","IFEI_UP_BTN",1,"momentary",0,"PRESS" },
    { "IFEI_ZONE_BTN","IFEI_ZONE_BTN",1,"momentary",0,"PRESS" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",4,"NVG" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",4,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",4,"DAY" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",5,"TEST" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",5,"OFF" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "LEFT_FIRE_BTN","LEFT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "LEFT_FIRE_BTN_COVER","LEFT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
    { "HMD_OFF_BRT","HMD_OFF_BRT",65535,"analog",0,"LEVEL" },
    { "IR_COOL_SW_ORIDE","IR_COOL_SW",2,"selector",6,"ORIDE" },
    { "IR_COOL_SW_NORM","IR_COOL_SW",1,"selector",6,"NORM" },
    { "IR_COOL_SW_OFF","IR_COOL_SW",0,"selector",6,"OFF" },
    { "SPIN_RECOVERY_COVER","SPIN_RECOVERY_COVER",1,"momentary",0,"OPEN" },
    { "SPIN_RECOVERY_SW_RCVY","SPIN_RECOVERY_SW",1,"selector",7,"RCVY" },
    { "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_SW",0,"selector",7,"NORM" },
    { "MASTER_ARM_SW_ARM","MASTER_ARM_SW",1,"selector",8,"ARM" },
    { "MASTER_ARM_SW_SAFE","MASTER_ARM_SW",0,"selector",8,"SAFE" },
    { "MASTER_MODE_AA","MASTER_MODE_AA",1,"momentary",0,"PRESS" },
    { "MASTER_MODE_AG","MASTER_MODE_AG",1,"momentary",0,"PRESS" },
    { "MASTER_CAUTION_RESET_SW","MASTER_CAUTION_RESET_SW",1,"momentary",0,"PRESS" },
    { "RIGHT_FIRE_BTN","RIGHT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "RIGHT_FIRE_BTN_COVER","RIGHT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "APU_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "AUX_REL_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_DISPENSE_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_JET_SEL_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ECM_MODE_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "FIRE_EXT_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "HMD_OFF_BRT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFEI_DWN_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFEI_ET_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFEI_MODE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFEI_QTY_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFEI_UP_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IFEI_ZONE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IR_COOL_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_ARM_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_CAUTION_RESET_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_MODE_AA", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_MODE_AG", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
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
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_BINGO", 0x7468, 5 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_BINGO_TEXTURE", 0x74C6, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_CLOCK_H", 0x746E, 2 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_CLOCK_M", 0x7470, 2 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_CLOCK_S", 0x7472, 2 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_CODES", 0x74AE, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_DD_1", 0x747A, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_DD_2", 0x747C, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_DD_3", 0x747E, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_DD_4", 0x7480, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_FF_L", 0x7482, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_FF_R", 0x7486, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_FF_TEXTURE", 0x74C0, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_FUEL_DOWN", 0x748A, 6 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_FUEL_UP", 0x7490, 6 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_L0_TEXTURE", 0x74CC, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_L100_TEXTURE", 0x74D4, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_L50_TEXTURE", 0x74D0, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_LPOINTER_TEXTURE", 0x74D8, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_LSCALE_TEXTURE", 0x74C8, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_L_TEXTURE", 0x7582, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_NOZ_TEXTURE", 0x74C2, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_OIL_PRESS_L", 0x7496, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_OIL_PRESS_R", 0x749A, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_OIL_TEXTURE", 0x74C4, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_R0_TEXTURE", 0x74CE, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_R100_TEXTURE", 0x74D6, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_R50_TEXTURE", 0x74D2, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_RPM_L", 0x749E, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_RPM_R", 0x74A2, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_RPM_TEXTURE", 0x74BC, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_RPOINTER_TEXTURE", 0x74DA, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_RSCALE_TEXTURE", 0x74CA, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_R_TEXTURE", 0x7584, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_SP", 0x74B2, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_T", 0x757C, 6 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TEMP_L", 0x74A6, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TEMP_R", 0x74AA, 3 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TEMP_TEXTURE", 0x74BE, 1 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TIMER_H", 0x7474, 2 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TIMER_M", 0x7476, 2 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TIMER_S", 0x7478, 2 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_TIME_SET_MODE", 0x74B6, 6 },
    { "Integrated Fuel/Engine Indicator (IFEI)", "IFEI_Z_TEXTURE", 0x74DC, 1 },
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[89] = {
  {nullptr, nullptr},
  {"IFEI_CLOCK_M", &displayFields[3]},
  {"IFEI_FUEL_DOWN", &displayFields[13]},
  {nullptr, nullptr},
  {"IFEI_LSCALE_TEXTURE", &displayFields[19]},
  {"IFEI_NOZ_TEXTURE", &displayFields[21]},
  {"IFEI_RPM_L", &displayFields[28]},
  {"IFEI_CLOCK_S", &displayFields[4]},
  {"IFEI_FF_TEXTURE", &displayFields[12]},
  {"IFEI_BINGO_TEXTURE", &displayFields[1]},
  {nullptr, nullptr},
  {"IFEI_FF_L", &displayFields[10]},
  {"IFEI_RPM_R", &displayFields[29]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_TEMP_TEXTURE", &displayFields[38]},
  {nullptr, nullptr},
  {"IFEI_FF_R", &displayFields[11]},
  {"IFEI_L100_TEXTURE", &displayFields[16]},
  {nullptr, nullptr},
  {"IFEI_L_TEXTURE", &displayFields[20]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_CODES", &displayFields[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_R_TEXTURE", &displayFields[33]},
  {"IFEI_R0_TEXTURE", &displayFields[25]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_R50_TEXTURE", &displayFields[27]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_FUEL_UP", &displayFields[14]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_RPOINTER_TEXTURE", &displayFields[31]},
  {nullptr, nullptr},
  {"IFEI_LPOINTER_TEXTURE", &displayFields[18]},
  {"IFEI_T", &displayFields[35]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_TIME_SET_MODE", &displayFields[42]},
  {"IFEI_BINGO", &displayFields[0]},
  {nullptr, nullptr},
  {"IFEI_RSCALE_TEXTURE", &displayFields[32]},
  {"IFEI_TIMER_H", &displayFields[39]},
  {nullptr, nullptr},
  {"IFEI_L50_TEXTURE", &displayFields[17]},
  {"IFEI_OIL_PRESS_L", &displayFields[22]},
  {"IFEI_L0_TEXTURE", &displayFields[15]},
  {"IFEI_OIL_TEXTURE", &displayFields[24]},
  {"IFEI_R100_TEXTURE", &displayFields[26]},
  {"IFEI_RPM_TEXTURE", &displayFields[30]},
  {"IFEI_OIL_PRESS_R", &displayFields[23]},
  {"IFEI_DD_1", &displayFields[6]},
  {"IFEI_DD_2", &displayFields[7]},
  {"IFEI_DD_3", &displayFields[8]},
  {"IFEI_DD_4", &displayFields[9]},
  {"IFEI_SP", &displayFields[34]},
  {"IFEI_TEMP_L", &displayFields[36]},
  {"IFEI_TEMP_R", &displayFields[37]},
  {"IFEI_TIMER_M", &displayFields[40]},
  {"IFEI_TIMER_S", &displayFields[41]},
  {"IFEI_Z_TEXTURE", &displayFields[43]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"IFEI_CLOCK_H", &displayFields[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 89;
  for (uint16_t i = 0; i < 89; ++i) {
    uint16_t idx = (startH + i >= 89) ? (startH + i - 89) : (startH + i);
    const auto& entry = displayFieldsByLabel[idx];
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.def;
  }
  return nullptr;
}

