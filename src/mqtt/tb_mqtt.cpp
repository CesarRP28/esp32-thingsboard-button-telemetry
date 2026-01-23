#include "tb_mqtt.h"          // Nuestro header: declara begin(), loop(), publishTelemetry(), etc.
#include <WiFi.h>             // Para verificar estado de WiFi (WL_CONNECTED)
#include <PubSubClient.h>     // Librería MQTT (cliente MQTT para Arduino/ESP32)
#include "secrets.h"          // Variables sensibles: TB_HOST, TB_PORT, TB_TOKEN (NO se sube al repo)

/* --------------------------------------------------------------------------
   1) Objetivo de este módulo (tb_mqtt)
   --------------------------------------------------------------------------
   - Mantener una conexión MQTT con ThingsBoard
   - Publicar telemetría (ESP32 -> ThingsBoard) en el topic estándar:
       v1/devices/me/telemetry
   - Recibir comandos remotos RPC (ThingsBoard -> ESP32) suscribiéndonos a:
       v1/devices/me/rpc/request/+
   - Implementar reconexión básica no bloqueante (reintentos cada N ms)
   -------------------------------------------------------------------------- */


/* --------------------------------------------------------------------------
   2) Objetos MQTT base
   --------------------------------------------------------------------------
   PubSubClient necesita un "transporte" TCP. Para ESP32 se usa WiFiClient.
   - espClient: canal TCP sobre WiFi
   - client:    cliente MQTT que usa espClient por debajo
   -------------------------------------------------------------------------- */
static WiFiClient espClient;
static PubSubClient client(espClient);


/* --------------------------------------------------------------------------
   3) Topics estándar de ThingsBoard
   --------------------------------------------------------------------------
   - Telemetry topic:
       v1/devices/me/telemetry
     Aquí publicas JSON con claves/valores.
     Ej: {"button":1,"button_str":"PRESSED"}

   - RPC request subscription topic:
       v1/devices/me/rpc/request/+
     "request/+" significa: “suscríbete a todos los requests con cualquier id”
     ThingsBoard envía comandos a ese topic.
   -------------------------------------------------------------------------- */
static const char* TB_TELEMETRY_TOPIC = "v1/devices/me/telemetry";
static const char* TB_RPC_SUB_TOPIC   = "v1/devices/me/rpc/request/+";


/* --------------------------------------------------------------------------
   4) Reconexión MQTT no bloqueante
   --------------------------------------------------------------------------
   - lastMqttAttemptMs guarda cuándo fue el último intento
   - MQTT_RECONNECT_INTERVAL_MS define el intervalo (5 segundos)
   Esto evita que el ESP32 "spamee" intentos de conexión en cada iteración.
   -------------------------------------------------------------------------- */
static uint32_t lastMqttAttemptMs = 0;
static const uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;


/* --------------------------------------------------------------------------
   5) Handler RPC (callback) registrado desde main.cpp
   --------------------------------------------------------------------------
   Queremos que el módulo MQTT NO decida qué hacer con el comando.
   Solo:
     - Recibe el RPC
     - Extrae 0/1
     - Llama al handler registrado en main (ej: prender LED, cambiar variable, etc.)

   RpcHandler es un tipo definido en tb_mqtt.h:
     typedef void (*RpcHandler)(int value);
   -------------------------------------------------------------------------- */
static tb_mqtt::RpcHandler rpcHandler = nullptr;

namespace tb_mqtt {

/* --------------------------------------------------------------------------
   setRpcHandler()
   --------------------------------------------------------------------------
   Permite que main.cpp registre una función para manejar el comando RPC.
   Ejemplo:
     tb_mqtt::setRpcHandler(onRpcCommand);
   -------------------------------------------------------------------------- */
void setRpcHandler(RpcHandler handler) {
  rpcHandler = handler;
}

/* --------------------------------------------------------------------------
   onMessage() - Callback MQTT
   --------------------------------------------------------------------------
   PubSubClient llama a esta función cuando llega un mensaje MQTT.
   Aquí recibimos RPC desde ThingsBoard.

   - topic ejemplo:
       v1/devices/me/rpc/request/12
     (12 es el requestId generado por ThingsBoard)

   - payload típico:
       {"method":"setValue","params":true}
     o
       {"method":"setValue","params":1}

   Nota: aquí hacemos un parseo MINIMO (string search), sin librerías JSON,
         para mantenerlo simple y rápido.
   -------------------------------------------------------------------------- */
static void onMessage(char* topic, byte* payload, unsigned int length) {

  /* ----------------------------
     1) Convertir payload a string
     ----------------------------
     PubSubClient nos da "payload" como bytes (byte*).
     Para poder usar strstr() necesitamos un char[] con terminación '\0'.
  */
  static char msg[256];                 // buffer fijo para el mensaje
  if (length >= sizeof(msg))            // si llega algo muy grande, lo cortamos
    length = sizeof(msg) - 1;

  memcpy(msg, payload, length);         // copiamos bytes -> buffer char
  msg[length] = '\0';                   // terminación de string

  // Logs de depuración: ayudan para evidencias y troubleshooting
  Serial.print("[TB][RPC] Topic: ");
  Serial.println(topic);
  Serial.print("[TB][RPC] Payload: ");
  Serial.println(msg);

  /* ----------------------------
     2) Parseo mínimo del JSON
     ----------------------------
     Queremos extraer el valor de "params" como 0 o 1.
     - Si params=true  -> 1
     - Si params=false -> 0
     - Si params=1     -> 1
     - Si params=0     -> 0

     value=-1 significa "no pude interpretarlo".
  */
  int value = -1;

  // Caso booleano
  if (strstr(msg, "true")  != nullptr) value = 1;
  if (strstr(msg, "false") != nullptr) value = 0;

  // Caso numérico explícito (más exacto)
  if (strstr(msg, "\"params\":1") != nullptr) value = 1;
  if (strstr(msg, "\"params\":0") != nullptr) value = 0;

  /* ----------------------------
     3) Si interpretamos 0/1, ejecutamos handler
     ---------------------------- */
  if (value == 0 || value == 1) {
    Serial.print("[TB][RPC] Parsed value: ");
    Serial.println(value);

    // Si main registró un handler, lo llamamos
    if (rpcHandler) {
      rpcHandler(value); // main decide: LED, variable, etc.
    }

    /* ----------------------------
       4) Responder al request RPC (opcional)
       ----------------------------
       ThingsBoard puede esperar una respuesta al request.
       El topic estándar de respuesta es:
         v1/devices/me/rpc/response/<requestId>

       Para obtener requestId:
       - extraemos la última parte del topic después del último '/'
         ejemplo: ".../request/12" -> requestId="12"
    */
    const char* lastSlash = strrchr(topic, '/');
    if (lastSlash) {
      char respTopic[128];
      snprintf(respTopic, sizeof(respTopic),
               "v1/devices/me/rpc/response/%s", lastSlash + 1);

      // Respuesta simple JSON (puedes mejorarla luego)
      const char* resp = "{\"status\":\"ok\"}";
      client.publish(respTopic, resp);
    }

  } else {
    // No pudimos interpretar params: dejamos log para depurar
    Serial.println("[TB][RPC] Could not parse params (expected true/false/1/0).");
  }
}

/* --------------------------------------------------------------------------
   begin()
   --------------------------------------------------------------------------
   Configura:
   - Host y puerto del broker MQTT (ThingsBoard)
   - Callback para mensajes entrantes (RPC)

   Importante: begin() NO conecta. Solo configura.
   La conexión real ocurre en loop() (connectOnce()).
   -------------------------------------------------------------------------- */
void begin() {
  client.setServer(TB_HOST, TB_PORT);      // broker (host:puerto)
  client.setCallback(onMessage);           // callback para RPC
}

/* --------------------------------------------------------------------------
   isConnected()
   --------------------------------------------------------------------------
   Devuelve true si el cliente MQTT está conectado al broker.
   Útil para evitar publicar telemetría cuando no hay conexión.
   -------------------------------------------------------------------------- */
bool isConnected() {
  return client.connected();
}

/* --------------------------------------------------------------------------
   connectOnce()
   --------------------------------------------------------------------------
   Intenta conectarse una vez a ThingsBoard por MQTT.

   En ThingsBoard MQTT:
   - Username = TB_TOKEN
   - Password = vacío (nullptr)

   Si conecta:
   - Suscribe al topic RPC request
   -------------------------------------------------------------------------- */
static void connectOnce() {
  Serial.print("[TB][MQTT] Connecting to ");
  Serial.print(TB_HOST);
  Serial.print(":");
  Serial.print(TB_PORT);
  Serial.print(" ... ");

  // Identificador del cliente MQTT (puede ser cualquier string “único”)
  const char* clientId = "ESP32_GE87";

  // Conectar a broker:
  // client.connect(clientId, username, password)
  bool ok = client.connect(clientId, TB_TOKEN, nullptr);

  if (ok) {
    Serial.println("CONNECTED ✅");

    // Al conectarnos, suscribimos para recibir RPC desde ThingsBoard
    bool subOk = client.subscribe(TB_RPC_SUB_TOPIC);
    Serial.print("[TB][RPC] Subscribe ");
    Serial.print(TB_RPC_SUB_TOPIC);
    Serial.println(subOk ? " ✅" : " ❌");

  } else {
    // Si falla, imprimimos el "state()" para depurar
    Serial.print("FAILED ❌ rc=");
    Serial.println(client.state());
  }
}

/* --------------------------------------------------------------------------
   publishTelemetry(const char* json)
   --------------------------------------------------------------------------
   Publica un payload JSON al topic estándar de telemetría.
   Ejemplo:
     {"button":1,"button_str":"PRESSED"}

   Retorna:
   - true si publish fue exitoso
   - false si no pudo publicar (ej: MQTT desconectado)
   -------------------------------------------------------------------------- */
bool publishTelemetry(const char* json) {
  if (!client.connected()) return false;    // seguridad: si no hay MQTT, no publiquemos

  bool ok = client.publish(TB_TELEMETRY_TOPIC, json);
  if (ok) {
    Serial.print("[TB][MQTT] Telemetry sent: ");
    Serial.println(json);
  } else {
    Serial.println("[TB][MQTT] Telemetry publish FAILED ❌");
  }
  return ok;
}

/* --------------------------------------------------------------------------
   loop()
   --------------------------------------------------------------------------
   Esta función debe llamarse continuamente desde main.cpp (dentro de loop()).

   Responsabilidades:
   1) Verifica que haya WiFi antes de intentar MQTT.
   2) Si MQTT está caído, intenta reconectar cada 5 segundos (no bloqueante).
   3) Llama a client.loop() para:
      - mantener viva la sesión MQTT
      - recibir mensajes entrantes (RPC)
      - disparar onMessage() cuando llegue un comando

   Importante:
   - Si NO llamas client.loop(), NO recibirás RPC.
   -------------------------------------------------------------------------- */
void loop() {
  // Si no hay WiFi, no tiene sentido intentar MQTT (broker no es accesible)
  if (WiFi.status() != WL_CONNECTED) return;

  // Si MQTT está desconectado, reconectamos con intervalo para no spamear
  if (!client.connected()) {
    uint32_t nowMs = millis();
    if (nowMs - lastMqttAttemptMs >= MQTT_RECONNECT_INTERVAL_MS) {
      lastMqttAttemptMs = nowMs;
      connectOnce();
    }
  }

  // Mantiene MQTT vivo y procesa mensajes entrantes (RPC)
  client.loop();
}

} // namespace tb_mqtt
