#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("=================================");
  Serial.println("GE-80 | PlatformIO ESP32 OK");
  Serial.println("=================================");
  Serial.print("Chip model: ");
  Serial.println(ESP.getChipModel());
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 1000) {
    last = millis();
    Serial.print("Uptime (s): ");
    Serial.println(millis() / 1000);
  }
}
