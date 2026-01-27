-- post_soft.lua: Script de comprobación de estado de hardware (POST software)
-- Se ejecuta inmediatamente cuando el monitor está activo

print("Iniciando POST software...")

-- Comprobar estado de UARTs
print("Comprobando UARTs...")
if lhos.uart then
  -- Intentar abrir UART1
  local ok1, err1 = lhos.uart.open(1, 9600, 39, 40)
  if ok1 then
    print("UART1 (RS232_1) OK")
    -- Cerrar para liberar
    lhos.uart.close(1)
  else
    print("UART1 (RS232_1) ERROR: " .. (err1 or "desconocido"))
  end

  -- Intentar abrir UART2
  local ok2, err2 = lhos.uart.open(2, 115200, 41, 42)
  if ok2 then
    print("UART2 (RS232_2) OK")
    -- Cerrar para liberar
    lhos.uart.close(2)
  else
    print("UART2 (RS232_2) ERROR: " .. (err2 or "desconocido"))
  end
else
  print("Módulo lhos.uart no disponible")
end

-- Comprobar ADC
print("Comprobando ADC...")
if lhos.adc then
  local val, err = lhos.adc.read(0)  -- Canal 0 (GPIO 1)
  if val then
    print("ADC canal 0 OK, valor: " .. val)
  else
    print("ADC canal 0 ERROR: " .. (err or "desconocido"))
  end
else
  print("Módulo lhos.adc no disponible")
end

-- Comprobar LEDs (si implementado)
print("Comprobando LEDs...")
if lhos.led then
  -- Intentar encender LED
  local ok, err = lhos.led.set(0, 255, 0, 0)  -- Rojo
  if ok then
    print("LED OK")
    -- Apagar
    lhos.led.set(0, 0, 0, 0)
  else
    print("LED ERROR: " .. (err or "desconocido"))
  end
else
  print("Módulo lhos.led no disponible")
end

-- Comprobar WiFi (si disponible)
print("Comprobando WiFi...")
if lhos.wifi then
  local status, err = lhos.wifi.get_status()
  if status then
    print("WiFi status: " .. status)
  else
    print("WiFi ERROR: " .. (err or "desconocido"))
  end
else
  print("Módulo lhos.wifi no disponible")
end

print("POST software completado.")