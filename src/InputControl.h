// InputControl.h - Centralized management for GPIO/PCA/HC165 inputs

#include "HIDManager.h" // for HIDAxis enum, or forward declare if needed

// -- GPIO (Inputs/Selectors/Analogs)
extern uint8_t numGPIOEncoders;
extern uint8_t encoderPinMask[48];
void buildGPIOEncoderStates();
void buildGpioGroupDefs();
void pollGPIOSelectors(bool forceSend = false);
void pollGPIOEncoders();
void pollGPIOMomentaries();

#define MAX_AUTO_ANALOGS   HID_AXIS_COUNT
struct AutoAnalogInput {
    const char* label;
    uint8_t gpio;
    HIDAxis axis;
};
extern AutoAnalogInput autoAnalogs[MAX_AUTO_ANALOGS];
extern size_t numAutoAnalogs;
void buildAutoAnalogInputs();

// —— PCA9555 (I²C expander) —— 
#define DISABLE_PCA9555                0   // 1 = skip PCA logic, 0 = enable
#define MAX_PCA9555_INPUTS             64  // Max PCA input mappings
#define MAX_PCA_GROUPS                 32  // Max selector groups
#define MAX_PCAS                       8   // Max PCA9555 chips (0x20–0x27)

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

// HC165 Logic
#ifndef HC165_INVERT_MASK
#define HC165_INVERT_MASK 0ULL
#endif

// HC165 (shift register) — table build, cache reset, and dispatcher
void buildHC165ResolvedInputs();                    // build flat tables from InputMappings[]
void resetHC165SelectorCache();                     // clear per-group latch
void processHC165Resolved(uint64_t cur, uint64_t prev, bool forceSend);