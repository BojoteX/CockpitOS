#include <Arduino.h>

static bool hc165_initialized = false;
static uint8_t plPin, cpPin, qhPin;
static uint8_t hc165_numBits = 8;  // default to 8 bits if not specified
static uint64_t lastReadBits = 0xFFFFFFFFFFFFFFFFULL;

void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH, uint8_t numBits) {
  plPin = pinPL;
  cpPin = pinCP;
  qhPin = pinQH;
  hc165_numBits = numBits;
  if (hc165_numBits == 0 || hc165_numBits > 64) hc165_numBits = 8;  // safe fallback

  pinMode(plPin, OUTPUT);
  pinMode(cpPin, OUTPUT);
  pinMode(qhPin, INPUT);

  digitalWrite(plPin, HIGH);
  digitalWrite(cpPin, LOW);
  hc165_initialized = true;
}

uint64_t HC165_read() {
    if (!hc165_initialized) return 0xFFFFFFFFFFFFFFFFULL;

    uint64_t prev = lastReadBits;  // Store previous value for diff
    uint64_t result = 0;

    digitalWrite(plPin, LOW);
    delayMicroseconds(5);
    digitalWrite(plPin, HIGH);
    delayMicroseconds(1);

    for (uint8_t i = 0; i < hc165_numBits; i++) {
        result <<= 1;
        result |= digitalRead(qhPin);

        digitalWrite(cpPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(cpPin, LOW);
        delayMicroseconds(1);
    }

#if DEBUG_ENABLED_FOR_HC165_ONLY
    if (result != prev) {
        HC165_printBitChanges(prev, result, hc165_numBits);
        delay(3);  // Optional: debounce output readability
    }
#endif

    lastReadBits = result;
    return result;
}

// --- Debug Helpers ---

void HC165_printBits(const char* prefix, uint64_t value, uint8_t numBits) {
#if DEBUG_ENABLED_FOR_HC165_ONLY
  char buf[128];
  size_t pos = 0;
  pos += snprintf(buf + pos, sizeof(buf) - pos, "%s: ", prefix);
  for (int i = numBits - 1; i >= 0 && pos < sizeof(buf) - 2; --i) {
    buf[pos++] = ((value >> i) & 1) ? '1' : '0';
  }
  buf[pos] = '\0';
  debugPrintln(buf);
#endif
}

void HC165_printBitChanges(uint64_t prev, uint64_t curr, uint8_t numBits) {
#if DEBUG_ENABLED_FOR_HC165_ONLY
  char buf[512];
  size_t pos = 0;
  uint64_t changed = prev ^ curr;
  for (uint8_t i = 0; i < numBits && pos < sizeof(buf) - 32; ++i) {
    if (changed & (1ULL << i)) {
      pos += snprintf(buf + pos, sizeof(buf) - pos,
        "  BIT %u: %d -> %d\n",
        i,
        (int)((prev >> i) & 1),
        (int)((curr >> i) & 1)
      );
    }
  }
  if (pos > 0) {
    buf[pos] = '\0';
    debugPrintln(buf);
  }
#endif
}
