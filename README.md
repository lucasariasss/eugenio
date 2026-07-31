# Proyecto ESP32 con interfaz web

Este repositorio contiene una aplicación para ESP32 basada en **ESP-IDF**. El firmware arranca como punto de acceso y expone una pequeña página web para controlar el simulador. El soporte opcional de estación permite además configurar una red Wi-Fi desde esa interfaz.

## Características

- Arranque inicial en modo **Access Point** (`Eugenio001`) para facilitar la configuración.
- Servidor HTTP integrado que sirve la interfaz web ubicada en `main/webpage/`.
- Variante opcional **Access Point + Station**, con credenciales guardadas en NVS.
- Envío de un byte por UART con la patología seleccionada en la web.
- Tareas FreeRTOS dedicadas para WiFi, HTTP y UART.
- Contenedor de desarrollo basado en Docker para reproducir el entorno de ESP-IDF y QEMU.

## Estructura del repositorio

```
CMakeLists.txt       - Proyecto ESP-IDF
main/
    main.c          - Punto de entrada
    wifi_app.*      - Manejo de WiFi y eventos
    http_server.*   - Servidor web y API REST
    uart_app.*      - Envío de comandos por UART
    app_nvs.*       - Almacenamiento de credenciales
    tasks_common.h  - Definiciones de tareas
    webpage/        - Archivos HTML, CSS, JS e imágenes
.devcontainer/      - Archivos para VS Code y Docker
```

## Requisitos

- ESP-IDF 5.2.1
- Herramientas de ESP-IDF agregadas al `PATH`

Opcionalmente se puede usar el contenedor que se encuentra en `.devcontainer` para evitar instalaciones manuales.

## Compilación rápida

```bash
idf.py set-target esp32
idf.py menuconfig   # configurar opciones si es necesario
idf.py build
idf.py flash monitor
```

El build predeterminado usa solamente el modo AP (`HAS_STA_MODE=0`). Las dos variantes se pueden construir de forma reproducible en directorios separados:

```bash
# Access Point
idf.py -B build-ap -DHAS_STA_MODE=0 build

# Access Point + Station
idf.py -B build-sta -DHAS_STA_MODE=1 build
```

Si se utiliza el contenedor, estas instrucciones se ejecutan dentro de la terminal del mismo.

## Base del proyecto

Este proyecto toma como base los conocimientos adquiridos en el curso [IoT Application Development with the ESP32 Using the ESP-IDF](https://www.udemy.com/course/iot-application-development-with-the-esp32-using-the-esp-idf/), de Kevin Aguilar (Udemy), adaptados a los requisitos del simulador Eugenio.

## Licencia

El código está disponible bajo dominio público (o CC0, a elección) según el archivo [LICENSE](LICENSE).
