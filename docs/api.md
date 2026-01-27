# API de lhOS - Bindings Lua

## Introducción

Los bindings de lhOS permiten acceder al hardware desde scripts Lua de manera segura y asíncrona. Todos los bindings están en la tabla global `lhos`.

## Módulos

### lhos.uart
Control de puertos UART (RS232).

- `lhos.uart.open(port, baud)`: Abre el puerto UART (1 o 2) a la velocidad especificada. Retorna true si éxito.
- `lhos.uart.close(port)`: Cierra el puerto UART.
- `lhos.uart.write(port, data)`: Escribe datos al puerto.
- `lhos.uart.read(port, len)`: Lee hasta `len` bytes del puerto.

Ejemplo:
```lua
lhos.uart.open(1, 9600)
lhos.uart.write(1, "Hello RS232")
local data = lhos.uart.read(1, 100)
lhos.uart.close(1)
```

### lhos.adc
Lectura de ADC.

- `lhos.adc.read(channel)`: Lee el valor del canal ADC especificado. Retorna el valor numérico.

Ejemplo:
```lua
local value = lhos.adc.read(0)
print("ADC Value: " .. value)
```

### lhos.led
Control de LEDs.

- `lhos.led.config(pin)`: Configura el pin GPIO como salida para LED.
- `lhos.led.set(pin, state)`: Enciende/apaga el LED en el pin especificado (state: true/false).
- `lhos.led.get(pin)`: Retorna el estado actual del LED (true/false).

Ejemplo:
```lua
lhos.led.config(2)
lhos.led.set(2, true)  -- Enciende LED en GPIO 2
local state = lhos.led.get(2)
print("LED state: " .. tostring(state))
```

### lhos.wifi
Gestión de WiFi.

- `lhos.wifi.init()`: Inicializa WiFi.
- `lhos.wifi.connect(ssid, password)`: Conecta a la red WiFi.
- `lhos.wifi.disconnect()`: Desconecta.
- `lhos.wifi.get_status()`: Retorna el estado de la conexión ("connected" o "disconnected").

Ejemplo:
```lua
lhos.wifi.init()
lhos.wifi.connect("MyNetwork", "password")
if lhos.wifi.get_status() == "connected" then
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

## Seguridad
- Siempre validar tipos de argumentos en la pila de Lua (`lua_gettop`, `luaL_checktype`).
- Usar `lhos.yield()` para operaciones asíncronas y no bloquear el Event Loop.

## Errores
Los bindings lanzan errores Lua en caso de fallos (e.g., puerto inválido).