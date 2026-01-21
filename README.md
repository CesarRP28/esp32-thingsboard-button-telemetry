# esp32-thingsboard-button-telemetry

Sprint 0 – Proyecto base ESP32 usando PlatformIO.

## GE-80 – Base PlatformIO ESP32
Este proyecto valida el entorno de desarrollo para ESP32.

### ¿Qué hace?
- Compila y flashea firmware en ESP32
- Muestra logs por monitor serial
- Verifica ejecución continua mediante uptime

### Hardware
- ESP32 DevKit (CP2102)
- Conexión USB
![ESP32 DevKit](docs/images/hardware-esp32.png)

### Evidencia

### Monitor serial – ejecución del firmware

El siguiente registro demuestra que el firmware fue compilado, flasheado y ejecutado
correctamente en el ESP32, mostrando un contador de uptime continuo.

![Monitor serial – uptime](docs/images/serial-uptime.png)


## GE-81 – Botón GPIO (evento)

Se implementó la lectura de un botón/cable en el GPIO 25 del ESP32 usando `INPUT_PULLUP`.
El firmware detecta cambios de estado (pressed/released) y los reporta por el monitor serial.

### Hardware
Estado PRESSED (GPIO en GND):  
![GE-81 HW Pressed](docs/images/ge-81-hw-1.png)

Estado RELEASED (GPIO a HIGH):
![GE-81 HW Released](docs/images/ge-81-hw-2.png) 

### Evidencia – Monitor serial

#### Estado PRESSED (GPIO a GND)
![GE-81 Serial Pressed](docs/images/ge-81-serial-1.png)

#### Estado RELEASED (GPIO en HIGH)
![GE-81 Serial Released](docs/images/ge-81-serial-2.png)


## GE-82 – Conexión WiFi (sin credenciales hardcodeadas)

### Descripción
En este ticket se implementa la conexión del ESP32 a una red WiFi utilizando el framework Arduino en PlatformIO, **evitando el hardcodeo de credenciales** dentro del código fuente.  
La solución mantiene una estructura limpia, modular y segura, permitiendo que el firmware se conecte a WiFi sin exponer información sensible en el repositorio.

Esta funcionalidad se integra sobre los tickets previos:
- **GE-80:** Base del proyecto PlatformIO para ESP32.
- **GE-81:** Lectura de botón mediante GPIO con `INPUT_PULLUP`.

---

### Implementación técnica

- La lógica de conexión WiFi se encapsula en un **módulo independiente** (`wifi_manager`), ubicado en `src/wifi/`.
- Las credenciales WiFi se definen en un archivo externo (`secrets.h`) que **no se versiona**.
- Se incluye un archivo plantilla (`secrets_template.h`) como referencia para la configuración local.
- El sistema reporta el estado de conexión, dirección IP y nivel de señal (RSSI) mediante el Monitor Serial.
- La lectura del botón GPIO continúa funcionando de manera correcta mientras el dispositivo está conectado a la red.

---

### Evidencia

La siguiente evidencia corresponde a la ejecución del firmware **GE-82**, donde se observa:

- Inicio correcto del sistema.
- Conexión exitosa del ESP32 a la red WiFi.
- Asignación de dirección IP y reporte del nivel de señal (RSSI).
- Monitoreo periódico del estado de conexión.
- Funcionamiento correcto del botón GPIO durante la conexión WiFi.

![GE-82 Serial Monitor](docs/images/ge-82-serial.png)

