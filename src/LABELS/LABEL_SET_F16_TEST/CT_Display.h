// Auto-generated — DO NOT EDIT
#pragma once

// Buffers and dirty flags for all display fields (global)
extern char cmds_ch_amount[5];
extern bool cmds_ch_amount_dirty;
extern char last_cmds_ch_amount[5];
extern char cmds_fl_amount[5];
extern bool cmds_fl_amount_dirty;
extern char last_cmds_fl_amount[5];
extern char cmds_o1_amount[5];
extern bool cmds_o1_amount_dirty;
extern char last_cmds_o1_amount[5];
extern char cmds_o2_amount[5];
extern bool cmds_o2_amount_dirty;
extern char last_cmds_o2_amount[5];
extern char ded_line_1[30];
extern bool ded_line_1_dirty;
extern char last_ded_line_1[30];
extern char ded_line_2[30];
extern bool ded_line_2_dirty;
extern char last_ded_line_2[30];
extern char ded_line_3[30];
extern bool ded_line_3_dirty;
extern char last_ded_line_3[30];
extern char ded_line_4[30];
extern bool ded_line_4_dirty;
extern char last_ded_line_4[30];
extern char ded_line_5[30];
extern bool ded_line_5_dirty;
extern char last_ded_line_5[30];
extern char uhf_chan_disp[3];
extern bool uhf_chan_disp_dirty;
extern char last_uhf_chan_disp[3];
extern char uhf_freq_disp[8];
extern bool uhf_freq_disp_dirty;
extern char last_uhf_freq_disp[8];

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

