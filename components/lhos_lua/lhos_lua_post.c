#include "lhos_lua_post.h"
#include "lauxlib.h"
#include "lhos_post.h"
#include "lua.h"

int
lhos_lua_post_send (lua_State *L)
{
  lhos_post_status_t status;
  esp_err_t err = lhos_post_get_status (&status);
  if (err != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushinteger (L, (int)err);
      return 2;
    }

  lua_newtable (L);
  lua_pushboolean (L, status.ram_ok);
  lua_setfield (L, -2, "ram_ok");
  lua_pushboolean (L, status.flash_ok);
  lua_setfield (L, -2, "flash_ok");
  lua_pushboolean (L, status.gpio_ok);
  lua_setfield (L, -2, "gpio_ok");
  lua_pushboolean (L, status.psram_ok);
  lua_setfield (L, -2, "psram_ok");
  lua_pushboolean (L, status.nvs_ok);
  lua_setfield (L, -2, "nvs_ok");
  lua_pushboolean (L, status.wifi_ok);
  lua_setfield (L, -2, "wifi_ok");
  lua_pushboolean (L, status.ble_ok);
  lua_setfield (L, -2, "ble_ok");

  return 1;
}

int
lhos_lua_post_receive (lua_State *L)
{
  return lhos_lua_post_send (L);
}

void
lhos_lua_post_register (lua_State *L)
{
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_post_send);
  lua_setfield (L, -2, "send");
  lua_pushcfunction (L, lhos_lua_post_receive);
  lua_setfield (L, -2, "receive");
  lua_setfield (L, -2, "post");
}
