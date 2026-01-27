# Arquitectura de lhOS

## Descripción General

lhOS implementa una arquitectura híbrida de microkernel, combinando el poder de ESP-IDF y FreeRTOS con la flexibilidad de Lua 5.4 para scripting asíncrono. El sistema está dividido en dos cores para optimizar el rendimiento y la separación de responsabilidades.

### Componentes Principales
- **Hardware**: ESP32-S3 con dual core.
- **ESP-IDF**: Framework base para drivers y protocolos.
- **FreeRTOS**: RTOS para multitarea.
- **Lua VM**: Runtime para scripts de usuario.
- **LittleFS**: Filesystem para scripts persistentes.
- **Componentes lhOS**: Módulos específicos (lhos_lua, etc.).

### Diagrama de Arquitectura

```plantuml
@startuml Arquitectura lhOS - Iteración Mejorada

title Arquitectura de lhOS - Microkernel Híbrido para ESP32-S3

package "Hardware" {
    [ESP32-S3] as ESP
    note right of ESP
        Dual Core Xtensa LX7
        8MB Flash, PSRAM
    end note
}

package "ESP-IDF 5.5.2" {
    package "Core 0" {
        [WiFi 6] as WiFi
        [BLE 5.0] as BLE
        [TCP/IP Stack] as TCP
        [Drivers de Interrupción] as Drivers
    }
    package "Core 1" {
        [FreeRTOS] as RTOS
        [VM Lua 5.4] as LuaVM
        note right of LuaVM
            lhOS Runtime
            Corrutinas asíncronas
        end note
        [Dispatcher] as Disp
        note right of Disp
            Gestiona Corrutinas de Lua
            en una única tarea
        end note
    }
    [LittleFS] as FS
    note right of FS
        /lfs/scripts/
        post_soft.lua, rs232.lua
    end note
}

package "Componentes lhOS" {
    [lhos_lua] as LuaComp
    [lhos_config] as Config
    [lhos_post] as Post
    [lhos_wifi] as WifiComp
    [lhos_led] as LedComp
    [lhos_ws2812b] as WS2812
    [lua54] as Lua54
    [lua_engine] as Engine
}

ESP --> Core0
ESP --> Core1

Core0 --> RTOS : Comunicación entre cores
RTOS --> LuaVM
Disp --> LuaVM

LuaVM --> LuaComp : Bindings C-Lua
LuaComp --> UART : lhos.uart
LuaComp --> ADC : lhos.adc
LuaComp --> LedComp : lhos.led
LuaComp --> WifiComp : lhos.wifi

LuaVM --> FS : Carga scripts

Config --> LuaComp
Post --> LuaComp
WifiComp --> LuaComp
LedComp --> LuaComp
WS2812 --> LuaComp
Lua54 --> LuaVM
Engine --> LuaVM

note as Patrones
    **Patrones de Diseño:**
    - Reactor Pattern: Eventos a bus de mensajes, Lua via callbacks.
    - Zero-Copy IPC: Punteros de buffers circulares.
    - FMEA: Recuperación automática en drivers.
end note

@enduml
```

### Patrones de Diseño
- **Reactor Pattern**: El código C notifica eventos al bus de mensajes; Lua reacciona mediante callbacks no bloqueantes.
- **Zero-Copy IPC**: Transferencia de datos entre procesos usando punteros de buffers circulares.
- **FMEA (Failure Mode and Effects Analysis)**: Drivers implementan recuperación automática ante fallos de hardware.

### Memoria y Asincronía
- Heap de Lua en PSRAM para eficiencia.
- Dispatcher central para coroutines, evitando múltiples tareas FreeRTOS por script.