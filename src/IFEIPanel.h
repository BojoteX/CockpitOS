#pragma once

#include "LABELS/LABEL_SET_IFEI_NO_VIDEO/DisplayMapping.h"

// Device as Input (buttons)
void IFEI_init();
void IFEI_loop();
void IFEIDisplay_loop();

// Device as Output (Display)
void IFEIDisplay_init();
void IFEIDisplay_commit();  // (flushes shadow RAM)

// Helpers
bool isFieldBlank(const char* s);

// --- 74HC165 IFEI pins ---
#define HC165_QH  38
#define HC165_PL  39
#define HC165_CP  40

// Left LCD
#define DATA0_PIN 34
#define WR0_PIN   35
#define CS0_PIN   36

// Right LCD
#define DATA1_PIN 18
#define WR1_PIN   21
#define CS1_PIN   33

// Backlight PINs
#define BL_GREEN_PIN 1
#define BL_WHITE_PIN 2
#define BL_NVG_PIN   4

// Max Display Regions
#define MAX_DISPLAY_FIELDS 64

// Max LEDs
#define IFEI_MAX_CHIPS 2

struct CommitRegion {
    const char* label;
    uint8_t chip;
    uint8_t addrStart;
    uint8_t addrEnd;
};

// You can statically cap this to numFieldDefs
static CommitRegion commitRegions[MAX_DISPLAY_FIELDS];
static int numCommitRegions = 0;

extern IFEIDisplay ifei;

class IFEIDisplay {
public:
    IFEIDisplay(HT1622* chips[IFEI_MAX_CHIPS]);

    // Basic methods
    void commit(bool force = false);
    void clear();
    void buildCommitRegions();
    void commitNextRegion();
    inline uint8_t (*getRamShadow())[64] { return ramShadow; }
    void readRegionFromShadow(const SegmentMap* map, int numDigits, int segsPerDigit, uint8_t ramShadow[IFEI_MAX_CHIPS][64], char* out, size_t outSize);

    // Refresh & Resets
    void blankBuffersAndDirty(); 	// Blanks all integers buffers and mark as dirty forcing a natural update
    void invalidateHardwareCache(); 	// Clears the shadow cache on the device

    // ---- WRITE HELPERS ---- (The functions that write the segments)
    void addFuelStringToShadow(const char* str, const SegmentMap map[6][14]);
    void addBingoStringToShadow(const char* str, const SegmentMap map[5][7]);
    void addRpmStringToShadow(const char* str, const SegmentMap map[3][7]);
    void addPointerBarToShadow(int percent, const SegmentMap* barMap, int numBars);
    void addAlphaNumFuelStringToShadow(const char* str, const SegmentMap map[6][14]);
    void addLabelToShadow(const SegmentMap& label, const char* value);
    void addAsciiString7SegToShadow(const char* str, const SegmentMap* map, int numDigits);

    // ---- CLEAR HELPERS ---- (Functions that clear the segments)
    void clear7SegFromShadow(const SegmentMap* map, int numDigits);
    void clearBingoFromShadow(const SegmentMap map[5][7]);
    void clearFuelFromShadow(const SegmentMap map[6][14]); 
    void clearLabelFromShadow(const SegmentMap* segMap);
    void clearBarFromShadow(const SegmentMap* barMap, int numBars);

private:
    HT1622* _chips[IFEI_MAX_CHIPS];
    uint8_t ramShadow[IFEI_MAX_CHIPS][64]; // Per-chip shadow RAM
    uint8_t lastShadow[IFEI_MAX_CHIPS][64];
};