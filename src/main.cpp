#include <Arduino.h>

static const int BUTTON_PIN = 25;  // GPIO seguro en ESP32

int lastState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("=================================");
  Serial.println("GE-81 | Button GPIO Input");
  Serial.println("=================================");
  Serial.print("Button pin: GPIO ");
  Serial.println(BUTTON_PIN);
  Serial.println("Expected: RELEASED=HIGH, PRESSED=LOW");
}

void loop() {
  int currentState = digitalRead(BUTTON_PIN);

  if (currentState != lastState) {
    lastState = currentState;

    if (currentState == LOW) {
      Serial.println("Button PRESSED (LOW)");
    } else {
      Serial.println("Button RELEASED (HIGH)");
    }
  }

  delay(20); // anti-rebote simple
}
