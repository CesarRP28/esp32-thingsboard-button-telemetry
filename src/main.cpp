#include <Arduino.h>
#include "wifi/wifi_manager.h"

static const int BUTTON_PIN = 25;  // GPIO seguro en ESP32
int lastState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println();
  Serial.println("=================================");
  Serial.println("GE-82 | WiFi + Button GPIO Input");
  Serial.println("=================================");
  Serial.print("Button pin: GPIO ");
  Serial.println(BUTTON_PIN);
  Serial.println("Expected: RELEASED=HIGH, PRESSED=LOW");
  Serial.println();

  // GE-82: WiFi
  wifi::connect(20000, 500);
}

void loop() {
  // GE-81: Botón
  int currentState = digitalRead(BUTTON_PIN);

  if (currentState != lastState) {
    lastState = currentState;

    if (currentState == LOW) {
      Serial.println("Button PRESSED (LOW)");
    } else {
      Serial.println("Button RELEASED (HIGH)");
    }
  }

  // GE-82: Log cada 5s
  static uint32_t lastWifiLog = 0;
  if (millis() - lastWifiLog > 5000) {
    lastWifiLog = millis();
    Serial.print("[WiFi] Connected: ");
    Serial.print(wifi::isConnected() ? "YES" : "NO");
    Serial.print(" | IP: ");
    Serial.println(wifi::ip());
  }

  delay(20);
}
