#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lhos.h"
#include "lhos_config.h"
#include "lhos_lua.h"

static const char *TAG = "LHOS_APP";

void
app_main (void)
{
  esp_log_level_set ("*", ESP_LOG_INFO);
  ESP_LOGI (TAG, "Starting lhOS placeholder...");
  lhos_config_init ();
  lhos_init ();
  lhos_lua_init ();

  if (lhos_lua_enabled ())
    {
      ESP_LOGI (TAG, "Lua integration enabled (CONFIG_LUA_ENABLED=1)");
      const char *lua_script
          = "-- lua example: coroutine + non-blocking yields\n"
            "local co = coroutine.create(function()\n"
            "  for i = 1, 10 do\n"
            "    print('Lua coroutine tick', i)\n"
            "    lhos.yield(1000) -- yield back to host for 1000 ms\n"
            "  end\n"
            "end)\n"
            "\n"
            "function on_event(ev)\n"
            "  print('Lua on_event:', ev)\n"
            "end\n"
            "\n"
            "coroutine.resume(co)\n"
            "\n"
            "-- simulate incoming events\n"
            "for i = 1,3 do\n"
            "  lhos.yield(500)\n"
            "  on_event('event_' .. i)\n"
            "end\n"
            "\n"
            "print('Lua script completed')\n";

      ESP_LOGI (TAG, "Lua script (display only):\n%s", lua_script);
    }

  while (1)
    {
      vTaskDelay (pdMS_TO_TICKS (1000));
      ESP_LOGI (TAG, "lhOS heartbeat");
    }
}
