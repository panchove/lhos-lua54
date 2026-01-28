-- boot.lua
-- Ejecutado al arranque: ejecuta POST y luego carga el script de POST "suave"
-- (post_soft.lua) si existe. Mantener este script pequeño y responsable de
-- orquestar el arranque de scripts de usuario.

local ok, status = pcall(function() return post.send() end)
if not ok or status == nil then
  print("POST: no disponible o error al invocar post.send()")
  -- Indicar fallo crítico en rojo: fade rápido y varios destellos
  if led and led.config then pcall(led.config) end
  if led and led.fade_to then pcall(led.fade_to, 200, 0, 0, 400, 20) end
  if led and led.flash then pcall(led.flash, 5, 150, 150) end
else
  print("POST result:")
  for k, v in pairs(status) do
    print("  ", k, tostring(v))
  end
end

-- Mapear estado POST a colores: verde = OK, ámbar = parcial, rojo = fallo
-- Helper: desempaqueta un entero RGB24 (0xRRGGBB) a r,g,b
local function unpack_rgb24(n)
  if type(n) ~= "number" then return nil end
  local r = math.floor(n / 65536) % 256
  local g = math.floor(n / 256) % 256
  local b = n % 256
  return r, g, b
end

-- Obtener color de configuración (acepta RGB24 o tabla {r,g,b})
local function get_color_from_cfg(kind)
  local ok, cfg = pcall(require, "config")
  if not ok or not cfg or not cfg.led then return nil end
  local cols = cfg.led.colors
  if not cols then return nil end
  local v = cols[kind]
  if not v then return nil end
  if type(v) == "number" then
    return unpack_rgb24(v)
  elseif type(v) == "table" then
    return v.r or 0, v.g or 0, v.b or 0
  end
  return nil
end

-- Determina color por estado si no hay configuración explícita
local function map_status_to_color(st)
  if not st then return 200,0,0 end
  local total = 0
  local ok_count = 0
  for k, v in pairs(st) do
    total = total + 1
    if v then ok_count = ok_count + 1 end
  end
  if total == 0 then return 200,0,0 end
  if ok_count == total then
    return 0, 200, 0 -- verde suave
  elseif ok_count == 0 then
    return 200, 0, 0 -- rojo
  else
    return 200, 80, 0 -- ámbar
  end
end

-- Resolver color final: preferir `config.led.colors`, caer a mapeo por defecto
local r,g,b = nil,nil,nil
do
  local cr,cg,cb = get_color_from_cfg("ok") -- default candidate
  if status then
    -- Try to pick from config based on aggregated status
    local total, ok_count = 0, 0
    for _, v in pairs(status) do total = total + 1; if v then ok_count = ok_count + 1 end end
    local kind = nil
    if total == 0 then kind = "fail"
    elseif ok_count == total then kind = "ok"
    elseif ok_count == 0 then kind = "fail"
    else kind = "partial" end
    local cr2, cg2, cb2 = get_color_from_cfg(kind)
    if cr2 then r,g,b = cr2,cg2,cb2 end
  end
  if not r then r,g,b = map_status_to_color(status) end
end

-- Aplicar color al LED (fade) si la API está disponible
if led and led.config then pcall(led.config) end
if led and led.fade_to then
  -- más suave y visible: 800ms con 40 pasos
  pcall(led.fade_to, r, g, b, 800, 40)
elseif led and led.set then
  pcall(led.set, r, g, b)
end

-- Intentar ejecutar post_soft.lua desde la FS montada en /lfs/scripts
local f = io.open("/lfs/scripts/post_soft.lua", "r")
if f then
  f:close()
  print("Ejecutando post_soft.lua desde /lfs/scripts/")
  dofile("/lfs/scripts/post_soft.lua")
else
  print("No se encontró /lfs/scripts/post_soft.lua")
end

-- Aquí podríamos continuar con la carga del resto del sistema (apps, rc, etc.)
return true
