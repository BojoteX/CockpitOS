// Auto-generated — DO NOT EDIT
#pragma once

#include "../../../lib/CUtils/src/CUtils.h"

// Buffers and dirty flags for all display fields (global)

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

