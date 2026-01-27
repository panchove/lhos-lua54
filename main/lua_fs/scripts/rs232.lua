print('Hello from embedded Lua')
-- UART test
if lhos.uart then
  print('Opening UART1')
  lhos.uart.open(1)
  print('Opening UART2')
  lhos.uart.open(2)
  print('UARTs opened')
else
  print('lhos.uart not available')
end
  print("Error abriendo RS232_2: " .. err2)
end

-- Simular uso (esperar un poco)
lhos.yield(2000)  -- Esperar 2 segundos

-- Cerrar puertos
if ok then
  local close_ok, close_err = lhos.uart.close(1)
  if close_ok then
    print("RS232_1 cerrado")
  else
    print("Error cerrando RS232_1: " .. close_err)
  end
end

if ok2 then
  local close_ok2, close_err2 = lhos.uart.close(2)
  if close_ok2 then
    print("RS232_2 cerrado")
  else
    print("Error cerrando RS232_2: " .. close_err2)
  end
end

print("Script RS232 completado")