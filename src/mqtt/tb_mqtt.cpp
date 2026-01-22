#include "tb_mqtt.h"
#include <WiFi.h>            // Para verificar estado WiFi antes de MQTT
#include <PubSubClient.h>    // Cliente MQTT liviano (PlatformIO: knolleary/PubSubClient)
#include "secrets.h"         // TB_SERVER, TB_PORT, TB_TOKEN (NO versionado)

// Cliente TCP que usará MQTT por debajo
static WiFiClient espClient;

// Cliente MQTT basado en WiFiClient
static PubSubClient client(espClient);

// Topic estándar para telemetría en ThingsBoard (MQTT)
// ThingsBoard espera telemetría publicada en: v1/devices/me/telemetry
static const char* TB_TELEMETRY_TOPIC = "v1/devices/me/telemetry";

namespace tb_mqtt {

/*
  begin()
  -------
  Configura a qué broker/puerto se conectará PubSubClient.

  IMPORTANTE:
  - Esto solo setea el servidor.
  - La conexión real ocurre en loop(), de forma controlada.
*/
void begin() {
  client.setServer(TB_SERVER, TB_PORT);
}

/*
  isConnected()
  -------------
  Devuelve el estado actual de la conexión MQTT.
*/
bool isConnected() {
  return client.connected();
}

/*
  connectOnce()
  -------------
  Intenta conectarse UNA vez a ThingsBoard.

  ThingsBoard MQTT Auth:
  - username = TB_TOKEN (token del dispositivo)
  - password = vacío (nullptr)

  Nota:
  - clientId debe ser único para evitar colisiones.
  - Si falla, se imprime el código rc para diagnóstico.
*/
static void connectOnce() {
  Serial.print("[TB][MQTT] Connecting to ");
  Serial.print(TB_SERVER);
  Serial.print(":");
  Serial.print(TB_PORT);
  Serial.print(" ... ");


  // clientId: identificador MQTT del cliente.
  // Puedes personalizarlo si quieres que sea único por dispositivo:
  // por ejemplo: "ESP32_GE83_001"
  const char* clientId = "ESP32_GE83";

  // connect(clientId, username, password)
  bool ok = client.connect(clientId, TB_TOKEN, nullptr);

  if (ok) {
    Serial.println("CONNECTED ✅");
  } else {
    // client.state() entrega el reason code de PubSubClient
    // Valores comunes:
    //  -2: conexión fallida
    //  4: bad username/password (token incorrecto)
    //  5: not authorized
    Serial.print("FAILED ❌ rc=");
    Serial.println(client.state());
  }
}

/*
  publishTelemetry(json)
  ----------------------
  Publica un payload JSON hacia ThingsBoard.

  Reglas:
  - Solo publica si MQTT está conectado.
  - Reporta por Serial si se envió o falló (sin exponer secretos).
*/
bool publishTelemetry(const char* json) {
  if (!client.connected()) {
    Serial.println("[TB] Telemetry skipped (MQTT not connected)");
    return false;
  }

  // publish(topic, payload)
  bool ok = client.publish(TB_TELEMETRY_TOPIC, json);

  if (ok) {
    Serial.print("[TB][MQTT] Telemetry sent: ");
    Serial.println(json);
  } else {
    Serial.println("[TB] Telemetry publish FAILED ❌");
  }

  return ok;
}

/*
  loop()
  ------
  Debe llamarse en cada iteración del loop principal.

  Funciones:
  - Verifica que WiFi esté conectado antes de intentar MQTT
  - Si MQTT no está conectado:
      * intenta conectarse (1 intento)
      * espera un poco para no spamear reintentos
  - Mantiene viva la sesión MQTT con client.loop()
*/
void loop() {
  // Si WiFi no está conectado, no intentamos MQTT
  // (la gestión de WiFi se maneja fuera, en wifi_manager)
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // Si el cliente MQTT no está conectado, intentamos conectar
  if (!client.connected()) {
    connectOnce();

    // Delay para evitar reintentos excesivos
    // Esto previene spam de logs y carga innecesaria si el token está mal o no hay broker
    delay(3000);
  }

  // Mantiene viva la conexión MQTT y procesa mensajes pendientes
  client.loop();
}

} // namespace tb_mqtt
