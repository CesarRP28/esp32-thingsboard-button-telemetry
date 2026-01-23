#pragma once
#include <Arduino.h>

/*
  Módulo ThingsBoard MQTT
  -----------------------
  Responsabilidad:
  - Configurar y mantener la conexión MQTT contra ThingsBoard
  - Reconectar automáticamente con reintentos temporizados (no bloqueante)
  - Publicar telemetría JSON al topic estándar de ThingsBoard

  Seguridad:
  - TB_TOKEN está en include/secrets.h (ignorado por git)
  - No imprimir token en logs
*/

namespace tb_mqtt {

  // Configura el servidor/puerto del broker MQTT.
  void begin();

  // Mantiene viva la conexión:
  // - intenta reconectar si está caído
  // - procesa el loop MQTT
  void loop();

  // Estado actual de MQTT
  bool isConnected();

  // Publica telemetría en JSON (ThingsBoard).
  bool publishTelemetry(const char* json);

} // namespace tb_mqtt
