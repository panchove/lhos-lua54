/*
 * Minimal `lhos_lua.c` — Lua VM skeleton and module registration.
 * Implements public API expected by other components:
 *   - lhos_lua_init()
 *   - lhos_lua_run_script(const char *)
 *   - lhos_lua_enabled()
 *   - lhos_lua_scheduler_run()
 *
 * This file intentionally uses forward declarations for optional
 * module registration functions to avoid depending on header files
 * that may be added/modified separately.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lhos_lua.h"
#include <stdlib.h>

#include "lhos_lua.h"

static const char *TAG = "LHOS_LUA";
static lua_State *g_L = NULL;
static QueueHandle_t lhos_event_queue = NULL;
static int lhos_ble_notify_ref = LUA_NOREF;
static int lhos_net_notify_ref = LUA_NOREF;

#include "lhos_lua.h"
#define LHOS_EVENT_MAX 256

#include "esp_system.h"

enum lhos_event_type
{
  LHOS_EVENT_TYPE_BLE = 1,
  LHOS_EVENT_TYPE_NET = 2,
};

struct lhos_event_item
{
  uint8_t type;
  uint8_t conn_id; /* for network events */
  uint16_t len;
  bool owned; /* true if `ptr` owns the buffer */
  void *ptr;  /* when owned==true, pointer to malloc'd buffer (len prefixed) */
  uint8_t data[LHOS_EVENT_MAX];
};

/* Forward declarations for optional module registration functions. */
void lhos_lua_net_register (lua_State *L);
void lhos_lua_ntp_register (lua_State *L);
void lhos_lua_post_register (lua_State *L);
void lhos_lua_ble_register (lua_State *L);
void lhos_lua_wifi_register (lua_State *L);
void lhos_lua_uart_register (lua_State *L);
void lhos_lua_posix_register (lua_State *L);

/* System helpers exposed to Lua */
int
lhos_lua_system_free_heap (lua_State *L)
{
  uint32_t sz = esp_get_free_heap_size ();
  lua_pushinteger (g_L, (lua_Integer)sz);
  return 1;
}
void lhos_lua_led_register (lua_State *L);

/* Provide no-op register functions so the core can call them even when
  the individual binding components were removed. These will be
  overridden if real registration functions are linked in. */
void
lhos_lua_net_register (lua_State *L)
{
  (void)L;
}
void
lhos_lua_ntp_register (lua_State *L)
{
  (void)L;
}
void
lhos_lua_post_register (lua_State *L)
{
  (void)L;
}
void
lhos_lua_ble_register (lua_State *L)
{
  (void)L;
}
void
lhos_lua_wifi_register (lua_State *L)
{
  (void)L;
}
void
lhos_lua_uart_register (lua_State *L)
{
  (void)L;
}
void
lhos_lua_led_register (lua_State *L)
{
  (void)L;
}

static int
lhos_lua_print (lua_State *L)
{
  int n = lua_gettop (L);
  lua_getglobal (L, "tostring");
  char buf[256];
  size_t pos = 0;
  for (int i = 1; i <= n; i++)
    {
      lua_pushvalue (L, -1);
      lua_pushvalue (L, i);
      lua_call (L, 1, 1);
      const char *s = lua_tostring (L, -1);
      if (s)
        {
          int written = snprintf (buf + pos, sizeof (buf) - pos, "%s%s",
                                  (i > 1 ? "\t" : ""), s);
          if (written > 0)
            pos += written;
        }
      lua_pop (L, 1);
      if (pos >= sizeof (buf) - 1)
        break;
    }
  buf[pos] = '\0';
  ESP_LOGI (TAG, "%s", buf);
  return 0;
}

bool
lhos_lua_enabled (void)
{
  return (g_L != NULL);
}

void
lhos_lua_init (void)
{
  if (g_L)
    return;

  ESP_LOGI (TAG, "Initializing Lua VM");
  g_L = luaL_newstate ();
  if (!g_L)
    {
      ESP_LOGE (TAG, "Failed to create Lua state");
      return;
    }

  luaL_openlibs (g_L);

  /* Replace global print with ESP log-backed print */
  lua_pushcfunction (g_L, lhos_lua_print);
  lua_setglobal (g_L, "print");

  /* Register optional modules (implementations may be no-op).
     Forward-declared functions allow this file to compile regardless
     of whether the individual module headers exist. */
  lhos_lua_net_register (g_L);
  lhos_lua_ntp_register (g_L);
  lhos_lua_post_register (g_L);
  lhos_lua_ble_register (g_L);
  lhos_lua_wifi_register (g_L);
  lhos_lua_uart_register (g_L);
  lhos_lua_posix_register (g_L);
  lhos_lua_led_register (g_L);

  /* system helper */
  lua_newtable (g_L);
  lua_pushcfunction (g_L, lhos_lua_system_free_heap);
  lua_setfield (g_L, -2, "free_heap");
  lua_setglobal (g_L, "system");

  ESP_LOGI (TAG, "Lua VM initialized");

  if (!lhos_event_queue)
    {
      lhos_event_queue = xQueueCreate (LHOS_EVENT_QUEUE_LEN,
                                       sizeof (struct lhos_event_item));
    }
}

void
lhos_lua_enqueue_notification (const uint8_t *data, size_t len)
{
  if (!lhos_event_queue || !data)
    return;
  struct lhos_event_item it;
  if (len > LHOS_EVENT_MAX)
    len = LHOS_EVENT_MAX;
  it.type = LHOS_EVENT_TYPE_BLE;
  it.conn_id = 0;
  it.len = (uint16_t)len;
  memcpy (it.data, data, it.len);
  xQueueSend (lhos_event_queue, &it, 0);
}

void
lhos_lua_enqueue_net_event (int conn_id, const uint8_t *data, size_t len)
{
  if (!lhos_event_queue || !data)
    return;
  struct lhos_event_item it;
  if (len > LHOS_EVENT_MAX)
    len = LHOS_EVENT_MAX;
  it.type = LHOS_EVENT_TYPE_NET;
  it.conn_id = (uint8_t)conn_id;
  it.len = (uint16_t)len;
  it.owned = false;
  it.ptr = NULL;
  memcpy (it.data, data, it.len);
  xQueueSend (lhos_event_queue, &it, 0);
}

void
lhos_lua_enqueue_net_event_owned (int conn_id, void *buf, size_t len)
{
  if (!lhos_event_queue || !buf)
    return;
  struct lhos_event_item it;
  if (len > 0 && len > UINT16_MAX)
    len = UINT16_MAX;
  it.type = LHOS_EVENT_TYPE_NET;
  it.conn_id = (uint8_t)conn_id;
  it.len = (uint16_t)len;
  it.owned = true;
  it.ptr = buf;
  xQueueSend (lhos_event_queue, &it, 0);
}

int
lhos_lua_register_ble_callback (lua_State *L)
{
  if (!g_L)
    return luaL_error (L, "Lua VM not initialized");
  if (!lua_isfunction (L, 1))
    return luaL_error (L, "expected function");
  /* store reference in registry attached to g_L */
  lua_pushvalue (L, 1);
  if (lhos_ble_notify_ref != LUA_NOREF)
    luaL_unref (L, LUA_REGISTRYINDEX, lhos_ble_notify_ref);
  lhos_ble_notify_ref = luaL_ref (L, LUA_REGISTRYINDEX);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_register_net_callback (lua_State *L)
{
  if (!g_L)
    return luaL_error (L, "Lua VM not initialized");
  if (!lua_isfunction (L, 1))
    return luaL_error (L, "expected function");
  lua_pushvalue (L, 1);
  if (lhos_net_notify_ref != LUA_NOREF)
    luaL_unref (L, LUA_REGISTRYINDEX, lhos_net_notify_ref);
  lhos_net_notify_ref = luaL_ref (L, LUA_REGISTRYINDEX);
  lua_pushboolean (L, 1);
  return 1;
}

int
lhos_lua_net_callback_registered (void)
{
  return (lhos_net_notify_ref != LUA_NOREF) ? 1 : 0;
}

int
lhos_lua_run_script (const char *script)
{
  if (!script)
    return LUA_ERRSYNTAX;

  if (!g_L)
    lhos_lua_init ();

  if (!g_L)
    return -1;

  int rc = luaL_loadstring (g_L, script);
  if (rc != LUA_OK)
    {
      ESP_LOGE (TAG, "luaL_loadstring error: %s", lua_tostring (g_L, -1));
      lua_pop (g_L, 1);
      return rc;
    }

  rc = lua_pcall (g_L, 0, LUA_MULTRET, 0);
  if (rc != LUA_OK)
    {
      ESP_LOGE (TAG, "lua_pcall error: %s", lua_tostring (g_L, -1));
      lua_pop (g_L, 1);
    }
  return rc;
}

void
lhos_lua_scheduler_run (void)
{
  /* Process pending notifications, then yield. */
  if (g_L && lhos_event_queue)
    {
      struct lhos_event_item it;
      while (xQueueReceive (lhos_event_queue, &it, 0) == pdTRUE)
        {
          if (it.type == LHOS_EVENT_TYPE_BLE)
            {
              if (lhos_ble_notify_ref != LUA_NOREF)
                {
                  lua_rawgeti (g_L, LUA_REGISTRYINDEX, lhos_ble_notify_ref);
                  lua_pushlstring (g_L, (const char *)it.data, it.len);
                  if (lua_pcall (g_L, 1, 0, 0) != LUA_OK)
                    {
                      ESP_LOGW (TAG, "Lua BLE callback error: %s",
                                lua_tostring (g_L, -1));
                      lua_pop (g_L, 1);
                    }
                }
            }
          else if (it.type == LHOS_EVENT_TYPE_NET)
            {
              if (lhos_net_notify_ref != LUA_NOREF)
                {
                  lua_rawgeti (g_L, LUA_REGISTRYINDEX, lhos_net_notify_ref);
                  lua_pushinteger (g_L, it.conn_id);
                  if (it.owned && it.ptr)
                    {
                      /* owned buffer format: [size_t len][data bytes] */
                      size_t buflen = *((size_t *)it.ptr);
                      char *buf = (char *)it.ptr + sizeof (size_t);
                      lua_pushlstring (g_L, buf, buflen);
                      if (lua_pcall (g_L, 2, 0, 0) != LUA_OK)
                        {
                          ESP_LOGW (TAG, "Lua NET callback error: %s",
                                    lua_tostring (g_L, -1));
                          lua_pop (g_L, 1);
                        }
                      free (it.ptr);
                    }
                  else
                    {
                      lua_pushlstring (g_L, (const char *)it.data, it.len);
                      if (lua_pcall (g_L, 2, 0, 0) != LUA_OK)
                        {
                          ESP_LOGW (TAG, "Lua NET callback error: %s",
                                    lua_tostring (g_L, -1));
                          lua_pop (g_L, 1);
                        }
                    }
                }
            }
        }
    }
  vTaskDelay (pdMS_TO_TICKS (1));
}
