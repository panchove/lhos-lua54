# lhOS — ESP32-S3 Lua runtime (placeholder)

Resumen rápido:

- Plataforma: ESP32-S3 (ESP-IDF 5.5.2)
- Objetivo: Microkernel lhOS con runtime Lua 5.4 (estructura inicial)

## Novedades (v0.1.0)

- Añadido componente `lhos_post` y binding Lua `post` para obtener el estado POST desde scripts.
- Nuevos bindings: `posix` (operaciones de fichero) y `led` (control WS2812b) expuestos a Lua.
- Limpieza: `components/lua54` ahora es manejado localmente/ignorado para desarrollos locales.

Comandos útiles (PowerShell):

```powershell
# Configurar entorno ESP-IDF (abrir la shell provista por ESP-IDF)
# Compilar
idf.py build

# Flashear (reemplazar COMx por puerto)
idf.py -p COMx flash monitor

# Tests unitarios (ver Run-Tests.ps1)
.\Run-Tests.ps1 -TestType unit
```

Convenciones (resumen):

- Macros: `LHOS_` prefix
- C → Lua bindings: `lhos_lua_` prefix
- Use PSRAM para heap de Lua con `MALLOC_CAP_SPIRAM`

Estructura creada por el scaffold:

- `main/main.c` — aplicación stub
- `components/lhos/` — componente placeholder (lhos.c/h)
- `tests/unity/` — ejemplo de test unitario
- `Run-Tests.ps1` — helper PowerShell para tests

Integración Lua (pasos iniciales):

- Añadir Lua 5.4 como componente en `components/lua` — incluir fuentes de Lua o usar un paquete precompilado.
- Asegurarse de exportar las cabeceras para que `components/lhos` pueda incluir `lhos_lua.h`.
- Habilitar en configuración con `sdkconfig.defaults` (`CONFIG_LUA_ENABLED=y`).
- Implementar bindings C→Lua en `components/lhos/lhos_lua.c` y exponer `lhos_lua_*`.

Ejemplo rápido para añadir el componente Lua como submódulo:

```powershell
# desde la raíz del repo
git submodule add <repo-con-lua-5.4> components/lua
# o copiar las fuentes de Lua a components/lua
```

