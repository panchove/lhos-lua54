# Guía de Usuario de lhOS

## Inicio
Al boot, lhOS monta LittleFS en `/lfs` y carga `post_soft.lua` desde `/lfs/scripts/`.

## Scripts Lua
- Ubicación: `main/lua_fs/scripts/`.
- Ejemplo `post_soft.lua`: Realiza checks de hardware (UART, ADC, LEDs, WiFi).
- Para actualizar: Edita los scripts, rebuild `main.bin`, flash solo la imagen.

## Uso de UART
```lua
-- En tu script
lhos.uart.open(1, 9600)  -- UART1 en GPIO 39/40
lhos.uart.write(1, "Data")
local response = lhos.uart.read(1, 10)
lhos.uart.close(1)
```

## POST Checks
El script `post_soft.lua` verifica:
- UART: Envía/receive datos.
- ADC: Lee valores.
- LEDs: Enciende/apaga.
- WiFi: Conecta y verifica.

Si falla, loggea errores.

## Asincronía
Usa `lhos.yield()` en loops para no bloquear.

## Logs
- Usa `print()` en Lua para logs seriales.
- C logs via ESP_LOG.

## Actualizaciones
- Scripts: Flash `main.bin` sin tocar el firmware.
- Firmware: Full flash con `idf.py flash`.