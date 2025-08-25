// Auto-generated — DO NOT EDIT
#pragma once

// Buffers and dirty flags for all display fields (global)
extern char dcs_bios[7];
extern bool dcs_bios_dirty;
extern char last_dcs_bios[7];
extern char pilotname[25];
extern bool pilotname_dirty;
extern char last_pilotname[25];
extern char _acft_name[25];
extern bool _acft_name_dirty;
extern char last__acft_name[25];

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

