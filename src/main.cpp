#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "mqtt/tb_mqtt.h"

static const int BUTTON_PIN = 25;
int lastState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("=================================");
  Serial.println("GE-83 | WiFi + MQTT ThingsBoard");
  Serial.println("=================================");
  Serial.print("Button pin: GPIO ");
  Serial.println(BUTTON_PIN);
  Serial.println("Expected: RELEASED=HIGH, PRESSED=LOW");
  Serial.println();

  wifi::connect(20000, 500);
  tb_mqtt::begin();
}

void loop() {
  tb_mqtt::loop();

  int currentState = digitalRead(BUTTON_PIN);
  if (currentState != lastState) {
    lastState = currentState;
    if (currentState == LOW) Serial.println("Button PRESSED (LOW)");
    else Serial.println("Button RELEASED (HIGH)");
  }

  delay(20);
}
