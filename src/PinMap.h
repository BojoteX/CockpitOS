#pragma once

// Map canonical S2 GPIO numbers to the current board's GPIO.
#if defined(ARDUINO_LOLIN_S3_MINI)
constexpr int map_s2_to_board(int n) {
    switch (n) {
    case 3:  return 2;   case 5:  return 4;   case 7:  return 12;
    case 9:  return 13;  case 12: return 10;  case 2:  return 3;
    case 4:  return 5;   case 8:  return 7;   case 10: return 8;
    case 13: return 9;   case 40: return 33;  case 38: return 37;
    case 36: return 38;  case 39: return 43;  case 37: return 44;
    case 35: return 36;  case 33: return 35;
    default: return n;   // identity for unmapped pins
    }
}
#define PIN(n) (map_s2_to_board(n))
#else
constexpr int map_s2_to_board(int n) { return n; }
#define PIN(n) (n)
#endif