#ifndef LHOS_LUA_WIFI_H
#define LHOS_LUA_WIFI_H

#include "lua.h"

int lhos_lua_wifi_init (lua_State *L);
int lhos_lua_wifi_connect (lua_State *L);
int lhos_lua_wifi_disconnect (lua_State *L);
int lhos_lua_wifi_get_status (lua_State *L);
void lhos_lua_wifi_register (lua_State *L);

#endif // LHOS_LUA_WIFI_H
