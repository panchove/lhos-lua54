#include "lhos_lua_wifi.h"
#include "lauxlib.h"
#include "lhos_wifi.h"
#include "lua.h"

int
lhos_lua_wifi_init (lua_State *L)
{
  esp_err_t r = lhos_wifi_init ();
  lua_pushinteger (L, (int)r);
  return 1;
}

int
lhos_lua_wifi_connect (lua_State *L)
{
  const char *ssid = luaL_checkstring (L, 1);
  const char *pwd = luaL_optstring (L, 2, "");
  esp_err_t r = lhos_wifi_connect (ssid, pwd);
  lua_pushinteger (L, (int)r);
  return 1;
}

int
lhos_lua_wifi_disconnect (lua_State *L)
{
  esp_err_t r = lhos_wifi_disconnect ();
  lua_pushinteger (L, (int)r);
  return 1;
}

int
lhos_lua_wifi_get_status (lua_State *L)
{
  bool connected = lhos_wifi_is_connected ();
  lua_pushboolean (L, connected);
  return 1;
}

void
lhos_lua_wifi_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_wifi_init);
  lua_setfield (L, -2, "init");
  lua_pushcfunction (L, lhos_lua_wifi_connect);
  lua_setfield (L, -2, "connect");
  lua_pushcfunction (L, lhos_lua_wifi_disconnect);
  lua_setfield (L, -2, "disconnect");
  lua_pushcfunction (L, lhos_lua_wifi_get_status);
  lua_setfield (L, -2, "get_status");
  lua_setfield (L, -2, "wifi");
}
