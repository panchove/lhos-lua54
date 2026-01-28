# lhOS — Directrices y Checklist Rápida

Resumen conciso de las reglas, convenciones y comandos obligatorios para el desarrollo de lhOS.

## Objetivo principal
- Desarrollar el microkernel híbrido lhOS para ESP32-S3 usando ESP-IDF 5.5.2, FreeRTOS y Lua 5.4 como runtime asíncrono.

## Stack obligatorio
- Hardware: ESP32-S3 (Xtensa LX7 Dual Core).
- Framework: ESP-IDF v5.5.2.
- Lenguaje: C17 (no C++). No añadir archivos .cpp/.cc/.cxx.
- Runtime: Lua 5.4 (corrutinas). Tests con Unity.

## Arquitectura y comportamiento
- Dual‑core: Core 0 -> protocolos críticos y drivers; Core 1 -> VM de Lua y lógica asíncrona.
- No crear una `vTask` por script. Usar un Dispatcher único que ejecute corrutinas Lua dentro de una sola tarea del sistema.
- Priorizar PSRAM para el heap de Lua (`pvPortMallocCaps` con `MALLOC_CAP_SPIRAM`).

## Convenciones de código
- Macros: prefijo `LHOS_` (ej. `LHOS_TICK_RATE_MS`).
- Tipos: prefijo `lhos_` (ej. `lhos_task_handle_t`).
- Bindings C→Lua: prefijo `lhos_lua_`.
- Validar siempre la pila de Lua (`lua_gettop`, `luaL_checktype`) antes de operar con hardware.
- Evitar `#pragma once`; usar include guards portables (`#ifndef / #define / #endif`).

## Patrones y requisitos de diseño
- Usar Reactor Pattern: C notifica eventos al bus; Lua reacciona con callbacks no bloqueantes.
- Zero‑Copy IPC: transferir punteros a buffers circulares en lugar de duplicar datos.
- FMEA: drivers deben implementar recuperación automática ante fallos de hardware.

## Reglas al generar código
- En C, añadir `IRAM_ATTR` a funciones en caminos críticos de interrupción.
- En Lua, usar `lhos.yield()` o APIs asíncronas para no bloquear el Event Loop.
- Tests unitarios con Unity (ESP-IDF).

## Comandos / Flujo en Windows (PowerShell)
Antes de ejecutar `idf.py` ejecutar `export.ps1` para configurar el entorno.

```powershell
# Preparar entorno (PowerShell)
# Ejecutar el script de entorno (PowerShell)
export.ps1

# Compilar
idf.py build

# Flashear y monitor
idf.py -p COMx flash monitor

# Ejecutar tests unitarios (script del repo)
.\Run-Tests.ps1 -TestType unit
```

## Notas rápidas
- El código nuevo debe compilar como C (no C++). Evitar dependencias que requieran C++.
- Documentar excepciones y decisiones arquitectónicas en `docs/`.

---
Generado a partir de las instrucciones internas del proyecto. Mantener este archivo actualizado cuando cambien las políticas.
