// DisplayMapping.cpp - Defines the display fields
#include "../../Globals.h"
#include "DisplayMapping.h"

const DisplayFieldDefLabel fieldDefs[] = {
};
size_t numFieldDefs = sizeof(fieldDefs) / sizeof(fieldDefs[0]);
FieldState fieldStates[sizeof(fieldDefs) / sizeof(fieldDefs[0])];

// --- Hash Table for fieldDefs[] ---
struct FieldDefHashEntry { const char* label; const DisplayFieldDefLabel* def; };
static const size_t FIELD_HASH_SIZE = 2;
static const FieldDefHashEntry fieldDefHashTable[FIELD_HASH_SIZE] = {
    {nullptr, nullptr},
    {nullptr, nullptr},
};

// --- labelHash-based lookup (uses shared labelHash from CUtils.h) ---
const DisplayFieldDefLabel* findFieldDefByLabel(const char* label) {
    uint16_t startH = labelHash(label) % FIELD_HASH_SIZE;
    for (uint16_t i = 0; i < FIELD_HASH_SIZE; ++i) {
        uint16_t idx = (startH + i >= FIELD_HASH_SIZE) ? (startH + i - FIELD_HASH_SIZE) : (startH + i);
        const auto& entry = fieldDefHashTable[idx];
        if (!entry.label) return nullptr;
        if (strcmp(entry.label, label) == 0) return entry.def;
    }
    return nullptr;
}


    const DisplayFieldDefLabel* findFieldByLabel(const char* label) {
        for (size_t i = 0; i < numFieldDefs; ++i)
            if (strcmp(fieldDefs[i].label, label) == 0)
                return &fieldDefs[i];
        return nullptr;
    }

    