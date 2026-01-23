#include "tb_mqtt.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

// Cliente TCP
static WiFiClient espClient;

// Cliente MQTT
static PubSubClient client(espClient);

// Topic estándar de telemetría en ThingsBoard
static const char* TB_TELEMETRY_TOPIC = "v1/devices/me/telemetry";

/*
  Control de reconexión MQTT (GE-85):
  - lastMqttAttemptMs guarda el último intento de conexión.
  - MQTT_RECONNECT_INTERVAL_MS define el intervalo entre intentos.
*/
static uint32_t lastMqttAttemptMs = 0;
static const uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000; // 5 segundos

namespace tb_mqtt {

void begin() {
  // Configura broker/puerto. No conecta aún.
  client.setServer(TB_HOST, TB_PORT);
}

bool isConnected() {
  return client.connected();
}

/*
  connectOnce()
  -------------
  Intenta conectarse UNA sola vez al broker MQTT de ThingsBoard.

  Auth ThingsBoard:
  - username = TB_TOKEN
  - password = vacío
*/
static void connectOnce() {
  // Log explícito indicando MQTT + host:port (mejor para evidencia)
  Serial.print("[TB][MQTT] Connecting to ");
  Serial.print(TB_HOST);
  Serial.print(":");
  Serial.print(TB_PORT);
  Serial.print(" ... ");

  // clientId debe ser "único" para evitar colisiones.
  // (En GE-86 podrías hacerlo con MAC para hacerlo realmente único.)
  const char* clientId = "ESP32_GE85";

  // Conecta: clientId, username(token), password(vacío)
  bool ok = client.connect(clientId, TB_TOKEN, nullptr);

  if (ok) {
    Serial.println("CONNECTED ✅");
  } else {
    // rc típico:
    // 4 = bad username/password (token mal)
    // 5 = not authorized
    Serial.print("FAILED ❌ rc=");
    Serial.println(client.state());
  }
}

/*
  publishTelemetry(json)
  ----------------------
  Publica telemetría en ThingsBoard si MQTT está conectado.
*/
bool publishTelemetry(const char* json) {
  if (!client.connected()) {
    Serial.println("[TB][MQTT] Telemetry skipped (MQTT not connected)");
    return false;
  }

  // Publica al topic de telemetría estándar
  bool ok = client.publish(TB_TELEMETRY_TOPIC, json);

  if (ok) {
    Serial.print("[TB][MQTT] Telemetry sent: ");
    Serial.println(json);
  } else {
    Serial.println("[TB][MQTT] Telemetry publish FAILED ❌");
  }

  return ok;
}

/*
  loop()
  ------
  Reconexión básica NO bloqueante:
  - Si WiFi no está, no intentamos MQTT.
  - Si MQTT está caído, intentamos reconectar cada 5s.
  - Ejecutamos client.loop() para mantener la sesión viva.
*/
void loop() {
  // Sin WiFi no hay MQTT
  if (WiFi.status() != WL_CONNECTED) return;

  // Si no está conectado, reintenta cada intervalo
  if (!client.connected()) {
    const uint32_t nowMs = millis();
    if (nowMs - lastMqttAttemptMs >= MQTT_RECONNECT_INTERVAL_MS) {
      lastMqttAttemptMs = nowMs;
      connectOnce(); // un intento, sin while infinito
    }
  }

  // Procesa el stack MQTT (mantiene viva la conexión)
  client.loop();
}

} // namespace tb_mqtt
