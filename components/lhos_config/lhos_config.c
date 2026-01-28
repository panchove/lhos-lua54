#include "lhos_config.h"
#include "esp_log.h"

static const char *TAG = "LHOS_CONFIG";

/* Legacy lhos_led_color_table removed; use lhos_palette_rgb24 or
   direct WS2812 API with packed RGB24 (0xRRGGBB). */

/* 16-color palette (RGB24 values) - matches Lua `config.led.palette` ordering
 */
/* Palette array removed; use LHOS_PAL_* macros or Lua-side palette in
  main/lua_fs/scripts/config.lua if needed. */

void
lhos_config_init (void)
{
  ESP_LOGI (TAG, "Initializing lhOS config (sdkconfig defaults)");
  ESP_LOGI (TAG, "Lua enabled: %s", lhos_config_lua_enabled () ? "yes" : "no");
  ESP_LOGI (TAG, "Lua use SPIRAM: %s",
            lhos_config_lua_use_spiram () ? "yes" : "no");
  ESP_LOGI (TAG, "Target: %s", lhos_config_target ());
}

bool
lhos_config_lua_enabled (void)
{
#ifdef CONFIG_LUA_ENABLED
  return true;
#else
  return false;
#endif
}

bool
lhos_config_lua_use_spiram (void)
{
#ifdef CONFIG_LUA_USE_SPIRAM
  return true;
#else
  return false;
#endif
}

const char *
lhos_config_target (void)
{
#ifdef CONFIG_IDF_TARGET_ESP32S3
  return "esp32s3";
#elif defined(CONFIG_IDF_TARGET_ESP32)
  return "esp32";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  return "esp32s2";
#else
  return "unknown";
#endif
}
