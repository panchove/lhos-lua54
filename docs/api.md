# API de lhOS - Bindings Lua

## Introducción

Los bindings de lhOS permiten acceder al hardware desde scripts Lua de manera segura y asíncrona. Todos los bindings están en la tabla global `lhos`.

## Módulos

### uart
Control de puertos UART (RS232).

- `uart.open(port, baud)`: Abre el puerto UART (1 o 2) a la velocidad especificada. Retorna true si éxito.
- `uart.close(port)`: Cierra el puerto UART.
- `uart.write(port, data)`: Escribe datos al puerto.
- `uart.read(port, len)`: Lee hasta `len` bytes del puerto.

Ejemplo:
```lua
uart.open(1, 9600)
uart.write(1, "Hello RS232")
local data = uart.read(1, 100)
uart.close(1)
```

### adc
Lectura de ADC.

- `adc.read(channel)`: Lee el valor del canal ADC especificado. Retorna el valor numérico.

Ejemplo:
```lua
local value = adc.read(0)
print("ADC Value: " .. value)
```

### led
Control de LEDs.

- `led.config(pin)`: Configura el pin GPIO como salida para LED o inicializa el driver WS2812b.
- `led.set(r, g, b)` / `led.set(pin, state)`: Enciende/apaga o establece color en WS2812b según implementación.
- `led.get(pin)`: Retorna el estado actual del LED (si está soportado).

Ejemplo:
```lua
led.config(2)
led.set(2, true)  -- Enciende LED en GPIO 2 (GPIO-style API)
-- o para WS2812b
led.set(255, 0, 0) -- rojo
local state = led.get(2)
print("LED state: " .. tostring(state))
```

### wifi
Gestión de WiFi.

- `wifi.init()`: Inicializa WiFi.
- `wifi.connect(ssid, password)`: Conecta a la red WiFi.
- `wifi.disconnect()`: Desconecta.
- `wifi.get_status()`: Retorna el estado de la conexión (bool o string, según binding).

Ejemplo:
```lua
wifi.init()
wifi.connect("MyNetwork", "password")
if wifi.get_status() then
    print("WiFi OK")
end
```

### lhos.ble
Gestión de Bluetooth Low Energy (BLE). (Pendiente de implementación)

- Próximamente: `lhos.ble.init()`, `lhos.ble.advertise()`, etc.

Ejemplo:
```lua
-- Pendiente
```

### Nuevos bindings

Se añadieron bindings adicionales disponibles para los scripts Lua:

- `post`: Acceso al POST (Power On Self Test). Provee `post.send()` y `post.receive()` que retornan una tabla con flags booleanos: `ram_ok`, `flash_ok`, `gpio_ok`, `psram_ok`, `nvs_ok`, `wifi_ok`, `ble_ok`.
- `posix`: Operaciones POSIX básicas de fichero (`open`, `read`, `write`, `close`, `stat`, `unlink`, `rename`, `mkdir`) para trabajar con el sistema de archivos desde Lua.
- `led` (WS2812b): Control de tiras inteligentes mediante el componente `lhos_ws2812b`. Funciones: `led.set()`, `led.off()`, `led.flash()`.

Ejemplo de uso (Lua):

```lua
local status = post.send()
if status and status.ram_ok then
    print("RAM OK")
end
```

## Seguridad
- Siempre validar tipos de argumentos en la pila de Lua (`lua_gettop`, `luaL_checktype`).
- Usar `lhos.yield()` para operaciones asíncronas y no bloquear el Event Loop.

## Errores
Los bindings lanzan errores Lua en caso de fallos (e.g., puerto inválido).