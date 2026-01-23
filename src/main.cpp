#include <Arduino.h>              // API base de Arduino (setup/loop, pinMode, digitalRead, etc.)
#include "wifi/wifi_manager.h"    // Módulo propio: maneja conexión y reconexión WiFi
#include "mqtt/tb_mqtt.h"         // Módulo propio: maneja conexión MQTT + telemetría + RPC (ThingsBoard)

/* --------------------------------------------------------------------------
   1) Definición de pines y estados iniciales
   -------------------------------------------------------------------------- */

// Pin del botón físico. En GE-81/84 usas GPIO25 como pin “seguro” para entrada.
static const int BUTTON_PIN = 25;

// Pin del LED. Muchas placas ESP32 usan GPIO2 como LED onboard.
// Si tu placa no tiene LED en GPIO2, puedes cambiar este pin o usar solo Serial.
static const int LED_PIN    = 2;

/*
  lastState guarda el último estado leído del botón.
  - Con INPUT_PULLUP:
    * RELEASED (no presionado) = HIGH
    * PRESSED  (presionado)    = LOW
*/
int lastState = HIGH;

/*
  remoteState representa un “estado remoto” recibido desde ThingsBoard vía RPC.
  - 0 = OFF
  - 1 = ON
  Se marca volatile porque puede cambiar desde un callback (onRpcCommand),
  y volatile le dice al compilador: “no asumas que este valor es constante”.
*/
volatile int remoteState = 0;


/* --------------------------------------------------------------------------
   2) Handler RPC: ThingsBoard -> ESP32 (bidireccional)
   --------------------------------------------------------------------------
   Esta función será llamada por tb_mqtt cuando llegue un mensaje RPC desde ThingsBoard.
   Ejemplo de RPC:
     {"method":"setValue","params":true}
   tb_mqtt lo parsea y lo convierte en value = 1 (ON) o value = 0 (OFF)
   -------------------------------------------------------------------------- */
void onRpcCommand(int value) {
  // Guardamos el valor remoto recibido (0/1)
  remoteState = value;

  // Acción visible: controlar el LED en el ESP32
  // - Si value=1 => HIGH (LED encendido)
  // - Si value=0 => LOW  (LED apagado)
  digitalWrite(LED_PIN, value ? HIGH : LOW);

  // Log para evidencia (vemos que el ESP32 recibió y aplicó el comando)
  Serial.print("[APP] RPC command applied. remoteState=");
  Serial.println(remoteState);

  // Confirmación hacia ThingsBoard:
  // Publicamos telemetría indicando qué comando llegó y qué estado quedó aplicado.
  // Esto cierra el ciclo bidireccional:
  // - TB manda comando
  // - ESP32 aplica
  // - ESP32 confirma enviando telemetría de vuelta
  if (tb_mqtt::isConnected()) {
    char payload[96]; // buffer para armar el JSON (pequeño y suficiente)

    // snprintf construye el JSON de forma segura evitando overflow (por el tamaño definido)
    snprintf(payload, sizeof(payload),
             "{\"remote_cmd\":%d,\"remote_state\":%d}",
             value, remoteState);

    // Enviar telemetría al topic estándar de ThingsBoard
    tb_mqtt::publishTelemetry(payload);
  }
}


/* --------------------------------------------------------------------------
   3) setup(): se ejecuta una sola vez al arrancar el ESP32
   -------------------------------------------------------------------------- */
void setup() {
  // Inicializa el monitor serial para logs y evidencias
  Serial.begin(115200);
  delay(300); // pequeña espera para que se estabilice el Serial en el arranque

  // Configuración del botón con resistencia PULLUP interna:
  // - botón liberado: HIGH
  // - botón presionado: LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configuración del LED como salida digital
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // arrancamos con LED apagado

  // Banner del ticket: sirve para evidencia y para identificar versión
  Serial.println();
  Serial.println("=================================");
  Serial.println("GE-87 | Device + Telemetry + RPC");
  Serial.println("=================================");
  Serial.print("Button pin: GPIO ");
  Serial.println(BUTTON_PIN);
  Serial.println("Expected: RELEASED=HIGH, PRESSED=LOW");
  Serial.println();

  /* ----------------------------
     Conectar WiFi (bloqueante solo en setup)
     ----------------------------
     wifi::connect(timeoutMs, retryDelayMs)
     - Espera hasta conectar o agotar el tiempo.
     - Si falla, el wifi::loop() seguirá reintentando en segundo plano.
  */
  wifi::connect(20000, 500);

  /* ----------------------------
     Inicializar MQTT + RPC
     ----------------------------
     tb_mqtt::begin() configura broker, callbacks, etc.
     La conexión real se mantiene en tb_mqtt::loop().
  */
  tb_mqtt::begin();

  // Registrar handler RPC:
  // A partir de aquí, cuando ThingsBoard envíe un RPC, tb_mqtt llamará onRpcCommand(value)
  tb_mqtt::setRpcHandler(onRpcCommand);
}


/* --------------------------------------------------------------------------
   4) loop(): se ejecuta repetidamente (muchas veces por segundo)
   --------------------------------------------------------------------------
   IMPORTANTE:
   - Aquí NO debemos bloquear con delays grandes.
   - Aquí mantenemos vivos WiFi y MQTT (reconexión básica).
   - Detectamos cambios del botón y enviamos telemetría solo cuando cambia.
   -------------------------------------------------------------------------- */
void loop() {
  /* ----------------------------
     Mantener conectividad
     ----------------------------
     - wifi::loop() reintenta WiFi si se cae (no bloqueante)
     - tb_mqtt::loop() reintenta MQTT si se cae (no bloqueante)
  */
  wifi::loop();
  tb_mqtt::loop();

  /* ----------------------------
     Botón físico -> ThingsBoard (telemetría por evento)
     ----------------------------
     Leemos el pin y comparamos con lastState.
     Solo cuando hay cambio:
       - imprimimos estado (pressed/released)
       - enviamos telemetría JSON
  */
  int currentState = digitalRead(BUTTON_PIN);

  // Detecta cambio (edge detection): evita spamear mensajes
  if (currentState != lastState) {
    lastState = currentState;

    // pressed = true cuando el pin está LOW (por INPUT_PULLUP)
    const bool pressed = (currentState == LOW);

    // Logs de evidencia local
    if (pressed) Serial.println("Button PRESSED (LOW)");
    else         Serial.println("Button RELEASED (HIGH)");

    // Enviar telemetría solo si MQTT está conectado
    if (tb_mqtt::isConnected()) {
      char payload[96];

      // Construimos JSON con:
      // - button: 1/0
      // - button_str: texto "PRESSED"/"RELEASED"
      snprintf(payload, sizeof(payload),
               "{\"button\":%d,\"button_str\":\"%s\"}",
               pressed ? 1 : 0,
               pressed ? "PRESSED" : "RELEASED");

      tb_mqtt::publishTelemetry(payload);
    }
  }

  // Delay corto para reducir rebote y ruido (anti-rebote simple)
  // Nota: para un debounce “formal” se puede mejorar en otro ticket.
  delay(20);
}
