/* Lightweight NTP helper for LHOS
 * Provides simple Lua-facing wrappers to start SNTP and query status.
 */

#include "lhos_ntp.h"
#include "esp_log.h"
#include "lauxlib.h"
#include "lua.h"

#include "lhos_lua.h"
#include "lwip/apps/sntp.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *TAG = "lhos_ntp";

struct ntp_notify_args
{
  int wait_ms;
};

static void
ntp_notify_task (void *pv)
{
  struct ntp_notify_args *a = (struct ntp_notify_args *)pv;
  TickType_t start = xTaskGetTickCount ();
  while (1)
    {
      time_t now = time (NULL);
      if (now != ((time_t)-1) && now > 1600000000)
        {
          char msg[64];
          snprintf (msg, sizeof (msg), "ntp_synced:%ld", (long)now);
          lhos_lua_enqueue_notification ((const uint8_t *)msg, strlen (msg));
          break;
        }
      TickType_t nowt = xTaskGetTickCount ();
      if ((nowt - start) >= pdMS_TO_TICKS (a->wait_ms))
        break;
      vTaskDelay (pdMS_TO_TICKS (1000));
    }
  free (a);
  vTaskDelete (NULL);
}

int
lhos_ntp_sync (lua_State *L)
{
  const char *server = NULL;
  bool notify = false;
  /* parse args: (server_string?, notify_bool?) */
  if (lua_isstring (L, 1))
    server = lua_tostring (L, 1);
  if (lua_isboolean (L, 2))
    notify = lua_toboolean (L, 2);
  else if (lua_isboolean (L, 1) && !server)
    notify = lua_toboolean (L, 1);

  ESP_LOGI (TAG, "Starting SNTP sync (server=%s, notify=%d)",
            server ? server : "pool.ntp.org", (int)notify);

  /* Stop any previous SNTP instance to avoid duplicates */
  sntp_stop ();
  sntp_setoperatingmode (SNTP_OPMODE_POLL);
  if (server)
    sntp_setservername (0, server);
  else
    sntp_setservername (0, "pool.ntp.org");
  sntp_init ();

  if (notify)
    {
      struct ntp_notify_args *args = malloc (sizeof (*args));
      if (args)
        {
          args->wait_ms = 60000; /* 60s timeout */
          xTaskCreatePinnedToCore (ntp_notify_task, "lhos_ntp_notify", 2048,
                                   args, tskIDLE_PRIORITY + 1, NULL, 1);
        }
    }

  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_ntp_status (lua_State *L)
{
  time_t now = time (NULL);
  if (now != ((time_t)-1) && now > 1600000000)
    lua_pushinteger (L, 1);
  else
    lua_pushinteger (L, 0);
  return 1;
}

int
lhos_ntp_get_time (lua_State *L)
{
  time_t t = time (NULL);
  if (t == ((time_t)-1))
    {
      lua_pushnil (L);
      lua_pushstring (L, "time not available");
      return 2;
    }
  lua_pushinteger (L, (lua_Integer)t);
  return 1;
}

int
lhos_ntp_set_server (lua_State *L)
{
  const char *server = luaL_checkstring (L, 1);
  if (!server)
    {
      lua_pushboolean (L, 0);
      lua_pushstring (L, "invalid server");
      return 2;
    }
  sntp_setservername (0, server);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_ntp_stop (lua_State *L)
{
  sntp_stop ();
  lua_pushboolean (L, 1);
  return 1;
}
