#include "lhos_ble.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lhos_lua.h"

static const char *TAG = "lhos_ble";

#ifdef CONFIG_BT_NIMBLE_ENABLED
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_gattc.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"

static int g_conn_handle = -1;
#endif

#ifdef CONFIG_BT_NIMBLE_ENABLED
static int
gap_event_cb (struct ble_gap_event *event, void *arg)
{
  switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status == 0)
        {
          g_conn_handle = event->connect.conn_handle;
          ESP_LOGI (TAG, "Connected, conn_handle=%d", g_conn_handle);
        }
      else
        {
          ESP_LOGW (TAG, "Connection failed; status=%d",
                    event->connect.status);
        }
      break;
    case BLE_GAP_EVENT_DISCONNECT:
      ESP_LOGI (TAG, "Disconnected, reason=%d", event->disconnect.reason);
      g_conn_handle = -1;
      break;
    default:
      break;
    }
  return 0;
}
#endif

int
lhos_ble_init (lua_State *L)
{
#ifdef CONFIG_BT_NIMBLE_ENABLED
  ESP_LOGI (TAG, "Initializing NimBLE stack...");
  nimble_port_init ();
  ESP_LOGI (TAG, "nimble_port_init called");
  lua_pushboolean (L, 1);
  return 1;
#else
  ESP_LOGW (TAG, "NimBLE not enabled in sdkconfig; BLE not supported");
  lua_pushboolean (L, 0);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
#endif
}

int
lhos_ble_scan (lua_State *L)
{
#ifdef CONFIG_BT_NIMBLE_ENABLED
  ESP_LOGI (TAG, "Starting NimBLE scan (5s)...");

  static int found_count = 0;

  /* callback invoked by NimBLE for gap events during discovery */
  int scan_cb (struct ble_gap_event * event, void *arg)
  {
    if (event->type == BLE_GAP_EVENT_DISC)
      {
        const uint8_t *addr = event->disc.addr.val;
        ESP_LOGI (TAG, "Found device %02x:%02x:%02x:%02x:%02x:%02x", addr[5],
                  addr[4], addr[3], addr[2], addr[1], addr[0]);
        found_count++;
      }
    return 0;
  }

  found_count = 0;
  int rc = ble_gap_disc (0 /*own addr type*/, 5 /*secs*/, scan_cb, NULL);
  if (rc != 0)
    {
      ESP_LOGW (TAG, "ble_gap_disc failed: %d", rc);
      lua_pushboolean (L, 0);
      lua_pushinteger (L, rc);
      return 2;
    }

  /* return number found so far (scan runs asynchronously) */
  lua_pushinteger (L, found_count);
  return 1;
#else
  ESP_LOGW (TAG, "NimBLE not enabled; scan not supported");
  lua_pushboolean (L, 0);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
#endif
}

int
lhos_ble_connect (lua_State *L)
{
#ifdef CONFIG_BT_NIMBLE_ENABLED
  /* Expect address string as first arg: "AA:BB:CC:DD:EE:FF" */
  const char *addr_str = luaL_checkstring (L, 1);
  unsigned int b[6];
  if (sscanf (addr_str, "%x:%x:%x:%x:%x:%x", &b[5], &b[4], &b[3], &b[2], &b[1],
              &b[0])
      != 6)
    {
      lua_pushboolean (L, 0);
      lua_pushinteger (L, -1);
      return 2;
    }
  struct ble_addr_t peer_addr;
  for (int i = 0; i < 6; i++)
    {
      peer_addr.val[i] = (uint8_t)b[i];
    }

  int rc = ble_gap_connect (BLE_OWN_ADDR_PUBLIC, &peer_addr, 10000 /*ms*/,
                            gap_event_cb, NULL);
  if (rc != 0)
    {
      ESP_LOGW (TAG, "ble_gap_connect failed: %d", rc);
      lua_pushboolean (L, 0);
      lua_pushinteger (L, rc);
      return 2;
    }

  lua_pushboolean (L, 1);
  return 1;
#else
  ESP_LOGW (TAG, "NimBLE not enabled; connect not supported");
  lua_pushboolean (L, 0);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
#endif
}

int
lhos_ble_disconnect (lua_State *L)
{
  ESP_LOGW (TAG, "disconnect not implemented");
  lua_pushboolean (L, 0);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
}

int
lhos_ble_read (lua_State *L)
{
#ifdef CONFIG_BT_NIMBLE_ENABLED
  if (g_conn_handle < 0)
    {
      lua_pushnil (L);
      lua_pushinteger (L, -1);
      return 2;
    }
  uint16_t attr_handle = (uint16_t)luaL_checkinteger (L, 1);
  uint8_t buf[512];
  uint16_t len = sizeof (buf);
  int rc = ble_gattc_read_flat (g_conn_handle, attr_handle, buf, &len);
  if (rc != 0)
    {
      lua_pushnil (L);
      lua_pushinteger (L, rc);
      return 2;
    }
  lua_pushlstring (L, (const char *)buf, len);
  return 1;
#else
  ESP_LOGW (TAG, "read not implemented");
  lua_pushnil (L);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
#endif
}

int
lhos_ble_write (lua_State *L)
{
#ifdef CONFIG_BT_NIMBLE_ENABLED
  if (g_conn_handle < 0)
    {
      lua_pushboolean (L, 0);
      lua_pushinteger (L, -1);
      return 2;
    }
  uint16_t attr_handle = (uint16_t)luaL_checkinteger (L, 1);
  size_t data_len;
  const char *data = luaL_checklstring (L, 2, &data_len);
  int rc = ble_gattc_write_flat (g_conn_handle, attr_handle, (void *)data,
                                 data_len, NULL, NULL);
  if (rc != 0)
    {
      lua_pushboolean (L, 0);
      lua_pushinteger (L, rc);
      return 2;
    }
  lua_pushboolean (L, 1);
  return 1;
#else
  ESP_LOGW (TAG, "write not implemented");
  lua_pushboolean (L, 0);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
#endif
}

int
lhos_ble_notify (lua_State *L)
{
  /* If passed a function, register it in the Lua VM via lhos_lua.
     If passed an integer, subscribe to the given characteristic handle. */
  if (lua_isfunction (L, 1))
    {
      return lhos_lua_register_ble_callback (L);
    }

#ifdef CONFIG_BT_NIMBLE_ENABLED
  if (!lua_isinteger (L, 1))
    {
      lua_pushboolean (L, 0);
      lua_pushinteger (L, -1);
      return 2;
    }
  if (g_conn_handle < 0)
    {
      lua_pushboolean (L, 0);
      lua_pushinteger (L, -1);
      return 2;
    }

  uint16_t char_handle = (uint16_t)lua_tointeger (L, 1);
  struct ble_gattc_subscribe_params params;
  memset (&params, 0, sizeof (params));
  params.ccc_handle = char_handle;

  /* notification callback: forward data into lhos_lua dispatcher */
  int notify_cb (uint16_t conn_handle, const struct ble_gatt_error *error,
                 struct os_mbuf *om, void *arg)
  {
    if (!om)
      return 0;
    int len = OS_MBUF_PKTLEN (om);
    if (len <= 0)
      return 0;
    uint8_t *buf = malloc (len);
    if (!buf)
      return 0;
    int off = 0;
    struct os_mbuf *m = om;
    while (m && off < len)
      {
        int copy = m->om_len;
        if (copy > len - off)
          copy = len - off;
        memcpy (buf + off, m->om_data, copy);
        off += copy;
        m = m->om_next;
      }
    lhos_lua_enqueue_notification (buf, len);
    free (buf);
    return 0;
  }

  int rc = ble_gattc_subscribe (g_conn_handle, &params, notify_cb, NULL);
  if (rc != 0)
    {
      lua_pushboolean (L, 0);
      lua_pushinteger (L, rc);
      return 2;
    }
  lua_pushboolean (L, 1);
  return 1;
#else
  ESP_LOGW (TAG, "NimBLE not enabled; notify subscribe not supported");
  lua_pushboolean (L, 0);
  lua_pushinteger (L, ESP_ERR_NOT_SUPPORTED);
  return 2;
#endif
}

/* lhos_ble no longer exposes a poll API; notifications are forwarded into
   the lhos_lua dispatcher via `lhos_lua_enqueue_notification`. */
