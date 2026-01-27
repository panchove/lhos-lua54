#include "lhos_config.h"
#include "esp_log.h"

static const char *TAG = "LHOS_CONFIG";

// Color table for RGB LED (common anode: 0=on, 1=off)
const lhos_led_rgb_t lhos_led_color_table[] = {
  { 1, 1, 1 }, // OFF
  { 0, 1, 1 }, // RED
  { 1, 0, 1 }, // GREEN
  { 1, 1, 0 }, // BLUE
  { 0, 0, 0 }, // WHITE
  { 0, 0, 1 }, // YELLOW
  { 1, 0, 0 }, // CYAN
  { 0, 1, 0 }  // MAGENTA
};

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
