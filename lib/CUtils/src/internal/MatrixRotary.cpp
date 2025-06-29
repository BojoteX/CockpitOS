// MatrixRotary.cpp - Shared rotary matrix decoder logic

uint8_t MatrixRotary_readPattern(const int* strobes, int count, int dataPin) {
  uint8_t pattern = 0;

  for (int i = 0; i < count; i++) {
    digitalWrite(strobes[i], LOW);
    delayMicroseconds(1);
    if (digitalRead(dataPin) == LOW) {
      pattern |= (1 << i);
    }
    digitalWrite(strobes[i], HIGH);
  }

  return pattern;
}