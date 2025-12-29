// InputControl.h - Centralized management for GPIO/PCA/HC165 inputs

#pragma once

#include "HIDManager.h" // for HIDAxis enum, or forward declare if needed

namespace AnalogAcq {
  void sample(uint8_t pin);
  void consume(uint8_t pin, uint16_t &avg12, uint16_t &min12, uint16_t &max12, uint16_t &ema12);
}

// ===== GPIO (Inputs/Selectors/Analogs) =====
extern uint8_t numGPIOEncoders;
extern uint8_t encoderPinMask[48];
void buildGPIOEncoderStates();
void buildGpioGroupDefs();
void pollGPIOEncoders();
void pollGPIOSelectors(bool forceSend = false);
void pollGPIOMomentaries(bool forceSend = false);

#define MAX_AUTO_ANALOGS   HID_AXIS_COUNT
struct AutoAnalogInput {
    const char* label;
    uint8_t gpio;
    HIDAxis axis;
};
extern AutoAnalogInput autoAnalogs[MAX_AUTO_ANALOGS];
extern size_t numAutoAnalogs;
void buildAutoAnalogInputs();

struct PCA9555Input {
    uint8_t addr, port, bit;  // bit==255 for fallback (-1)
    bool isMomentary, isSelector;
    int16_t group, oride_value;
    const char* label;
};

struct PcaState {
    uint8_t addr, p0, p1;
};

extern PCA9555Input pca9555Inputs[MAX_PCA9555_INPUTS];
extern size_t numPca9555Inputs;
extern PcaState pcas[MAX_PCAS];
extern size_t numPcas;
extern bool lastStatePCA9555[MAX_PCA9555_INPUTS];
extern int16_t lastValSelector[MAX_PCA_GROUPS][MAX_PCAS];

void buildPcaList();
void buildPCA9555ResolvedInputs();
void pollPCA9555_flat(bool forceSend = false);


// ===== HC165 Logic =====
#define HC165_INVERT_MASK 0ULL

// HC165 (shift register) — table build, cache reset, and dispatcher
void buildHC165ResolvedInputs();                    // build flat tables from InputMappings[]
void resetHC165SelectorCache();                     // clear per-group latch
void processHC165Resolved(uint64_t cur, uint64_t prev, bool forceSend);


// ===== MATRIX rotary (strobe/data) — fully generic; discovers from InputMappings[] =====
#define MAX_MATRIX_ROTARIES  8
#define MAX_MATRIX_STROBES   8
#define MAX_MATRIX_POS       16

// Lazy-builds on first call from InputMappings[] (source="MATRIX"; port=GPIO; bit=pattern decimal).
void Matrix_poll(bool forceSend);


// ===== TM1637 momentary keys — fully generic; discovers from InputMappings[] =====
#define MAX_TM1637_DEV    4
#define MAX_TM1637_KEYS   64

void TM1637_poll(bool forceSend);