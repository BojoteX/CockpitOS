// Auto-generated — DO NOT EDIT
#pragma once

#include "../../../lib/CUtils/src/CUtils.h"

// Buffers and dirty flags for all display fields (global)
extern char ufc_comm1_display[3];
extern bool ufc_comm1_display_dirty;
extern char last_ufc_comm1_display[3];
extern char ufc_comm2_display[3];
extern bool ufc_comm2_display_dirty;
extern char last_ufc_comm2_display[3];
extern char ufc_option_cueing_1[2];
extern bool ufc_option_cueing_1_dirty;
extern char last_ufc_option_cueing_1[2];
extern char ufc_option_cueing_2[2];
extern bool ufc_option_cueing_2_dirty;
extern char last_ufc_option_cueing_2[2];
extern char ufc_option_cueing_3[2];
extern bool ufc_option_cueing_3_dirty;
extern char last_ufc_option_cueing_3[2];
extern char ufc_option_cueing_4[2];
extern bool ufc_option_cueing_4_dirty;
extern char last_ufc_option_cueing_4[2];
extern char ufc_option_cueing_5[2];
extern bool ufc_option_cueing_5_dirty;
extern char last_ufc_option_cueing_5[2];
extern char ufc_option_display_1[5];
extern bool ufc_option_display_1_dirty;
extern char last_ufc_option_display_1[5];
extern char ufc_option_display_2[5];
extern bool ufc_option_display_2_dirty;
extern char last_ufc_option_display_2[5];
extern char ufc_option_display_3[5];
extern bool ufc_option_display_3_dirty;
extern char last_ufc_option_display_3[5];
extern char ufc_option_display_4[5];
extern bool ufc_option_display_4_dirty;
extern char last_ufc_option_display_4[5];
extern char ufc_option_display_5[5];
extern bool ufc_option_display_5_dirty;
extern char last_ufc_option_display_5[5];
extern char ufc_scratchpad_number_display[9];
extern bool ufc_scratchpad_number_display_dirty;
extern char last_ufc_scratchpad_number_display[9];
extern char ufc_scratchpad_string_1_display[3];
extern bool ufc_scratchpad_string_1_display_dirty;
extern char last_ufc_scratchpad_string_1_display[3];
extern char ufc_scratchpad_string_2_display[3];
extern bool ufc_scratchpad_string_2_display_dirty;
extern char last_ufc_scratchpad_string_2_display[3];

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

