#include "lhos_lua_uart.h"
#include "lauxlib.h"
#include "lhos_uart.h"
#include "lua.h"

int
lhos_lua_uart_open (lua_State *L)
{
  return lhos_uart_open (L);
}

int
lhos_lua_uart_close (lua_State *L)
{
  return lhos_uart_close (L);
}

int
lhos_lua_uart_read (lua_State *L)
{
  return lhos_uart_read (L);
}

int
lhos_lua_uart_write (lua_State *L)
{
  return lhos_uart_write (L);
}

int
lhos_lua_uart_available (lua_State *L)
{
  return lhos_uart_available (L);
}

void
lhos_lua_uart_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_uart_open);
  lua_setfield (L, -2, "open");
  lua_pushcfunction (L, lhos_lua_uart_close);
  lua_setfield (L, -2, "close");
  lua_pushcfunction (L, lhos_lua_uart_read);
  lua_setfield (L, -2, "read");
  lua_pushcfunction (L, lhos_lua_uart_write);
  lua_setfield (L, -2, "write");
  lua_pushcfunction (L, lhos_lua_uart_available);
  lua_setfield (L, -2, "available");
  /* set as global `uart` */
  lua_setglobal (L, "uart");
}
