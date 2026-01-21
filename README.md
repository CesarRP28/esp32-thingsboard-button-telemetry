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

Se implementó la lectura de un botón/cable en un GPIO del ESP32 usando `INPUT_PULLUP`.
El firmware detecta cambios de estado (pressed/released) y los reporta por el monitor serial.

### Hardware
Estado RELEASED (GPIO en HIGH):
![GE-81 HW Released](docs/images/ge-81-hw-1.png)

Estado PRESSED (GPIO a GND):
![GE-81 HW Pressed](docs/images/ge-81-hw-2.png)

### Evidencia – Monitor serial
Estado RELEASED:
![GE-81 Serial Released](docs/images/ge-81-serial-1.png)

Estado PRESSED:
![GE-81 Serial Pressed](docs/images/ge-81-serial-2.png)
