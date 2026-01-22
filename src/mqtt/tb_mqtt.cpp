#include "tb_mqtt.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

static WiFiClient espClient;
static PubSubClient client(espClient);

namespace tb_mqtt {

void begin() {
  client.setServer(TB_SERVER, TB_PORT);
}

bool isConnected() {
  return client.connected();
}

static void connectOnce() {
  Serial.print("[TB] Connecting to ThingsBoard... ");

  // clientId puede ser cualquier string único
  const char* clientId = "ESP32_GE83";

  // En ThingsBoard: username = TB_TOKEN, password vacío
  bool ok = client.connect(clientId, TB_TOKEN, nullptr);

  if (ok) {
    Serial.println("CONNECTED ✅");
  } else {
    Serial.print("FAILED ❌ rc=");
    Serial.println(client.state()); // códigos de PubSubClient
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // Si se cae WiFi, no insistimos aquí. (WiFi manager lo manejará)
    return;
  }

  if (!client.connected()) {
    connectOnce();
    // espera corta para no spamear si el token está mal o el broker no responde
    delay(3000);
  }

  client.loop();
}

} // namespace tb_mqtt
