#include "../../Globals.h"
#include "CT_Display.h"
#include "DisplayMapping.h"

char ufc_comm1_display[3] = {};
bool ufc_comm1_display_dirty = false;
char last_ufc_comm1_display[3] = {};
char ufc_comm2_display[3] = {};
bool ufc_comm2_display_dirty = false;
char last_ufc_comm2_display[3] = {};
char ufc_option_cueing_1[2] = {};
bool ufc_option_cueing_1_dirty = false;
char last_ufc_option_cueing_1[2] = {};
char ufc_option_cueing_2[2] = {};
bool ufc_option_cueing_2_dirty = false;
char last_ufc_option_cueing_2[2] = {};
char ufc_option_cueing_3[2] = {};
bool ufc_option_cueing_3_dirty = false;
char last_ufc_option_cueing_3[2] = {};
char ufc_option_cueing_4[2] = {};
bool ufc_option_cueing_4_dirty = false;
char last_ufc_option_cueing_4[2] = {};
char ufc_option_cueing_5[2] = {};
bool ufc_option_cueing_5_dirty = false;
char last_ufc_option_cueing_5[2] = {};
char ufc_option_display_1[5] = {};
bool ufc_option_display_1_dirty = false;
char last_ufc_option_display_1[5] = {};
char ufc_option_display_2[5] = {};
bool ufc_option_display_2_dirty = false;
char last_ufc_option_display_2[5] = {};
char ufc_option_display_3[5] = {};
bool ufc_option_display_3_dirty = false;
char last_ufc_option_display_3[5] = {};
char ufc_option_display_4[5] = {};
bool ufc_option_display_4_dirty = false;
char last_ufc_option_display_4[5] = {};
char ufc_option_display_5[5] = {};
bool ufc_option_display_5_dirty = false;
char last_ufc_option_display_5[5] = {};
char ufc_scratchpad_number_display[9] = {};
bool ufc_scratchpad_number_display_dirty = false;
char last_ufc_scratchpad_number_display[9] = {};
char ufc_scratchpad_string_1_display[3] = {};
bool ufc_scratchpad_string_1_display_dirty = false;
char last_ufc_scratchpad_string_1_display[3] = {};
char ufc_scratchpad_string_2_display[3] = {};
bool ufc_scratchpad_string_2_display_dirty = false;
char last_ufc_scratchpad_string_2_display[3] = {};

DisplayBufferEntry CT_DisplayBuffers[] = {
    { "UFC_COMM1_DISPLAY", ufc_comm1_display, 2, &ufc_comm1_display_dirty, last_ufc_comm1_display },
    { "UFC_COMM2_DISPLAY", ufc_comm2_display, 2, &ufc_comm2_display_dirty, last_ufc_comm2_display },
    { "UFC_OPTION_CUEING_1", ufc_option_cueing_1, 1, &ufc_option_cueing_1_dirty, last_ufc_option_cueing_1 },
    { "UFC_OPTION_CUEING_2", ufc_option_cueing_2, 1, &ufc_option_cueing_2_dirty, last_ufc_option_cueing_2 },
    { "UFC_OPTION_CUEING_3", ufc_option_cueing_3, 1, &ufc_option_cueing_3_dirty, last_ufc_option_cueing_3 },
    { "UFC_OPTION_CUEING_4", ufc_option_cueing_4, 1, &ufc_option_cueing_4_dirty, last_ufc_option_cueing_4 },
    { "UFC_OPTION_CUEING_5", ufc_option_cueing_5, 1, &ufc_option_cueing_5_dirty, last_ufc_option_cueing_5 },
    { "UFC_OPTION_DISPLAY_1", ufc_option_display_1, 4, &ufc_option_display_1_dirty, last_ufc_option_display_1 },
    { "UFC_OPTION_DISPLAY_2", ufc_option_display_2, 4, &ufc_option_display_2_dirty, last_ufc_option_display_2 },
    { "UFC_OPTION_DISPLAY_3", ufc_option_display_3, 4, &ufc_option_display_3_dirty, last_ufc_option_display_3 },
    { "UFC_OPTION_DISPLAY_4", ufc_option_display_4, 4, &ufc_option_display_4_dirty, last_ufc_option_display_4 },
    { "UFC_OPTION_DISPLAY_5", ufc_option_display_5, 4, &ufc_option_display_5_dirty, last_ufc_option_display_5 },
    { "UFC_SCRATCHPAD_NUMBER_DISPLAY", ufc_scratchpad_number_display, 8, &ufc_scratchpad_number_display_dirty, last_ufc_scratchpad_number_display },
    { "UFC_SCRATCHPAD_STRING_1_DISPLAY", ufc_scratchpad_string_1_display, 2, &ufc_scratchpad_string_1_display_dirty, last_ufc_scratchpad_string_1_display },
    { "UFC_SCRATCHPAD_STRING_2_DISPLAY", ufc_scratchpad_string_2_display, 2, &ufc_scratchpad_string_2_display_dirty, last_ufc_scratchpad_string_2_display },
};
const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);

const DisplayBufferHashEntry CT_DisplayBufferHash[31] = {
    {"UFC_OPTION_CUEING_2", &CT_DisplayBuffers[3]},
    {"UFC_OPTION_CUEING_3", &CT_DisplayBuffers[4]},
    {"UFC_OPTION_CUEING_4", &CT_DisplayBuffers[5]},
    {"UFC_OPTION_CUEING_5", &CT_DisplayBuffers[6]},
    {"UFC_COMM1_DISPLAY", &CT_DisplayBuffers[0]},
    {"UFC_OPTION_DISPLAY_1", &CT_DisplayBuffers[7]},
    {"UFC_OPTION_DISPLAY_2", &CT_DisplayBuffers[8]},
    {"UFC_OPTION_DISPLAY_3", &CT_DisplayBuffers[9]},
    {"UFC_OPTION_DISPLAY_4", &CT_DisplayBuffers[10]},
    {"UFC_OPTION_DISPLAY_5", &CT_DisplayBuffers[11]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"UFC_COMM2_DISPLAY", &CT_DisplayBuffers[1]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"UFC_SCRATCHPAD_NUMBER_DISPLAY", &CT_DisplayBuffers[12]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"UFC_SCRATCHPAD_STRING_1_DISPLAY", &CT_DisplayBuffers[13]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"UFC_SCRATCHPAD_STRING_2_DISPLAY", &CT_DisplayBuffers[14]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"UFC_OPTION_CUEING_1", &CT_DisplayBuffers[2]},
};

const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {
    uint16_t startH = labelHash(label) % 31;
    for (uint16_t i = 0; i < 31; ++i) {
        uint16_t idx = (startH + i >= 31) ? (startH + i - 31) : (startH + i);
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

