#ifndef LHOS_LUA_UART_H
#define LHOS_LUA_UART_H

#include "lua.h"

int lhos_lua_uart_open (lua_State *L);
int lhos_lua_uart_close (lua_State *L);
int lhos_lua_uart_read (lua_State *L);
int lhos_lua_uart_write (lua_State *L);
void lhos_lua_uart_register (lua_State *L);

#endif // LHOS_LUA_UART_H
