/* Minimal UART helpers for LHOS Lua bindings.
 * Lua wrappers expect arguments on the stack and return Lua values.
 * API (Lua):
 *   uart.open(uart_num, baud, tx_pin, rx_pin) -> true|nil, err
 *   uart.close(uart_num) -> true|nil, err
 *   uart.write(uart_num, data) -> bytes_written | nil, err
 *   uart.read(uart_num, max_bytes=256, timeout_ms=100) -> data | nil, err
 */

#include "lhos_uart.h"
#include "esp_log.h"
#include "lauxlib.h"
#include "lua.h"

#include "driver/uart.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "lhos_uart";

int
lhos_uart_open (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  int baud = (int)luaL_optinteger (L, 2, 115200);
  int tx_pin = (int)luaL_optinteger (L, 3, UART_PIN_NO_CHANGE);
  int rx_pin = (int)luaL_optinteger (L, 4, UART_PIN_NO_CHANGE);
  /* Optional options table at arg 5 */
  int rx_buf_size = 1024;
  int tx_buf_size = 0;
  uart_parity_t parity = UART_PARITY_DISABLE;
  uart_hw_flowcontrol_t flow = UART_HW_FLOWCTRL_DISABLE;
  if (lua_istable (L, 5))
    {
      lua_getfield (L, 5, "parity");
      if (lua_isstring (L, -1))
        {
          const char *p = lua_tostring (L, -1);
          if (strcmp (p, "even") == 0)
            parity = UART_PARITY_EVEN;
          else if (strcmp (p, "odd") == 0)
            parity = UART_PARITY_ODD;
          else
            parity = UART_PARITY_DISABLE;
        }
      lua_pop (L, 1);
      lua_getfield (L, 5, "flow_control");
      if (lua_isboolean (L, -1) && lua_toboolean (L, -1))
        flow = UART_HW_FLOWCTRL_CTS_RTS;
      lua_pop (L, 1);
      lua_getfield (L, 5, "rx_buffer_size");
      if (lua_isnumber (L, -1))
        rx_buf_size = (int)lua_tointeger (L, -1);
      lua_pop (L, 1);
      lua_getfield (L, 5, "tx_buffer_size");
      if (lua_isnumber (L, -1))
        tx_buf_size = (int)lua_tointeger (L, -1);
      lua_pop (L, 1);
    }

  if (rx_buf_size < 128)
    rx_buf_size = 128;
  if (rx_buf_size > 65536)
    rx_buf_size = 65536;

  uart_config_t cfg = {
    .baud_rate = baud,
    .data_bits = UART_DATA_8_BITS,
    .parity = parity,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = flow,
    .source_clk = UART_SCLK_APB,
  };

  esp_err_t rc = uart_param_config (uart_num, &cfg);
  if (rc != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, esp_err_to_name (rc));
      return 2;
    }

  rc = uart_set_pin (uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE,
                     UART_PIN_NO_CHANGE);
  if (rc != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, esp_err_to_name (rc));
      return 2;
    }

  /* install driver with RX/TX buffer sizes (circular buffer provided by
   * driver) */
  rc = uart_driver_install (uart_num, rx_buf_size, tx_buf_size, 0, NULL, 0);
  if (rc != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, esp_err_to_name (rc));
      return 2;
    }

  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_uart_close (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  esp_err_t rc = uart_driver_delete (uart_num);
  if (rc != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, esp_err_to_name (rc));
      return 2;
    }
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_uart_write (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  size_t len = 0;
  const char *data = luaL_checklstring (L, 2, &len);
  if (!data || len == 0)
    {
      lua_pushinteger (L, 0);
      return 1;
    }
  int written = uart_write_bytes (uart_num, data, len);
  if (written < 0)
    {
      lua_pushnil (L);
      lua_pushstring (L, "write_failed");
      return 2;
    }
  lua_pushinteger (L, written);
  return 1;
}

int
lhos_uart_read (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  int max_bytes = (int)luaL_optinteger (L, 2, 256);
  int timeout_ms = (int)luaL_optinteger (L, 3, 100);

  if (max_bytes <= 0)
    max_bytes = 256;
  uint8_t *buf = malloc (max_bytes);
  if (!buf)
    {
      lua_pushnil (L);
      lua_pushstring (L, "oom");
      return 2;
    }
  int r
      = uart_read_bytes (uart_num, buf, max_bytes, pdMS_TO_TICKS (timeout_ms));
  if (r < 0)
    {
      free (buf);
      lua_pushnil (L);
      lua_pushstring (L, "read_failed");
      return 2;
    }
  lua_pushlstring (L, (const char *)buf, r);
  free (buf);
  return 1;
}

int
lhos_uart_available (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  size_t len = 0;
  esp_err_t rc = uart_get_buffered_data_len (uart_num, &len);
  if (rc != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, esp_err_to_name (rc));
      return 2;
    }
  lua_pushinteger (L, (lua_Integer)len);
  return 1;
}
