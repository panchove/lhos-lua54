/* LHOS UART minimal API exposed to Lua
 */
#ifndef LHOS_UART_H
#define LHOS_UART_H

#ifndef LUA_STATE_DECL
typedef struct lua_State lua_State;
#define LUA_STATE_DECL
#endif

int lhos_uart_open (lua_State *L);
int lhos_uart_close (lua_State *L);
int lhos_uart_read (lua_State *L);
int lhos_uart_write (lua_State *L);
int lhos_uart_available (lua_State *L);

#endif /* LHOS_UART_H */
