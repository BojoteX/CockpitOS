// Auto-generated — DO NOT EDIT
#pragma once

#include "../../../lib/CUtils/src/CUtils.h"

// Buffers and dirty flags for all display fields (global)
extern char plt_cmws_bit_line_1[4];
extern bool plt_cmws_bit_line_1_dirty;
extern char last_plt_cmws_bit_line_1[4];
extern char plt_cmws_bit_line_2[5];
extern bool plt_cmws_bit_line_2_dirty;
extern char last_plt_cmws_bit_line_2[5];
extern char plt_cmws_chaff_count[4];
extern bool plt_cmws_chaff_count_dirty;
extern char last_plt_cmws_chaff_count[4];
extern char plt_cmws_chaff_letter[2];
extern bool plt_cmws_chaff_letter_dirty;
extern char last_plt_cmws_chaff_letter[2];
extern char plt_cmws_flare_count[4];
extern bool plt_cmws_flare_count_dirty;
extern char last_plt_cmws_flare_count[4];
extern char plt_cmws_flare_letter[2];
extern bool plt_cmws_flare_letter_dirty;
extern char last_plt_cmws_flare_letter[2];
extern char plt_cmws_page[5];
extern bool plt_cmws_page_dirty;
extern char last_plt_cmws_page[5];

extern DisplayBufferEntry CT_DisplayBuffers[];
extern const size_t numCTDisplayBuffers;
extern const DisplayBufferHashEntry CT_DisplayBufferHash[];
const DisplayBufferEntry* findDisplayBufferByLabel(const char* label);


    // --- Forward declarations for opaque driver pointer types ---
    typedef void* DisplayDriverPtr;

    // --- Forward declaration for field def struct (real type in DisplayMapping.h) ---
    struct DisplayFieldDefLabel;
    struct FieldState;

    void renderField(const char* label, const char* value, const DisplayFieldDefLabel* defOverride = nullptr, FieldState* stateOverride = nullptr);

    