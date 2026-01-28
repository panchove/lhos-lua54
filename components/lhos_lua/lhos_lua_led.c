/* Lua binding for LED (WS2812B) using lhos_ws2812b component */
#include "lhos_lua_led.h"
#include "esp_err.h"
#include "lauxlib.h"
#include "lhos_ws2812b.h"
#include "lua.h"

static int
push_error (lua_State *L, const char *msg)
{
  lua_pushnil (L);
  lua_pushstring (L, msg);
  return 2;
}

int
lhos_lua_led_set (lua_State *L)
{
  int r = (int)luaL_checkinteger (L, 1);
  int g = (int)luaL_checkinteger (L, 2);
  int b = (int)luaL_checkinteger (L, 3);
  esp_err_t err = lhos_ws2812b_set_color ((uint8_t)r, (uint8_t)g, (uint8_t)b);
  if (err != ESP_OK)
    return push_error (L, "failed to set color");
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_off (lua_State *L)
{
  esp_err_t err = lhos_ws2812b_off ();
  if (err != ESP_OK)
    return push_error (L, "failed to turn off LED");
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_config (lua_State *L)
{
  (void)L;
  esp_err_t err = lhos_ws2812b_init ();
  if (err != ESP_OK)
    return push_error (L, "failed to init ws2812b");
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_get (lua_State *L)
{
  (void)L;
  /* Not supported by underlying driver */
  return push_error (L, "get not supported");
}

int
lhos_lua_led_set_enum (lua_State *L)
{
  int idx = (int)luaL_checkinteger (L, 1);
  esp_err_t err = lhos_ws2812b_set_color_enum (idx);
  if (err != ESP_OK)
    return push_error (L, "failed to set enum color");
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_led_flash (lua_State *L)
{
  int flashes = (int)luaL_checkinteger (L, 1);
  uint32_t on_ms = (uint32_t)luaL_checkinteger (L, 2);
  uint32_t off_ms = (uint32_t)luaL_checkinteger (L, 3);
  esp_err_t err = lhos_ws2812b_flash ((uint8_t)flashes, on_ms, off_ms);
  if (err != ESP_OK)
    return push_error (L, "failed to flash led");
  lua_pushboolean (L, 1);
  return 1;
}

void
lhos_lua_led_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_led_set);
  lua_setfield (L, -2, "set");
  lua_pushcfunction (L, lhos_lua_led_off);
  lua_setfield (L, -2, "off");
  lua_pushcfunction (L, lhos_lua_led_config);
  lua_setfield (L, -2, "config");
  lua_pushcfunction (L, lhos_lua_led_get);
  lua_setfield (L, -2, "get");
  lua_pushcfunction (L, lhos_lua_led_set_enum);
  lua_setfield (L, -2, "set_enum");
  lua_pushcfunction (L, lhos_lua_led_flash);
  lua_setfield (L, -2, "flash");
  lua_setglobal (L, "led");
}
#include "lauxlib.h"
#include "lhos_led.h"
#include "lhos_lua_led.h"
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
