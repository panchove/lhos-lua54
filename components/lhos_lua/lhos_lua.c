/* Real Lua VM wrapper. Builds when `components/lua54` is available and
   `CONFIG_LUA_ENABLED` is set. */
#include "lhos_lua.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lhos_post.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef CONFIG_LUA_ENABLED
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#endif

static const char *TAG = "LHOS_LUA";

#ifdef CONFIG_LUA_ENABLED
static lua_State *L = NULL;
/* forward decl for scheduler add (defined later) */
void lhos_lua_schedule_add (lua_State *co, uint64_t when_ms);
/* Scheduler node type and head (defined here so yield handler can inspect
 * list) */
typedef struct sched_node_s
{
  lua_State *co;
  uint64_t resume_at_ms;
  struct sched_node_s *next;
} sched_node_t;

static sched_node_t *sched_head;

static void *
l_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
  (void)ud;
  (void)osize;

  if (nsize == 0)
    {
      if (ptr)
        {
#ifdef CONFIG_LUA_USE_SPIRAM
          heap_caps_free (ptr);
#else
          heap_caps_free (ptr);
#endif
        }
      return NULL;
    }

#ifdef CONFIG_LUA_USE_SPIRAM
  const int caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
#else
  const int caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
#endif

  if (ptr == NULL)
    {
      return heap_caps_malloc (nsize, caps);
    }
  else
    {
      void *p = heap_caps_realloc (ptr, nsize, caps);
      if (p == NULL)
        {
          /* allocation failed; original pointer still valid */
          return NULL;
        }
      return p;
    }
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

static int
lhos_lua_yield (lua_State *L)
{
  int ms = (int)luaL_checkinteger (L, 1);
  ESP_LOGI (TAG, "Lua requested yield %d ms", ms);
  /* If this call is from the main thread (not yieldable), perform a
     cooperative wait: process due scheduled coroutines while waiting. */
  if (!lua_isyieldable (L))
    {
      uint64_t start = esp_timer_get_time () / 1000ULL;
      uint64_t until = start + (uint64_t)ms;
      while (1)
        {
          uint64_t now = esp_timer_get_time () / 1000ULL;
          /* process any due coroutines */
          while (sched_head && sched_head->resume_at_ms <= now)
            {
              sched_node_t *n = sched_head;
              sched_head = n->next;
              int nres = 0;
              int rc = lua_resume (n->co, L, 0, &nres);
              if (rc == LUA_OK)
                {
                  ESP_LOGI (TAG, "Coroutine finished");
                }
              else if (rc == LUA_YIELD)
                {
                  ESP_LOGI (TAG, "Coroutine yielded; awaiting reschedule");
                }
              else
                {
                  ESP_LOGE (TAG, "Coroutine resume error: %s",
                            lua_tostring (n->co, -1));
                  lua_pop (n->co, 1);
                }
              heap_caps_free (n);
            }
          if (now >= until)
            break;
          vTaskDelay (pdMS_TO_TICKS (1));
        }
      return 0;
    }

  /* Schedule this coroutine to be resumed after `ms` milliseconds. */
  {
    uint64_t now = esp_timer_get_time () / 1000ULL;
    lhos_lua_schedule_add (L, now + (uint64_t)ms);
  }
  return lua_yieldk (L, 0, 0, NULL);
}

/* POSIX-like file operations exposed to Lua under `lhos.posix` */
static int
lhos_lua_posix_open (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  const char *mode = luaL_optstring (L, 2, "r");
  int flags = 0;
  if (strcmp (mode, "r") == 0)
    flags = O_RDONLY;
  else if (strcmp (mode, "r+") == 0)
    flags = O_RDWR;
  else if (strcmp (mode, "w") == 0)
    flags = O_WRONLY | O_CREAT | O_TRUNC;
  else if (strcmp (mode, "w+") == 0)
    flags = O_RDWR | O_CREAT | O_TRUNC;
  else if (strcmp (mode, "a") == 0)
    flags = O_WRONLY | O_CREAT | O_APPEND;
  else if (strcmp (mode, "a+") == 0)
    flags = O_RDWR | O_CREAT | O_APPEND;
  else
    flags = O_RDONLY;

  int fd = open (path, flags, 0644);
  if (fd >= 0)
    {
      lua_pushinteger (L, fd);
      return 1;
    }
  lua_pushnil (L);
  lua_pushstring (L, strerror (errno));
  return 2;
}

static int
lhos_lua_posix_read (lua_State *L)
{
  int fd = (int)luaL_checkinteger (L, 1);
  size_t cnt = (size_t)luaL_checkinteger (L, 2);
  if (cnt == 0)
    {
      lua_pushliteral (L, "");
      return 1;
    }
  void *buf = heap_caps_malloc (cnt, MALLOC_CAP_DEFAULT);
  if (!buf)
    {
      lua_pushnil (L);
      lua_pushstring (L, "ENOMEM");
      return 2;
    }
  ssize_t r = read (fd, buf, cnt);
  if (r < 0)
    {
      lua_pushnil (L);
      lua_pushstring (L, strerror (errno));
      heap_caps_free (buf);
      return 2;
    }
  lua_pushlstring (L, (const char *)buf, (size_t)r);
  heap_caps_free (buf);
  return 1;
}

static int
lhos_lua_posix_write (lua_State *L)
{
  int fd = (int)luaL_checkinteger (L, 1);
  size_t len = 0;
  const char *data = luaL_checklstring (L, 2, &len);
  ssize_t w = write (fd, data, len);
  if (w < 0)
    {
      lua_pushnil (L);
      lua_pushstring (L, strerror (errno));
      return 2;
    }
  lua_pushinteger (L, w);
  return 1;
}

static int
lhos_lua_posix_close (lua_State *L)
{
  int fd = (int)luaL_checkinteger (L, 1);
  if (close (fd) == 0)
    {
      lua_pushboolean (L, 1);
      return 1;
    }
  lua_pushnil (L);
  lua_pushstring (L, strerror (errno));
  return 2;
}

static int
lhos_lua_posix_stat (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  struct stat st;
  if (stat (path, &st) != 0)
    {
      lua_pushnil (L);
      lua_pushstring (L, strerror (errno));
      return 2;
    }
  lua_newtable (L);
  lua_pushinteger (L, (lua_Integer)st.st_size);
  lua_setfield (L, -2, "size");
  lua_pushinteger (L, (lua_Integer)st.st_mode);
  lua_setfield (L, -2, "mode");
  lua_pushinteger (L, (lua_Integer)st.st_mtime);
  lua_setfield (L, -2, "mtime");
  return 1;
}

static int
lhos_lua_posix_unlink (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  if (unlink (path) == 0)
    {
      lua_pushboolean (L, 1);
      return 1;
    }
  lua_pushnil (L);
  lua_pushstring (L, strerror (errno));
  return 2;
}

static int
lhos_lua_posix_rename (lua_State *L)
{
  const char *oldp = luaL_checkstring (L, 1);
  const char *newp = luaL_checkstring (L, 2);
  if (rename (oldp, newp) == 0)
    {
      lua_pushboolean (L, 1);
      return 1;
    }
  lua_pushnil (L);
  lua_pushstring (L, strerror (errno));
  return 2;
}

static int
lhos_lua_posix_mkdir (lua_State *L)
{
  const char *path = luaL_checkstring (L, 1);
  int mode = (int)luaL_optinteger (L, 2, 0755);
  if (mkdir (path, (mode_t)mode) == 0)
    {
      lua_pushboolean (L, 1);
      return 1;
    }
  lua_pushnil (L);
  lua_pushstring (L, strerror (errno));
  return 2;
}

static int
lhos_lua_post_status (lua_State *L)
{
  lhos_post_status_t status;
  esp_err_t err = lhos_post_get_status (&status);
  if (err != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to get POST status");
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
  lua_pushboolean (L, status.wifi_ok);
  lua_setfield (L, -2, "wifi_ok");
  lua_pushboolean (L, status.ble_ok);
  lua_setfield (L, -2, "ble_ok");

  return 1;
}

/* UART operations exposed to Lua under `lhos.uart` */
static int
lhos_lua_uart_open (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);
  int baud_rate = (int)luaL_checkinteger (L, 2);
  int tx_pin = (int)luaL_checkinteger (L, 3);
  int rx_pin = (int)luaL_checkinteger (L, 4);

  uart_config_t uart_config = {
    .baud_rate = baud_rate,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };

  esp_err_t err = uart_param_config ((uart_port_t)uart_num, &uart_config);
  if (err != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to config UART");
      return 2;
    }

  err = uart_set_pin ((uart_port_t)uart_num, tx_pin, rx_pin,
                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (err != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to set UART pins");
      return 2;
    }

  err = uart_driver_install ((uart_port_t)uart_num, 256, 0, 0, NULL, 0);
  if (err != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to install UART driver");
      return 2;
    }

  lua_pushboolean (L, true);
  return 1;
}

static int
lhos_lua_uart_close (lua_State *L)
{
  int uart_num = (int)luaL_checkinteger (L, 1);

  esp_err_t err = uart_driver_delete ((uart_port_t)uart_num);
  if (err != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to delete UART driver");
      return 2;
    }

  lua_pushboolean (L, true);
  return 1;
}

/* WiFi operations exposed to Lua under `lhos.wifi` */
static int
lhos_lua_wifi_init (lua_State *L)
{
  esp_err_t ret = esp_netif_init ();
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to init netif");
      return 2;
    }
  ret = esp_event_loop_create_default ();
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to create event loop");
      return 2;
    }
  esp_netif_create_default_wifi_sta ();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT ();
  ret = esp_wifi_init (&cfg);
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to init WiFi");
      return 2;
    }
  ret = esp_wifi_set_mode (WIFI_MODE_STA);
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to set WiFi mode");
      return 2;
    }
  lua_pushboolean (L, true);
  return 1;
}

static int
lhos_lua_wifi_connect (lua_State *L)
{
  const char *ssid = luaL_checkstring (L, 1);
  const char *password = luaL_checkstring (L, 2);
  wifi_config_t wifi_config = { 0 };
  strcpy ((char *)wifi_config.sta.ssid, ssid);
  strcpy ((char *)wifi_config.sta.password, password);
  esp_err_t ret = esp_wifi_set_config (WIFI_IF_STA, &wifi_config);
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to set WiFi config");
      return 2;
    }
  ret = esp_wifi_start ();
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to start WiFi");
      return 2;
    }
  ret = esp_wifi_connect ();
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to connect WiFi");
      return 2;
    }
  lua_pushboolean (L, true);
  return 1;
}

static int
lhos_lua_wifi_disconnect (lua_State *L)
{
  esp_err_t ret = esp_wifi_disconnect ();
  if (ret != ESP_OK)
    {
      lua_pushnil (L);
      lua_pushstring (L, "Failed to disconnect WiFi");
      return 2;
    }
  lua_pushboolean (L, true);
  return 1;
}

static int
lhos_lua_wifi_get_status (lua_State *L)
{
  wifi_ap_record_t ap_info;
  esp_err_t ret = esp_wifi_sta_get_ap_info (&ap_info);
  if (ret == ESP_OK)
    {
      lua_pushstring (L, "connected");
    }
  else
    {
      lua_pushstring (L, "disconnected");
    }
  return 1;
}

/* LED operations exposed to Lua under `lhos.led` */

static int
lhos_lua_led_set (lua_State *L)
{
  int pin = luaL_checkinteger (L, 1);
  int level = lua_toboolean (L, 2);

  esp_err_t ret = gpio_set_level (pin, level);
  if (ret != ESP_OK)
    {
      lua_pushstring (L, "Failed to set LED level");
      lua_error (L);
    }
  return 0;
}

static int
lhos_lua_led_get (lua_State *L)
{
  int pin = luaL_checkinteger (L, 1);
  int level = gpio_get_level (pin);
  lua_pushboolean (L, level);
  return 1;
}

static int
lhos_lua_led_config (lua_State *L)
{
  int pin = luaL_checkinteger (L, 1);
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << pin),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  esp_err_t ret = gpio_config (&io_conf);
  if (ret != ESP_OK)
    {
      lua_pushstring (L, "Failed to configure LED pin");
      lua_error (L);
    }
  return 0;
}

void
lhos_lua_init (void)
{
  if (L)
    return;
  ESP_LOGI (TAG, "Initializing Lua VM");
  L = lua_newstate (l_alloc, NULL, 0);
  if (!L)
    {
      ESP_LOGE (TAG, "Failed to create Lua state");
      return;
    }
  luaL_openlibs (L);

  /* Register lhos table with `yield` function and override `print`. */
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_yield);
  lua_setfield (L, -2, "yield");
  lua_pushcfunction (L, lhos_lua_post_status);
  lua_setfield (L, -2, "post_status");

  /* Create lhos.posix table with POSIX-like file I/O bindings */
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_posix_open);
  lua_setfield (L, -2, "open");
  lua_pushcfunction (L, lhos_lua_posix_read);
  lua_setfield (L, -2, "read");
  lua_pushcfunction (L, lhos_lua_posix_write);
  lua_setfield (L, -2, "write");
  lua_pushcfunction (L, lhos_lua_posix_close);
  lua_setfield (L, -2, "close");
  lua_pushcfunction (L, lhos_lua_posix_stat);
  lua_setfield (L, -2, "stat");
  lua_pushcfunction (L, lhos_lua_posix_unlink);
  lua_setfield (L, -2, "unlink");
  lua_pushcfunction (L, lhos_lua_posix_rename);
  lua_setfield (L, -2, "rename");
  lua_pushcfunction (L, lhos_lua_posix_mkdir);
  lua_setfield (L, -2, "mkdir");

  /* attach posix table to lhos */
  lua_setfield (L, -2, "posix");

  /* Create lhos.uart table with UART bindings */
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_uart_open);
  lua_setfield (L, -2, "open");
  lua_pushcfunction (L, lhos_lua_uart_close);
  lua_setfield (L, -2, "close");

  /* attach uart table to lhos */
  lua_setfield (L, -2, "uart");

  /* Create lhos.wifi table with WiFi bindings */
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_wifi_init);
  lua_setfield (L, -2, "init");
  lua_pushcfunction (L, lhos_lua_wifi_connect);
  lua_setfield (L, -2, "connect");
  lua_pushcfunction (L, lhos_lua_wifi_disconnect);
  lua_setfield (L, -2, "disconnect");
  lua_pushcfunction (L, lhos_lua_wifi_get_status);
  lua_setfield (L, -2, "get_status");

  /* attach wifi table to lhos */
  lua_setfield (L, -2, "wifi");

  /* Create lhos.led table with LED bindings */
  lua_newtable (L);
  lua_pushcfunction (L, lhos_lua_led_config);
  lua_setfield (L, -2, "config");
  lua_pushcfunction (L, lhos_lua_led_set);
  lua_setfield (L, -2, "set");
  lua_pushcfunction (L, lhos_lua_led_get);
  lua_setfield (L, -2, "get");

  /* attach led table to lhos */
  lua_setfield (L, -2, "led");

  lua_setglobal (L, "lhos");

  lua_pushcfunction (L, lhos_lua_print);
  lua_setglobal (L, "print");
}

int
lhos_lua_run_script (const char *script)
{
  if (!L)
    {
      ESP_LOGW (TAG, "Lua VM not initialized");
      return -1;
    }
  if (luaL_loadstring (L, script) != LUA_OK)
    {
      ESP_LOGE (TAG, "Lua load error: %s", lua_tostring (L, -1));
      lua_pop (L, 1);
      return -1;
    }
  if (lua_pcall (L, 0, LUA_MULTRET, 0) != LUA_OK)
    {
      ESP_LOGE (TAG, "Lua runtime error: %s", lua_tostring (L, -1));
      lua_pop (L, 1);
      return -1;
    }
  return 0;
}

/* Scheduler implementation -------------------------------------------------*/
void
lhos_lua_schedule_add (lua_State *co, uint64_t when_ms)
{
  sched_node_t *n
      = heap_caps_malloc (sizeof (sched_node_t), MALLOC_CAP_DEFAULT);
  if (!n)
    {
      ESP_LOGE (TAG, "Failed to allocate scheduler node");
      return;
    }
  n->co = co;
  n->resume_at_ms = when_ms;
  n->next = NULL;

  /* insert sorted by resume_at_ms */
  if (!sched_head || sched_head->resume_at_ms > when_ms)
    {
      n->next = sched_head;
      sched_head = n;
      return;
    }
  sched_node_t *cur = sched_head;
  while (cur->next && cur->next->resume_at_ms <= when_ms)
    cur = cur->next;
  n->next = cur->next;
  cur->next = n;
}

void
lhos_lua_scheduler_run (void)
{
  while (1)
    {
      if (!sched_head)
        return; /* nothing scheduled, exit scheduler */

      uint64_t now = esp_timer_get_time () / 1000ULL;
      if (sched_head->resume_at_ms > now)
        {
          uint64_t wait_ms = sched_head->resume_at_ms - now;
          vTaskDelay (pdMS_TO_TICKS ((uint32_t)wait_ms));
          continue;
        }

      /* pop head */
      sched_node_t *n = sched_head;
      sched_head = n->next;

      /* Resume the coroutine. `L` is the main state created in
       * lhos_lua_init.
       */
      int nres = 0;
      int rc = lua_resume (n->co, L, 0, &nres);
      if (rc == LUA_OK)
        {
          ESP_LOGI (TAG, "Coroutine finished");
        }
      else if (rc == LUA_YIELD)
        {
          /* coroutine yielded again; lhos_lua_yield already re-scheduled it
           */
          ESP_LOGI (TAG, "Coroutine yielded; awaiting reschedule");
        }
      else
        {
          ESP_LOGE (TAG, "Coroutine resume error: %s",
                    lua_tostring (n->co, -1));
          lua_pop (n->co, 1);
        }

      heap_caps_free (n);
    }
}

bool
lhos_lua_enabled (void)
{
#ifdef CONFIG_LUA_ENABLED
  return true;
#else
  return false;
#endif
}

#else /* CONFIG_LUA_ENABLED not set */

void
lhos_lua_init (void)
{
  ESP_LOGI (TAG, "Initializing Lua integration stub (no real Lua linked)");
}

int
lhos_lua_run_script (const char *script)
{
  ESP_LOGI (TAG, "Run script stub: %s", script ? script : "<null>");
  return 0;
}

bool
lhos_lua_enabled (void)
{
#ifdef CONFIG_LUA_ENABLED
  return true;
#else
  return false;
#endif
}

#endif /* CONFIG_LUA_ENABLED */
