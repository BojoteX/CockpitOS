#include "../../Globals.h"
#include "CT_Display.h"
#include "DisplayMapping.h"

char ifei_bingo[6] = {};
bool ifei_bingo_dirty = false;
char last_ifei_bingo[6] = {};
char ifei_bingo_texture[2] = {};
bool ifei_bingo_texture_dirty = false;
char last_ifei_bingo_texture[2] = {};
char ifei_clock_h[3] = {};
bool ifei_clock_h_dirty = false;
char last_ifei_clock_h[3] = {};
char ifei_clock_m[3] = {};
bool ifei_clock_m_dirty = false;
char last_ifei_clock_m[3] = {};
char ifei_clock_s[3] = {};
bool ifei_clock_s_dirty = false;
char last_ifei_clock_s[3] = {};
char ifei_codes[4] = {};
bool ifei_codes_dirty = false;
char last_ifei_codes[4] = {};
char ifei_dd_1[2] = {};
bool ifei_dd_1_dirty = false;
char last_ifei_dd_1[2] = {};
char ifei_dd_2[2] = {};
bool ifei_dd_2_dirty = false;
char last_ifei_dd_2[2] = {};
char ifei_dd_3[2] = {};
bool ifei_dd_3_dirty = false;
char last_ifei_dd_3[2] = {};
char ifei_dd_4[2] = {};
bool ifei_dd_4_dirty = false;
char last_ifei_dd_4[2] = {};
char ifei_ff_l[4] = {};
bool ifei_ff_l_dirty = false;
char last_ifei_ff_l[4] = {};
char ifei_ff_r[4] = {};
bool ifei_ff_r_dirty = false;
char last_ifei_ff_r[4] = {};
char ifei_ff_texture[2] = {};
bool ifei_ff_texture_dirty = false;
char last_ifei_ff_texture[2] = {};
char ifei_fuel_down[7] = {};
bool ifei_fuel_down_dirty = false;
char last_ifei_fuel_down[7] = {};
char ifei_fuel_up[7] = {};
bool ifei_fuel_up_dirty = false;
char last_ifei_fuel_up[7] = {};
char ifei_l0_texture[2] = {};
bool ifei_l0_texture_dirty = false;
char last_ifei_l0_texture[2] = {};
char ifei_l100_texture[2] = {};
bool ifei_l100_texture_dirty = false;
char last_ifei_l100_texture[2] = {};
char ifei_l50_texture[2] = {};
bool ifei_l50_texture_dirty = false;
char last_ifei_l50_texture[2] = {};
char ifei_lpointer_texture[2] = {};
bool ifei_lpointer_texture_dirty = false;
char last_ifei_lpointer_texture[2] = {};
char ifei_lscale_texture[2] = {};
bool ifei_lscale_texture_dirty = false;
char last_ifei_lscale_texture[2] = {};
char ifei_l_texture[2] = {};
bool ifei_l_texture_dirty = false;
char last_ifei_l_texture[2] = {};
char ifei_noz_texture[2] = {};
bool ifei_noz_texture_dirty = false;
char last_ifei_noz_texture[2] = {};
char ifei_oil_press_l[4] = {};
bool ifei_oil_press_l_dirty = false;
char last_ifei_oil_press_l[4] = {};
char ifei_oil_press_r[4] = {};
bool ifei_oil_press_r_dirty = false;
char last_ifei_oil_press_r[4] = {};
char ifei_oil_texture[2] = {};
bool ifei_oil_texture_dirty = false;
char last_ifei_oil_texture[2] = {};
char ifei_r0_texture[2] = {};
bool ifei_r0_texture_dirty = false;
char last_ifei_r0_texture[2] = {};
char ifei_r100_texture[2] = {};
bool ifei_r100_texture_dirty = false;
char last_ifei_r100_texture[2] = {};
char ifei_r50_texture[2] = {};
bool ifei_r50_texture_dirty = false;
char last_ifei_r50_texture[2] = {};
char ifei_rpm_l[4] = {};
bool ifei_rpm_l_dirty = false;
char last_ifei_rpm_l[4] = {};
char ifei_rpm_r[4] = {};
bool ifei_rpm_r_dirty = false;
char last_ifei_rpm_r[4] = {};
char ifei_rpm_texture[2] = {};
bool ifei_rpm_texture_dirty = false;
char last_ifei_rpm_texture[2] = {};
char ifei_rpointer_texture[2] = {};
bool ifei_rpointer_texture_dirty = false;
char last_ifei_rpointer_texture[2] = {};
char ifei_rscale_texture[2] = {};
bool ifei_rscale_texture_dirty = false;
char last_ifei_rscale_texture[2] = {};
char ifei_r_texture[2] = {};
bool ifei_r_texture_dirty = false;
char last_ifei_r_texture[2] = {};
char ifei_sp[4] = {};
bool ifei_sp_dirty = false;
char last_ifei_sp[4] = {};
char ifei_t[7] = {};
bool ifei_t_dirty = false;
char last_ifei_t[7] = {};
char ifei_temp_l[4] = {};
bool ifei_temp_l_dirty = false;
char last_ifei_temp_l[4] = {};
char ifei_temp_r[4] = {};
bool ifei_temp_r_dirty = false;
char last_ifei_temp_r[4] = {};
char ifei_temp_texture[2] = {};
bool ifei_temp_texture_dirty = false;
char last_ifei_temp_texture[2] = {};
char ifei_timer_h[3] = {};
bool ifei_timer_h_dirty = false;
char last_ifei_timer_h[3] = {};
char ifei_timer_m[3] = {};
bool ifei_timer_m_dirty = false;
char last_ifei_timer_m[3] = {};
char ifei_timer_s[3] = {};
bool ifei_timer_s_dirty = false;
char last_ifei_timer_s[3] = {};
char ifei_time_set_mode[7] = {};
bool ifei_time_set_mode_dirty = false;
char last_ifei_time_set_mode[7] = {};
char ifei_z_texture[2] = {};
bool ifei_z_texture_dirty = false;
char last_ifei_z_texture[2] = {};

DisplayBufferEntry CT_DisplayBuffers[] = {
    { "IFEI_BINGO", ifei_bingo, 5, &ifei_bingo_dirty, last_ifei_bingo },
    { "IFEI_BINGO_TEXTURE", ifei_bingo_texture, 1, &ifei_bingo_texture_dirty, last_ifei_bingo_texture },
    { "IFEI_CLOCK_H", ifei_clock_h, 2, &ifei_clock_h_dirty, last_ifei_clock_h },
    { "IFEI_CLOCK_M", ifei_clock_m, 2, &ifei_clock_m_dirty, last_ifei_clock_m },
    { "IFEI_CLOCK_S", ifei_clock_s, 2, &ifei_clock_s_dirty, last_ifei_clock_s },
    { "IFEI_CODES", ifei_codes, 3, &ifei_codes_dirty, last_ifei_codes },
    { "IFEI_DD_1", ifei_dd_1, 1, &ifei_dd_1_dirty, last_ifei_dd_1 },
    { "IFEI_DD_2", ifei_dd_2, 1, &ifei_dd_2_dirty, last_ifei_dd_2 },
    { "IFEI_DD_3", ifei_dd_3, 1, &ifei_dd_3_dirty, last_ifei_dd_3 },
    { "IFEI_DD_4", ifei_dd_4, 1, &ifei_dd_4_dirty, last_ifei_dd_4 },
    { "IFEI_FF_L", ifei_ff_l, 3, &ifei_ff_l_dirty, last_ifei_ff_l },
    { "IFEI_FF_R", ifei_ff_r, 3, &ifei_ff_r_dirty, last_ifei_ff_r },
    { "IFEI_FF_TEXTURE", ifei_ff_texture, 1, &ifei_ff_texture_dirty, last_ifei_ff_texture },
    { "IFEI_FUEL_DOWN", ifei_fuel_down, 6, &ifei_fuel_down_dirty, last_ifei_fuel_down },
    { "IFEI_FUEL_UP", ifei_fuel_up, 6, &ifei_fuel_up_dirty, last_ifei_fuel_up },
    { "IFEI_L0_TEXTURE", ifei_l0_texture, 1, &ifei_l0_texture_dirty, last_ifei_l0_texture },
    { "IFEI_L100_TEXTURE", ifei_l100_texture, 1, &ifei_l100_texture_dirty, last_ifei_l100_texture },
    { "IFEI_L50_TEXTURE", ifei_l50_texture, 1, &ifei_l50_texture_dirty, last_ifei_l50_texture },
    { "IFEI_LPOINTER_TEXTURE", ifei_lpointer_texture, 1, &ifei_lpointer_texture_dirty, last_ifei_lpointer_texture },
    { "IFEI_LSCALE_TEXTURE", ifei_lscale_texture, 1, &ifei_lscale_texture_dirty, last_ifei_lscale_texture },
    { "IFEI_L_TEXTURE", ifei_l_texture, 1, &ifei_l_texture_dirty, last_ifei_l_texture },
    { "IFEI_NOZ_TEXTURE", ifei_noz_texture, 1, &ifei_noz_texture_dirty, last_ifei_noz_texture },
    { "IFEI_OIL_PRESS_L", ifei_oil_press_l, 3, &ifei_oil_press_l_dirty, last_ifei_oil_press_l },
    { "IFEI_OIL_PRESS_R", ifei_oil_press_r, 3, &ifei_oil_press_r_dirty, last_ifei_oil_press_r },
    { "IFEI_OIL_TEXTURE", ifei_oil_texture, 1, &ifei_oil_texture_dirty, last_ifei_oil_texture },
    { "IFEI_R0_TEXTURE", ifei_r0_texture, 1, &ifei_r0_texture_dirty, last_ifei_r0_texture },
    { "IFEI_R100_TEXTURE", ifei_r100_texture, 1, &ifei_r100_texture_dirty, last_ifei_r100_texture },
    { "IFEI_R50_TEXTURE", ifei_r50_texture, 1, &ifei_r50_texture_dirty, last_ifei_r50_texture },
    { "IFEI_RPM_L", ifei_rpm_l, 3, &ifei_rpm_l_dirty, last_ifei_rpm_l },
    { "IFEI_RPM_R", ifei_rpm_r, 3, &ifei_rpm_r_dirty, last_ifei_rpm_r },
    { "IFEI_RPM_TEXTURE", ifei_rpm_texture, 1, &ifei_rpm_texture_dirty, last_ifei_rpm_texture },
    { "IFEI_RPOINTER_TEXTURE", ifei_rpointer_texture, 1, &ifei_rpointer_texture_dirty, last_ifei_rpointer_texture },
    { "IFEI_RSCALE_TEXTURE", ifei_rscale_texture, 1, &ifei_rscale_texture_dirty, last_ifei_rscale_texture },
    { "IFEI_R_TEXTURE", ifei_r_texture, 1, &ifei_r_texture_dirty, last_ifei_r_texture },
    { "IFEI_SP", ifei_sp, 3, &ifei_sp_dirty, last_ifei_sp },
    { "IFEI_T", ifei_t, 6, &ifei_t_dirty, last_ifei_t },
    { "IFEI_TEMP_L", ifei_temp_l, 3, &ifei_temp_l_dirty, last_ifei_temp_l },
    { "IFEI_TEMP_R", ifei_temp_r, 3, &ifei_temp_r_dirty, last_ifei_temp_r },
    { "IFEI_TEMP_TEXTURE", ifei_temp_texture, 1, &ifei_temp_texture_dirty, last_ifei_temp_texture },
    { "IFEI_TIMER_H", ifei_timer_h, 2, &ifei_timer_h_dirty, last_ifei_timer_h },
    { "IFEI_TIMER_M", ifei_timer_m, 2, &ifei_timer_m_dirty, last_ifei_timer_m },
    { "IFEI_TIMER_S", ifei_timer_s, 2, &ifei_timer_s_dirty, last_ifei_timer_s },
    { "IFEI_TIME_SET_MODE", ifei_time_set_mode, 6, &ifei_time_set_mode_dirty, last_ifei_time_set_mode },
    { "IFEI_Z_TEXTURE", ifei_z_texture, 1, &ifei_z_texture_dirty, last_ifei_z_texture },
};
const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);

const DisplayBufferHashEntry CT_DisplayBufferHash[89] = {
    {nullptr, nullptr},
    {"IFEI_CLOCK_M", &CT_DisplayBuffers[3]},
    {"IFEI_FUEL_DOWN", &CT_DisplayBuffers[13]},
    {nullptr, nullptr},
    {"IFEI_LSCALE_TEXTURE", &CT_DisplayBuffers[19]},
    {"IFEI_NOZ_TEXTURE", &CT_DisplayBuffers[21]},
    {"IFEI_RPM_L", &CT_DisplayBuffers[28]},
    {"IFEI_CLOCK_S", &CT_DisplayBuffers[4]},
    {"IFEI_FF_TEXTURE", &CT_DisplayBuffers[12]},
    {"IFEI_BINGO_TEXTURE", &CT_DisplayBuffers[1]},
    {nullptr, nullptr},
    {"IFEI_FF_L", &CT_DisplayBuffers[10]},
    {"IFEI_RPM_R", &CT_DisplayBuffers[29]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_TEMP_TEXTURE", &CT_DisplayBuffers[38]},
    {nullptr, nullptr},
    {"IFEI_FF_R", &CT_DisplayBuffers[11]},
    {"IFEI_L100_TEXTURE", &CT_DisplayBuffers[16]},
    {nullptr, nullptr},
    {"IFEI_L_TEXTURE", &CT_DisplayBuffers[20]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_CODES", &CT_DisplayBuffers[5]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_R_TEXTURE", &CT_DisplayBuffers[33]},
    {"IFEI_R0_TEXTURE", &CT_DisplayBuffers[25]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_R50_TEXTURE", &CT_DisplayBuffers[27]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_FUEL_UP", &CT_DisplayBuffers[14]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_RPOINTER_TEXTURE", &CT_DisplayBuffers[31]},
    {nullptr, nullptr},
    {"IFEI_LPOINTER_TEXTURE", &CT_DisplayBuffers[18]},
    {"IFEI_T", &CT_DisplayBuffers[35]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_TIME_SET_MODE", &CT_DisplayBuffers[42]},
    {"IFEI_BINGO", &CT_DisplayBuffers[0]},
    {nullptr, nullptr},
    {"IFEI_RSCALE_TEXTURE", &CT_DisplayBuffers[32]},
    {"IFEI_TIMER_H", &CT_DisplayBuffers[39]},
    {nullptr, nullptr},
    {"IFEI_L50_TEXTURE", &CT_DisplayBuffers[17]},
    {"IFEI_OIL_PRESS_L", &CT_DisplayBuffers[22]},
    {"IFEI_L0_TEXTURE", &CT_DisplayBuffers[15]},
    {"IFEI_OIL_TEXTURE", &CT_DisplayBuffers[24]},
    {"IFEI_R100_TEXTURE", &CT_DisplayBuffers[26]},
    {"IFEI_RPM_TEXTURE", &CT_DisplayBuffers[30]},
    {"IFEI_OIL_PRESS_R", &CT_DisplayBuffers[23]},
    {"IFEI_DD_1", &CT_DisplayBuffers[6]},
    {"IFEI_DD_2", &CT_DisplayBuffers[7]},
    {"IFEI_DD_3", &CT_DisplayBuffers[8]},
    {"IFEI_DD_4", &CT_DisplayBuffers[9]},
    {"IFEI_SP", &CT_DisplayBuffers[34]},
    {"IFEI_TEMP_L", &CT_DisplayBuffers[36]},
    {"IFEI_TEMP_R", &CT_DisplayBuffers[37]},
    {"IFEI_TIMER_M", &CT_DisplayBuffers[40]},
    {"IFEI_TIMER_S", &CT_DisplayBuffers[41]},
    {"IFEI_Z_TEXTURE", &CT_DisplayBuffers[43]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"IFEI_CLOCK_H", &CT_DisplayBuffers[2]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
};

const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {
    uint16_t startH = labelHash(label) % 89;
    for (uint16_t i = 0; i < 89; ++i) {
        uint16_t idx = (startH + i >= 89) ? (startH + i - 89) : (startH + i);
        const auto& entry = CT_DisplayBufferHash[idx];
        if (!entry.label) continue;
        if (strcmp(entry.label, label) == 0) return entry.entry;
    }
    return nullptr;
}


    void renderField(const char* label, const char* strValue,
                     const DisplayFieldDefLabel* defOverride,
                     FieldState* stateOverride) {
        const DisplayFieldDefLabel* def = defOverride ? defOverride : findFieldDefByLabel(label);
        if (!def) return;
        if (!def->renderFunc) { debugPrintf("[DISPLAY] No renderFunc for label '%s', skipping\n", def->label); return; }

        FieldState* state = stateOverride;
        if (!state) {
            int idx = def - fieldDefs;
            state = &fieldStates[idx];
        }

        bool valid = true;
        if (def->type == FIELD_NUMERIC) {
            const int value = strToIntFast(strValue);
            valid = (value >= def->minValue && value <= def->maxValue);
        }
        const char* toShow = valid ? strValue : state->lastValue;

        // bound compare/copy by field length, clamp to cache size
        uint8_t need = def->numDigits ? def->numDigits : 1;
        if (need > sizeof(state->lastValue)) need = sizeof(state->lastValue);

        if (memcmp(toShow, state->lastValue, need) == 0) return;

        memcpy(state->lastValue, toShow, need);
        if (def->clearFunc) def->clearFunc(def->driver, def->segMap, *def);
        def->renderFunc(def->driver, def->segMap, toShow, *def);
    }

    