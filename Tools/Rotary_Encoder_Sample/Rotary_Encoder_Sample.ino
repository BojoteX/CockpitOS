/* MatrixRotaryTest.ino  —  ESP32-S2 generic MATRIX proof
   - Uses only source/port/bit
   - Patterns from bit (decimal). Pins from port.
   - One multi-bit row supplies data pin; one-bit rows supply strobe pins.
   - No registration. Lazy build on first poll. Bounded, deterministic.
*/

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

// ---------- Pins (ALR-67 example) ----------
#define ALR67_STROBE_1  16
#define ALR67_STROBE_2  17
#define ALR67_STROBE_3  21
#define ALR67_STROBE_4  37
#define ALR67_DataPin   38

// ---------- Minimal HID stub ----------
static inline void HIDManager_setNamedButton(const char* label, bool force, bool pressed) {
  if (pressed) {
    Serial.print("[EMIT] "); Serial.print(force ? "(force) " : ""); Serial.println(label);
  }
}

// ---------- InputMappings shape (subset) ----------
struct InputMapping {
  const char* label;
  const char* source;    // "MATRIX"
  int16_t     port;      // GPIO: data on multi-bit row; strobe GPIO on one-bit rows
  int16_t     bit;       // pattern (decimal). -1 = fallback
  int16_t     hidId;     // unused
  const char* oride_label; // unused
  int16_t     oride_value; // unused
  const char* controlType; // "selector" (unused here)
  int16_t     group;       // unused
};

// ---------- Your five rows (plus optional fallback) ----------
static const InputMapping InputMappings[] = {
  { "RWR_DIS_TYPE_SW_F", "MATRIX", ALR67_DataPin, 15, -1, "RWR_DIS_TYPE_SW", 4, "selector", 6 }, // multi-bit → data
  { "RWR_DIS_TYPE_SW_U", "MATRIX", ALR67_STROBE_1,  1, -1, "RWR_DIS_TYPE_SW", 3, "selector", 6 }, // 1  → strobe0
  { "RWR_DIS_TYPE_SW_A", "MATRIX", ALR67_STROBE_2,  2, -1, "RWR_DIS_TYPE_SW", 2, "selector", 6 }, // 2  → strobe1
  { "RWR_DIS_TYPE_SW_I", "MATRIX", ALR67_STROBE_3,  4, -1, "RWR_DIS_TYPE_SW", 1, "selector", 6 }, // 4  → strobe2
  { "RWR_DIS_TYPE_SW_N", "MATRIX", ALR67_STROBE_4,  8, -1, "RWR_DIS_TYPE_SW", 0, "selector", 6 }, // 8  → strobe3
  // Optional fallback when no contact:
  // { "RWR_DIS_TYPE_SW_N", "MATRIX", ALR67_DataPin, -1, -1, "RWR_DIS_TYPE_SW", 4, "selector", 6 },
};
static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);

// ---------- MATRIX generic (lazy-build) ----------
#define MAX_MATRIX_ROTARIES  4
#define MAX_MATRIX_STROBES   8
#define MAX_MATRIX_POS       16

struct MatrixPos { uint8_t pattern; const char* label; };
struct MatrixRotary {
  const char* family;  uint8_t familyLen;
  uint8_t dataPin;  uint8_t strobeCount;  uint8_t strobes[MAX_MATRIX_STROBES];
  MatrixPos pos[MAX_MATRIX_POS]; uint8_t posCount; int8_t fallbackIdx;
  uint8_t lastPattern; bool configured;
};

static MatrixRotary g_matrix[MAX_MATRIX_ROTARIES];
static uint8_t      g_matCount = 0;
static bool         g_built    = false;

static inline uint8_t lastUnderscore(const char* s){ uint8_t p=0,i=0; while(s[i]){ if(s[i]=='_') p=i; if(++i==255) break; } return p; }
static inline bool sameKey(const char* a,uint8_t al,const char* b,uint8_t bl){ if(al!=bl) return false; for(uint8_t i=0;i<al;++i) if(a[i]!=b[i]) return false; return true; }
static inline uint8_t popcount8(uint8_t x){ x = x - ((x>>1)&0x55u); x = (x&0x33u) + ((x>>2)&0x33u); return (uint8_t)((x + (x>>4)) & 0x0Fu); }
static inline int8_t oneBitIndex(uint8_t x){ if(x==0u || (x & (uint8_t)(x-1u))) return -1; uint8_t i=0; while(((x>>i)&1u)==0u) ++i; return (int8_t)i; }

static inline uint8_t scanPattern(const uint8_t* strobes, uint8_t n, uint8_t dataPin){
  uint8_t pat=0;
  for(uint8_t i=0;i<n;++i){
    const uint8_t s=strobes[i]; if(s==0xFF) continue;
    digitalWrite(s, LOW);
    delayMicroseconds(1);
    if (digitalRead(dataPin) == LOW) pat |= (uint8_t)(1u << i);
    digitalWrite(s, HIGH);
  }
  return pat;
}

static void Matrix_buildOnce(){
  if (g_built) return;
  g_built = true;

  // Discover families
  for (size_t i=0; i<InputMappingSize && g_matCount<MAX_MATRIX_ROTARIES; ++i){
    const auto& m = InputMappings[i];
    if (!m.source || strcmp(m.source,"MATRIX")!=0 || !m.label) continue;
    const uint8_t cut = lastUnderscore(m.label); if (cut==0) continue;
    bool found=false;
    for(uint8_t r=0;r<g_matCount;++r)
      if(sameKey(g_matrix[r].family,g_matrix[r].familyLen,m.label,cut)){ found=true; break; }
    if(found) continue;

    MatrixRotary& R = g_matrix[g_matCount++];
    R.family=m.label; R.familyLen=cut;
    R.dataPin=0xFF; R.strobeCount=0; for(uint8_t k=0;k<MAX_MATRIX_STROBES;++k) R.strobes[k]=0xFF;
    R.posCount=0; R.fallbackIdx=-1; R.lastPattern=0xFE; R.configured=false;
  }

  // Build each family: positions + infer pins
  for (uint8_t r=0; r<g_matCount; ++r){
    MatrixRotary& R = g_matrix[r];

    for (size_t i=0; i<InputMappingSize; ++i){
      const auto& m = InputMappings[i];
      if (!m.source || strcmp(m.source,"MATRIX")!=0 || !m.label) continue;
      const uint8_t cut = lastUnderscore(m.label);
      if (!sameKey(R.family,R.familyLen,m.label,cut)) continue;

      // Collect label+pattern from bit
      if (R.posCount < MAX_MATRIX_POS){
        if ((int8_t)m.bit < 0 && R.fallbackIdx < 0) R.fallbackIdx = (int8_t)R.posCount;
        R.pos[R.posCount++] = MatrixPos{ (uint8_t)m.bit, m.label };
      }

      // infer data pin from multi-bit OR fallback row
      if (m.port >= 0) {
        const uint8_t pc = popcount8((uint8_t)m.bit);
        if ((pc > 1 && R.dataPin == 0xFF) || ((int16_t)m.bit < 0 && R.dataPin == 0xFF))
          R.dataPin = (uint8_t)m.port;
      }

      // Infer strobe pin from one-bit rows
      if (m.port >= 0) {
        const int8_t idx = oneBitIndex((uint8_t)m.bit);
        if (idx >= 0 && idx < (int8_t)MAX_MATRIX_STROBES) R.strobes[(uint8_t)idx] = (uint8_t)m.port;
      }
    }

    // Finalize strobe count
    uint8_t maxIdx=0; for(uint8_t i=0;i<MAX_MATRIX_STROBES;++i) if(R.strobes[i]!=0xFF && i>maxIdx) maxIdx=i;
    R.strobeCount = (uint8_t)(maxIdx+1);

    // If data pin still unknown, grab any port from family
    if (R.dataPin==0xFF){
      for (size_t i=0; i<InputMappingSize; ++i){
        const auto& m = InputMappings[i];
        if (!m.source || strcmp(m.source,"MATRIX")!=0 || !m.label) continue;
        const uint8_t cut = lastUnderscore(m.label);
        if (!sameKey(R.family,R.familyLen,m.label,cut)) continue;
        if (m.port >= 0){ R.dataPin=(uint8_t)m.port; break; }
      }
    }

    // GPIO configure once
    if (R.dataPin!=0xFF){
      pinMode(R.dataPin, INPUT_PULLUP);
      for(uint8_t i=0;i<R.strobeCount;++i){ const uint8_t s=R.strobes[i]; if(s==0xFF) continue; pinMode(s, OUTPUT); digitalWrite(s,HIGH); }
      R.configured=true;
    }
  }
}

static void Matrix_poll(bool forceSend){
  Matrix_buildOnce();
  for (uint8_t r=0;r<g_matCount;++r){
    MatrixRotary& R = g_matrix[r];
    if (!R.configured || !R.posCount || !R.strobeCount) continue;

    const uint8_t pat = scanPattern(R.strobes, R.strobeCount, R.dataPin);
    if (!forceSend && pat==R.lastPattern) continue;

    int match=-1;
    for (uint8_t i=0;i<R.posCount;++i){ if(R.pos[i].pattern==pat){ match=(int)i; break; } }
    if (match<0) match = R.fallbackIdx;
    if (match>=0) HIDManager_setNamedButton(R.pos[match].label, forceSend, true);
    R.lastPattern = pat;
  }
}

// ---------- Sketch ----------
static unsigned long lastPoll=0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("MATRIX generic test starting…");

  // Force a first emit after GPIO setup
  Matrix_poll(true);
}

void loop() {
  const unsigned long now = millis();
  if (now - lastPoll < 10) return; // ~100 Hz bounded polling
  lastPoll = now;

  Matrix_poll(false);
}
