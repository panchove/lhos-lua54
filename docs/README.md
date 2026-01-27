# lhOS - Microkernel Híbrido para ESP32-S3

## Descripción

lhOS es un microkernel híbrido basado en **ESP-IDF 5.5.2** y **FreeRTOS**, que utiliza **Lua 5.4** como runtime asíncrono para la lógica de negocio e intercambio de datos entre procesos (IPC). Diseñado para el hardware ESP32-S3, prioriza la eficiencia, la asincronía y la modularidad.

### Características Principales
- **Dual-Core Split**: Core 0 para protocolos críticos (WiFi 6, BLE 5.0, TCP/IP, drivers de interrupción); Core 1 para VM Lua y lógica asíncrona.
- **Asincronía**: Dispatcher central gestiona coroutines de Lua en una única tarea de FreeRTOS.
- **Memoria**: Uso prioritario de PSRAM para el heap de Lua.
- **Bindings C-Lua**: Exposición de hardware (UART, ADC, LEDs, WiFi) a scripts Lua.
- **Filesystem**: LittleFS para almacenamiento de scripts, permitiendo actualizaciones sin reflashear el firmware.
- **Patrones de Diseño**: Reactor Pattern, Zero-Copy IPC, FMEA.

### Stack Tecnológico
- **Hardware**: ESP32-S3 (Xtensa LX7 Dual Core, 8MB Flash).
- **Framework**: ESP-IDF v5.5.2.
- **Lenguaje**: C17 con extensiones GNU, Lua 5.4.
- **RTOS**: FreeRTOS.
- **Filesystem**: LittleFS.
- **Estándares**: ISO 12207 / ISO 15288 para documentación y procesos.

### Estructura del Proyecto
- `main/`: Código principal, scripts Lua en `lua_fs/scripts/`.
- `components/`: Componentes lhOS (lhos_lua, lhos_config, etc.).
- `docs/`: Documentación (esta carpeta).
- `tests/`: Pruebas unitarias con Unity.

## Instalación
1. Clona el repositorio: `git clone <url>`.
2. Configura ESP-IDF v5.5.2 en PowerShell (sin MSYS2).
3. Ejecuta `export.ps1` para configurar el entorno.
4. Build: `idf.py build`.
5. Flash: `idf.py -p COMx flash`.
6. Flash imagen LittleFS: `python -m esptool --chip esp32s3 -p COMx write_flash 0x4a0000 build/main.bin`.

## Uso
- Scripts Lua se cargan desde `/lfs/scripts/` al boot.
- Ejemplo: `post_soft.lua` realiza POST checks de hardware.
- Bindings: `lhos.uart.open(1, 9600)` para UART1.

## Contribución
Sigue las convenciones de código: Macros `LHOS_`, tipos `lhos_`, headers con include guards.

## Licencia
[Especificar licencia, e.g., MIT].