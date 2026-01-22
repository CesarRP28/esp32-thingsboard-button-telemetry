#include <Arduino.h>                 // Librería base de Arduino
#include "wifi/wifi_manager.h"       // Módulo propio para conexión WiFi (GE-82)
#include "mqtt/tb_mqtt.h"            // Módulo propio para MQTT ThingsBoard (GE-83/84)

// GPIO donde está conectado el botón
// Se usa INPUT_PULLUP, por lo que:
//  - HIGH  -> botón liberado
//  - LOW   -> botón presionado
static const int BUTTON_PIN = 25;

// Variable para almacenar el último estado del botón
// Se inicializa en HIGH (botón liberado)
int lastState = HIGH;

void setup() {
  // Inicializa la comunicación serial para depuración
  Serial.begin(115200);
  delay(300); // Pequeña espera para estabilizar el puerto serial

  // Configura el pin del botón como entrada con resistencia pull-up interna
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Mensajes de arranque (útiles para evidencia y debugging)
  Serial.println();
  Serial.println("=================================");
  Serial.println("GE-84 | Button Telemetry to ThingsBoard");
  Serial.println("=================================");
  Serial.print("Button pin: GPIO ");
  Serial.println(BUTTON_PIN);
  Serial.println("Expected: RELEASED=HIGH, PRESSED=LOW");
  Serial.println();

  // Conexión a la red WiFi (credenciales en secrets.h)
  // Este método bloquea hasta conectar o agotar el timeout
  wifi::connect(20000, 500);

  // Inicializa el cliente MQTT para ThingsBoard
  // (configura servidor y puerto, no conecta aún)
  tb_mqtt::begin();
}

void loop() {
  // Mantiene viva la conexión MQTT:
  // - intenta reconectar si se pierde
  // - procesa tráfico MQTT
  tb_mqtt::loop();

  // Lee el estado actual del botón
  int currentState = digitalRead(BUTTON_PIN);

  // Detecta cambio de estado (flanco)
  // Esto evita enviar telemetría repetida en cada iteración del loop
  if (currentState != lastState) {

    // Actualiza el último estado conocido
    lastState = currentState;

    // Determina si el botón está presionado
    // Con INPUT_PULLUP: LOW = PRESSED
    const bool pressed = (currentState == LOW);

    // Mensaje local por Serial (útil para evidencia)
    if (pressed) {
      Serial.println("Button PRESSED (LOW)");
    } else {
      Serial.println("Button RELEASED (HIGH)");
    }

    // Solo envía telemetría si MQTT está conectado
    if (tb_mqtt::isConnected()) {

      // Buffer para el payload JSON
      // Tamaño suficiente para el mensaje
      char payload[96];

      // Construye el JSON a enviar a ThingsBoard
      // button:     1 (presionado) / 0 (liberado)
      // button_str: "PRESSED" / "RELEASED"
      snprintf(payload, sizeof(payload),
               "{\"button\":%d,\"button_str\":\"%s\"}",
               pressed ? 1 : 0,
               pressed ? "PRESSED" : "RELEASED");

      // Publica la telemetría al topic estándar de ThingsBoard
      tb_mqtt::publishTelemetry(payload);

    } else {
      // Si MQTT no está conectado, se omite el envío
      Serial.println("[TB] Not connected, telemetry skipped");
    }
  }

  // Pequeña pausa para reducir carga de CPU y rebotes simples
  delay(20);
}
