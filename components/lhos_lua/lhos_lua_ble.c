
/* Minimal in-tree BLE Lua binding stub.
 * Returns (false, ESP_ERR_NOT_SUPPORTED) for operations until
 * a real BLE implementation is added back into the tree.
 */

#include "esp_err.h"
#include "lauxlib.h"
#include "lhos_ble.h"
#include "lua.h"

int
lhos_lua_ble_init (lua_State *L)
{
  return lhos_ble_init (L);
}

int
lhos_lua_ble_scan (lua_State *L)
{
  return lhos_ble_scan (L);
}

int
lhos_lua_ble_connect (lua_State *L)
{
  return lhos_ble_connect (L);
}

int
lhos_lua_ble_disconnect (lua_State *L)
{
  return lhos_ble_disconnect (L);
}

int
lhos_lua_ble_notify (lua_State *L)
{
  return lhos_ble_notify (L);
}

int
lhos_lua_ble_read (lua_State *L)
{
  return lhos_ble_read (L);
}

int
lhos_lua_ble_write (lua_State *L)
{
  return lhos_ble_write (L);
}

void
lhos_lua_ble_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_ble_init);
  lua_setfield (L, -2, "init");
  lua_pushcfunction (L, lhos_lua_ble_scan);
  lua_setfield (L, -2, "scan");
  lua_pushcfunction (L, lhos_lua_ble_connect);
  lua_setfield (L, -2, "connect");
  lua_pushcfunction (L, lhos_lua_ble_disconnect);
  lua_setfield (L, -2, "disconnect");
  lua_pushcfunction (L, lhos_lua_ble_notify);
  lua_setfield (L, -2, "notify");
  lua_pushcfunction (L, lhos_lua_ble_read);
  lua_setfield (L, -2, "read");
  lua_pushcfunction (L, lhos_lua_ble_write);
  lua_setfield (L, -2, "write");
  lua_setfield (L, -2, "ble");
}
