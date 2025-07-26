#define SW1 37
#define SW2 38
#define SW3 39
#define SW4 40

void setup() {
  Serial.begin(115200);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW4, INPUT_PULLUP);
}

void loop() {
  int s1 = digitalRead(SW1);
  int s2 = digitalRead(SW2);
  int s3 = digitalRead(SW3);
  int s4 = digitalRead(SW4);

  if (s1 == LOW && s2 == HIGH && s3 == HIGH && s4 == HIGH) Serial.println("Position 1");
  else if (s1 == HIGH && s2 == LOW && s3 == HIGH && s4 == HIGH) Serial.println("Position 2");
  else if (s1 == HIGH && s2 == HIGH && s3 == LOW && s4 == HIGH) Serial.println("Position 3");
  else if (s1 == HIGH && s2 == HIGH && s3 == HIGH && s4 == LOW) Serial.println("Position 4");
  else Serial.println("⚠️ Unknown or multiple active!");

  delay(200);
}

