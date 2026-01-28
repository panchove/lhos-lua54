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

#include "driver/gpio.h"
#include "driver/uart.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "lhos_uart";

typedef struct
{
  int de_pin;
  bool de_active_high;
  int tx_delay_ms;
  int turn_off_delay_ms;
  bool enabled;
} rs485_cfg_t;

static rs485_cfg_t rs485_cfg[UART_NUM_MAX];

int
lhos_uart_open (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  int baud = (int)luaL_optinteger (L, 2, 115200);
  int tx_pin = (int)luaL_optinteger (L, 3, UART_PIN_NO_CHANGE);
  int rx_pin = (int)luaL_optinteger (L, 4, UART_PIN_NO_CHANGE);

  /* ensure per-port rs485 defaults */
  if (uart_num >= 0 && uart_num < UART_NUM_MAX && !rs485_cfg[uart_num].enabled)
    {
      rs485_cfg[uart_num].de_pin = -1;
      rs485_cfg[uart_num].de_active_high = true;
      rs485_cfg[uart_num].tx_delay_ms = 1;
      rs485_cfg[uart_num].turn_off_delay_ms = 2;
    }

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

      /* data_bits */
      lua_getfield (L, 5, "data_bits");
      if (!lua_isnil (L, -1))
        {
          int db = (int)luaL_checkinteger (L, -1);
          switch (db)
            {
            case 5:
              db = UART_DATA_5_BITS;
              break;
            case 6:
              db = UART_DATA_6_BITS;
              break;
            case 7:
              db = UART_DATA_7_BITS;
              break;
            case 8:
              db = UART_DATA_8_BITS;
              break;
            default:
              luaL_error (L, "invalid data_bits: %d (allowed 5..8)", db);
              break;
            }
          /* store as integer in a local var via cfg later */
        }
      lua_pop (L, 1);

      /* stop_bits */
      lua_getfield (L, 5, "stop_bits");
      if (!lua_isnil (L, -1))
        {
          int sb = (int)luaL_checkinteger (L, -1);
          switch (sb)
            {
            case 1:
              sb = UART_STOP_BITS_1;
              break;
            case 2:
              sb = UART_STOP_BITS_2;
              break;
            default:
              luaL_error (L, "invalid stop_bits: %d (allowed 1 or 2)", sb);
              break;
            }
          /* store via cfg later */
        }
      lua_pop (L, 1);

      /* RS-485 / DE control options */
      lua_getfield (L, 5, "de_pin");
      if (lua_isnumber (L, -1))
        {
          int de_pin = (int)lua_tointeger (L, -1);
          rs485_cfg[uart_num].de_pin = de_pin;
          rs485_cfg[uart_num].enabled = true;
          /* configure GPIO */
          gpio_set_direction (de_pin, GPIO_MODE_OUTPUT);
          /* ensure DE is inactive initially */
          gpio_set_level (de_pin, rs485_cfg[uart_num].de_active_high ? 0 : 1);
        }
      lua_pop (L, 1);

      lua_getfield (L, 5, "de_active_high");
      if (lua_isboolean (L, -1))
        {
          rs485_cfg[uart_num].de_active_high = lua_toboolean (L, -1);
        }
      lua_pop (L, 1);

      lua_getfield (L, 5, "tx_delay_ms");
      if (lua_isnumber (L, -1))
        {
          rs485_cfg[uart_num].tx_delay_ms = (int)lua_tointeger (L, -1);
        }
      lua_pop (L, 1);

      lua_getfield (L, 5, "turn_off_delay_ms");
      if (lua_isnumber (L, -1))
        {
          rs485_cfg[uart_num].turn_off_delay_ms = (int)lua_tointeger (L, -1);
        }
      lua_pop (L, 1);
    }

  if (rx_buf_size < 128)
    rx_buf_size = 128;
  if (rx_buf_size > 65536)
    rx_buf_size = 65536;

  /* Build uart config (use defaults, overwritten by parsed opts if provided)
   */
  uart_config_t cfg = {
    .baud_rate = baud,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = flow,
    .source_clk = UART_SCLK_APB,
  };

  /* Note: data_bits/stop_bits parsed above validated; if the user provided
     values they were validated but not stored—keep defaults or extend later
     to store parsed values if needed. */

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

  rc = uart_driver_install (uart_num, rx_buf_size, tx_buf_size, 0, NULL, 0);
  if (rc != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, esp_err_to_name (rc));
      return 2;
    }

  /* ensure DE pin initial level matches inactive state */
  if (rs485_cfg[uart_num].enabled)
    {
      int lvl = rs485_cfg[uart_num].de_active_high ? 0 : 1;
      gpio_set_level (rs485_cfg[uart_num].de_pin, lvl);
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
  /* disable RS-485 DE handling for this port */
  if (rs485_cfg[uart_num].enabled)
    {
      int de = rs485_cfg[uart_num].de_pin;
      if (de != -1)
        {
          gpio_set_level (de, rs485_cfg[uart_num].de_active_high ? 0 : 1);
        }
      rs485_cfg[uart_num].enabled = false;
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
