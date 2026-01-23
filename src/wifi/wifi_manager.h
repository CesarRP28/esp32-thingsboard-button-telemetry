#pragma once
#include <Arduino.h>

/*
  Módulo WiFi Manager
  -------------------
  Responsabilidad:
  - Conectarse a WiFi (connect)
  - Mantener la conexión viva (loop) mediante reconexión básica no bloqueante

  Importante:
  - connect() puede bloquear por un tiempo (timeout) solo al inicio (setup).
  - loop() NO debe bloquear: reintenta cada cierto intervalo (backoff simple).
*/

namespace wifi {

  // Conexión inicial a WiFi.
  // - timeoutMs: tiempo máximo intentando conectar en setup()
  // - retryDelayMs: tiempo entre intentos (y para imprimir puntos)
  void connect(uint32_t timeoutMs = 20000, uint32_t retryDelayMs = 500);

  // Función que se llama en el loop principal para mantener WiFi vivo.
  // Si WiFi se cae, reintenta reconectar sin bloquear.
  void loop();

  // Retorna true si WiFi está conectado.
  bool isConnected();

} // namespace wifi
