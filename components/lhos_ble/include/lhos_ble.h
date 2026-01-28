#ifndef LHOS_BLE_H
#define LHOS_BLE_H

#include "esp_err.h"
#include "lua.h"

int lhos_ble_init (lua_State *L);
int lhos_ble_scan (lua_State *L);
int lhos_ble_connect (lua_State *L);
int lhos_ble_disconnect (lua_State *L);
int lhos_ble_read (lua_State *L);
int lhos_ble_write (lua_State *L);
int lhos_ble_notify (lua_State *L);
int lhos_ble_poll (lua_State *L);

#endif // LHOS_BLE_H
