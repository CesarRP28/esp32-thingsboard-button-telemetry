#include "wifi_manager.h"
#include <WiFi.h>

// Credenciales fuera del repo
#include "secrets.h"

namespace wifi {

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String ip() {
  if (!isConnected()) return String("-");
  return WiFi.localIP().toString();
}

void connect(uint32_t timeoutMs, uint32_t retryDelayMs) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  Serial.println("[WiFi] Connecting...");
  Serial.printf("[WiFi] SSID: %s\n", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (!isConnected() && (millis() - start) < timeoutMs) {
    Serial.print(".");
    delay(retryDelayMs);
  }
  Serial.println();

  if (isConnected()) {
    Serial.println("[WiFi] Connected ✅");
    Serial.print("[WiFi] IP: ");
    Serial.println(ip());
    Serial.print("[WiFi] RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("[WiFi] Connection FAILED ❌");
    Serial.println("[WiFi] Tips: check SSID/PASSWORD, use 2.4GHz, verify signal.");
  }
}

} // namespace wifi
