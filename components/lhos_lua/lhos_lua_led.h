#ifndef LHOS_LUA_LED_H
#define LHOS_LUA_LED_H

#include "lua.h"

int lhos_lua_led_set (lua_State *L);
int lhos_lua_led_get (lua_State *L);
int lhos_lua_led_config (lua_State *L);
void lhos_lua_led_register (lua_State *L);

#endif // LHOS_LUA_LED_H
