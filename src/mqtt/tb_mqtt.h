#pragma once
#include <Arduino.h>

/*
  Módulo MQTT para ThingsBoard (TB)
  --------------------------------
  Responsabilidad:
  - Configurar el cliente MQTT (broker/puerto)
  - Mantener conexión (reintentos + client.loop)
  - Exponer un método simple para publicar telemetría en el topic estándar de TB

  Seguridad:
  - El token (TB_TOKEN) se lee desde include/secrets.h (archivo ignorado por git)
  - Este módulo NO imprime el token en Serial
*/

namespace tb_mqtt {

  // Inicializa el cliente MQTT (configura servidor/puerto).
  // Nota: No conecta inmediatamente; la conexión real ocurre dentro de loop().
  void begin();

  // Debe llamarse continuamente en el loop principal.
  // - Intenta conectar si no hay conexión
  // - Mantiene la sesión MQTT viva
  void loop();

  // Indica si el cliente MQTT está conectado actualmente.
  bool isConnected();

  // Publica telemetría en ThingsBoard usando JSON.
  // Ejemplo:
  //   {"button":1,"button_str":"PRESSED"}
  //
  // Retorna true si publish() fue exitoso.
  // Nota: si no hay conexión MQTT, retorna false.
  bool publishTelemetry(const char* json);

} // namespace tb_mqtt
