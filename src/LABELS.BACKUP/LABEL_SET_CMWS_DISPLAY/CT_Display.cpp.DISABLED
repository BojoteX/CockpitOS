#include "../../Globals.h"
#include "CT_Display.h"
#include "DisplayMapping.h"

char plt_cmws_bit_line_1[4] = {};
bool plt_cmws_bit_line_1_dirty = false;
char last_plt_cmws_bit_line_1[4] = {};
char plt_cmws_bit_line_2[5] = {};
bool plt_cmws_bit_line_2_dirty = false;
char last_plt_cmws_bit_line_2[5] = {};
char plt_cmws_chaff_count[4] = {};
bool plt_cmws_chaff_count_dirty = false;
char last_plt_cmws_chaff_count[4] = {};
char plt_cmws_chaff_letter[2] = {};
bool plt_cmws_chaff_letter_dirty = false;
char last_plt_cmws_chaff_letter[2] = {};
char plt_cmws_flare_count[4] = {};
bool plt_cmws_flare_count_dirty = false;
char last_plt_cmws_flare_count[4] = {};
char plt_cmws_flare_letter[2] = {};
bool plt_cmws_flare_letter_dirty = false;
char last_plt_cmws_flare_letter[2] = {};
char plt_cmws_page[5] = {};
bool plt_cmws_page_dirty = false;
char last_plt_cmws_page[5] = {};

DisplayBufferEntry CT_DisplayBuffers[] = {
    { "PLT_CMWS_BIT_LINE_1", plt_cmws_bit_line_1, 3, &plt_cmws_bit_line_1_dirty, last_plt_cmws_bit_line_1 },
    { "PLT_CMWS_BIT_LINE_2", plt_cmws_bit_line_2, 4, &plt_cmws_bit_line_2_dirty, last_plt_cmws_bit_line_2 },
    { "PLT_CMWS_CHAFF_COUNT", plt_cmws_chaff_count, 3, &plt_cmws_chaff_count_dirty, last_plt_cmws_chaff_count },
    { "PLT_CMWS_CHAFF_LETTER", plt_cmws_chaff_letter, 1, &plt_cmws_chaff_letter_dirty, last_plt_cmws_chaff_letter },
    { "PLT_CMWS_FLARE_COUNT", plt_cmws_flare_count, 3, &plt_cmws_flare_count_dirty, last_plt_cmws_flare_count },
    { "PLT_CMWS_FLARE_LETTER", plt_cmws_flare_letter, 1, &plt_cmws_flare_letter_dirty, last_plt_cmws_flare_letter },
    { "PLT_CMWS_PAGE", plt_cmws_page, 4, &plt_cmws_page_dirty, last_plt_cmws_page },
};
const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);

const DisplayBufferHashEntry CT_DisplayBufferHash[17] = {
    {"PLT_CMWS_BIT_LINE_2", &CT_DisplayBuffers[1]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"PLT_CMWS_CHAFF_COUNT", &CT_DisplayBuffers[2]},
    {"PLT_CMWS_PAGE", &CT_DisplayBuffers[6]},
    {nullptr, nullptr},
    {"PLT_CMWS_FLARE_COUNT", &CT_DisplayBuffers[4]},
    {"PLT_CMWS_FLARE_LETTER", &CT_DisplayBuffers[5]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"PLT_CMWS_CHAFF_LETTER", &CT_DisplayBuffers[3]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"PLT_CMWS_BIT_LINE_1", &CT_DisplayBuffers[0]},
};

const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {
    uint16_t startH = labelHash(label) % 17;
    for (uint16_t i = 0; i < 17; ++i) {
        uint16_t idx = (startH + i >= 17) ? (startH + i - 17) : (startH + i);
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

