// HC165.cpp - 74HC165 shift register read logic

static bool hc165_initialized = false;
static uint8_t plPin, cpPin, qhPin;

void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH) {
  plPin = pinPL;
  cpPin = pinCP;
  qhPin = pinQH;

  pinMode(plPin, OUTPUT);
  pinMode(cpPin, OUTPUT);
  pinMode(qhPin, INPUT);

  digitalWrite(plPin, HIGH);
  digitalWrite(cpPin, LOW);
  hc165_initialized = true;
}

uint8_t HC165_read() {
  if (!hc165_initialized) return 0xFF;

  uint8_t result = 0;

  digitalWrite(plPin, LOW);
  delayMicroseconds(5);
  digitalWrite(plPin, HIGH);
  delayMicroseconds(1);

  for (int i = 0; i < 8; i++) {
    result <<= 1;
    result |= digitalRead(qhPin);

    digitalWrite(cpPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(cpPin, LOW);
    delayMicroseconds(1);
  }

  return result;
}