#ifndef LHOS_LUA_BLE_H
#define LHOS_LUA_BLE_H

#include "lua.h"

void lhos_lua_ble_register (lua_State *L);
int lhos_lua_ble_init (lua_State *L);
int lhos_lua_ble_scan (lua_State *L);
int lhos_lua_ble_connect (lua_State *L);
int lhos_lua_ble_disconnect (lua_State *L);
int lhos_lua_ble_notify (lua_State *L);
int lhos_lua_ble_read (lua_State *L);
int lhos_lua_ble_write (lua_State *L);

#endif // LHOS_LUA_BLE_H
