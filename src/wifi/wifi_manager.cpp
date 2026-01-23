#include "wifi_manager.h"
#include <WiFi.h>
#include "secrets.h"

/*
  Estado anterior del WiFi para detectar cambios (edge detection).
  Esto nos permite:
  - Imprimir "Connected ✅ (recovered)" SOLO una vez cuando el WiFi vuelva.
  - Evitar logs repetitivos cuando el estado no cambia.
*/
static wl_status_t lastStatus = WL_IDLE_STATUS;

/*
  Parámetros de reconexión básica:
  - RECONNECT_INTERVAL_MS: cada cuánto reintentamos si WiFi está caído.
  - lastAttemptMs: marca el último intento para no spamear.
*/
static uint32_t lastAttemptMs = 0;
static const uint32_t RECONNECT_INTERVAL_MS = 5000; // 5 segundos

namespace wifi {

bool isConnected() {
  // WL_CONNECTED significa que ya obtuvo IP y está asociado a la red
  return WiFi.status() == WL_CONNECTED;
}

void connect(uint32_t timeoutMs, uint32_t retryDelayMs) {
  // Mensajes iniciales (no mostramos SSID real)
  Serial.println("[WiFi] Connecting...");
  Serial.println("[WiFi] SSID: (hidden)");

  // Modo estación (se conecta a un AP/router)
  WiFi.mode(WIFI_STA);

  // Inicia la conexión con credenciales almacenadas en secrets.h
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Espera hasta conectar o agotar el timeout (bloqueante solo en setup)
  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
    Serial.print(".");
    delay(retryDelayMs);
  }
  Serial.println();

  // Sincroniza lastStatus con el estado actual al finalizar connect()
  lastStatus = WiFi.status();

  if (isConnected()) {
    // Conectado: mostramos datos útiles para evidencia
    Serial.println("[WiFi] Connected ✅");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    // No conectó en setup: igual seguimos, porque loop() reintentará
    Serial.println("[WiFi] Connect FAILED ❌ (will retry in loop)");
  }
}

void loop() {
  // Lee el estado actual del WiFi
  wl_status_t st = WiFi.status();

  // Si el estado cambió desde la última vez, lo registramos
  if (st != lastStatus) {
    lastStatus = st;

    // Si volvió a conectarse, lo anunciamos una sola vez
    if (st == WL_CONNECTED) {
      Serial.println("[WiFi] Connected ✅ (recovered)");
      Serial.print("[WiFi] IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("[WiFi] RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    } else {
      // Estado cambió a algo distinto de conectado
      // (útil para diagnóstico sin spamear)
      Serial.print("[WiFi] Status changed: ");
      Serial.println((int)st);
    }
  }

  // Si ya está conectado, no hacemos nada más
  if (st == WL_CONNECTED) return;

  // Reintentos espaciados para no spamear reconexión
  const uint32_t nowMs = millis();
  if (nowMs - lastAttemptMs < RECONNECT_INTERVAL_MS) return;
  lastAttemptMs = nowMs;

  // Intento de reconexión NO bloqueante:
  // - WiFi.begin() inicia el intento, pero no esperamos aquí.
  Serial.println("[WiFi] Disconnected. Reconnecting...");
  WiFi.disconnect(); // limpia estado previo
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

} // namespace wifi
