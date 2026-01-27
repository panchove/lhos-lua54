#include "lhos_lua.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LHOS_LUA";

void
lhos_lua_init (void)
{
  ESP_LOGI (TAG, "Initializing Lua integration stub (no real Lua linked)");

  if (LHOS_LUA_ALLOC_SPIRAM)
    {
      ESP_LOGI (
          TAG,
          "Configured to allocate Lua heap in SPIRAM (MALLOC_CAP_SPIRAM)");
      /* Real allocator setup should be done by Lua component when linked. */
    }
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
