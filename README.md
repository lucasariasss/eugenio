# Proyecto ESP32 con interfaz web

Este repositorio contiene una aplicación para ESP32 basada en **ESP-IDF**. El firmware arranca como punto de acceso y expone una pequeña página web desde la cual se puede configurar la red WiFi y enviar comandos por UART a otro microcontrolador.

## Características

- Arranque inicial en modo **Access Point** (`Eugenio_AP`) para facilitar la configuración.
- Servidor HTTP integrado que sirve la interfaz web ubicada en `main/webpage/`.
- Posibilidad de conectarse a una red WiFi en modo **Station**, guardando las credenciales en NVS.
- Envió de un byte por UART con la patología seleccionada en la web.
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

- ESP-IDF >= 5.4
- Herramientas de ESP-IDF agregadas al `PATH`

Opcionalmente se puede usar el contenedor que se encuentra en `.devcontainer` para evitar instalaciones manuales.

## Compilación rápida

```bash
idf.py set-target esp32
idf.py menuconfig   # configurar opciones si es necesario
idf.py build
idf.py flash monitor
```

Si se utiliza el contenedor, estas instrucciones se ejecutan dentro de la terminal del mismo.

## Licencia

El código está disponible bajo dominio público (o CC0, a elección) según el archivo [LICENSE](LICENSE).
