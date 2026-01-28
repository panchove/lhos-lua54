# Guía de Build y Flash para lhOS

## Requisitos
- ESP-IDF v5.5.2 instalado.
- PowerShell (sin MSYS2).
- Python para esptool.
- ESP32-S3 con 8MB Flash.

- Se recomienda habilitar PSRAM en `idf.py menuconfig` y asignar el heap de Lua a PSRAM (`MALLOC_CAP_SPIRAM`) para mejorar la memoria disponible para scripts.

## Configuración del Entorno
1. Ejecuta `export.ps1` en el directorio del proyecto para configurar ESP-IDF.

Ejemplo PowerShell (rápido):

```powershell
& 'C:\Espressif\frameworks\esp-idf-v5.5.2\export.ps1'
idf.py build
```

## Build
1. `idf.py build`: Compila el proyecto. Genera `lhos-lua54.bin` y `main.bin` (imagen LittleFS).

## Flash
1. Conecta el ESP32 y verifica el puerto COM (e.g., COM3).
2. `idf.py -p COM3 flash`: Flashea el firmware principal.
3. `python -m esptool --chip esp32s3 -p COM3 write_flash 0x4a0000 build/main.bin`: Flashea la imagen LittleFS.

## Monitor
- `idf.py -p COM3 monitor`: Abre el monitor serial para ver logs.

## Particiones
- Tabla personalizada en `partitions.csv`.
- Partición `main` (LittleFS) en 0x4A0000 (1MB).

## Tests
- `Run-Tests.ps1 -TestType unit`: Ejecuta tests unitarios con Unity.

## Troubleshooting
- Si build falla por tamaño, verifica `partitions.csv` y `sdkconfig` (flash size 8MB).
- Para scripts, edita `main/lua_fs/scripts/` y rebuild `main.bin`.