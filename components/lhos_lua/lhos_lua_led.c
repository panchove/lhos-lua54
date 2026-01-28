#include "lhos_lua_led.h"
#include "lauxlib.h"
#include "lhos_led.h"
#include "lua.h"

int
lhos_lua_led_init (lua_State *L)
{
  esp_err_t r = lhos_led_init ();
  lua_pushinteger (L, (int)r);
  return 1;
}

int
lhos_lua_led_set (lua_State *L)
{
  int color = luaL_checkinteger (L, 1);
  esp_err_t r = lhos_led_set_color ((lhos_led_color_t)color);
  lua_pushinteger (L, (int)r);
  return 1;
}

int
lhos_lua_led_off (lua_State *L)
{
  esp_err_t r = lhos_led_off ();
  lua_pushinteger (L, (int)r);
  return 1;
}

void
lhos_lua_led_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_led_init);
  lua_setfield (L, -2, "init");
  lua_pushcfunction (L, lhos_lua_led_set);
  lua_setfield (L, -2, "set");
  lua_pushcfunction (L, lhos_lua_led_off);
  lua_setfield (L, -2, "off");
  lua_setfield (L, -2, "led");
}
