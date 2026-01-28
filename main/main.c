#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#define LHOS_USE_LITTLEFS 1
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lhos.h"
#include "lhos_config.h"
#include "lhos_lua.h"
#include "lhos_ws2812b.h"
#include <sys/stat.h>

static const char *TAG = "LHOS_APP";

adc_oneshot_unit_handle_t adc1_handle;

typedef struct
{
  const char *script;
} lua_task_arg_t;

static void
lhos_lua_task (void *pv)
{
  lua_task_arg_t *targ = (lua_task_arg_t *)pv;
  ESP_LOGI (TAG, "Lua task started");
  if (targ && targ->script)
    {
      ESP_LOGI (TAG, "Initializing Lua VM");
      lhos_lua_init ();
      ESP_LOGI (TAG, "Running Lua script");
      lhos_lua_run_script (targ->script);
      /* Run scheduler to resume yielded coroutines until none remain. */
      ESP_LOGI (TAG, "Running Lua scheduler");
      lhos_lua_scheduler_run ();
      ESP_LOGI (TAG, "Lua execution complete");
    }
  if (targ)
    {
      if (targ->script)
        vPortFree ((void *)targ->script);
      vPortFree (targ);
    }
  ESP_LOGI (TAG, "Lua task ending");
  vTaskDelete (NULL);
}

void
app_main (void)
{
  esp_log_level_set ("*", ESP_LOG_INFO);
  ESP_LOGI (TAG, "Starting lhOS...");

  /* Initialize NVS early for WiFi/BLE tests */
  ESP_LOGI (TAG, "Initializing NVS...");
  esp_err_t ret = nvs_flash_init ();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK (nvs_flash_erase ());
      ret = nvs_flash_init ();
    }
  ESP_ERROR_CHECK (ret);
  ESP_LOGI (TAG, "NVS initialized");

  /* Initialize LED early */
  ESP_LOGI (TAG, "Initializing WS2812B LED...");
  // lhos_ws2812b_init (); // TEMPORARILY DISABLED FOR DEBUG
  ESP_LOGI (TAG, "WS2812B LED initialized (disabled)");

  /* POST disabled: component removed in this build */
  ESP_LOGI (TAG, "POST disabled in this build");

  ESP_LOGI (TAG, "Initializing config...");
  lhos_config_init ();
  ESP_LOGI (TAG, "Initializing lhOS...");
  lhos_init ();

  /* Configure ADC for sensor on GPIO 1 */
  ESP_LOGI (TAG, "Configuring ADC for sensor on GPIO 1...");
  adc_oneshot_unit_init_cfg_t init_config1 = {
    .unit_id = ADC_UNIT_1,
  };
  esp_err_t adc_err = adc_oneshot_new_unit (&init_config1, &adc1_handle);
  if (adc_err != ESP_OK)
    {
      ESP_LOGE (TAG, "Failed to initialize ADC: %s",
                esp_err_to_name (adc_err));
    }
  else
    {
      adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
      };
      adc_oneshot_config_channel (adc1_handle, ADC_CHANNEL_0, &config);
      ESP_LOGI (TAG, "ADC configured for sensor on GPIO 1");
    }
  /* Start Lua VM on a dedicated task pinned to core 1 so initialization
     (which may perform heavy allocations) doesn't block the main task
     and trigger the Task WDT. */
  if (lhos_lua_enabled ())
    {
      ESP_LOGI (TAG, "Starting Lua task on core 1");
      // No embedded script, load from filesystem

      /* Try to mount a filesystem and read init.lua; prefer LittleFS when
         available at build time, otherwise fall back to SPIFFS. */
      const char *script_to_use = NULL;
      char *file_buf = NULL;
      struct stat st;
#if defined(LHOS_USE_LITTLEFS)
      esp_vfs_littlefs_conf_t conf = {
        .base_path = "/lfs",
        .partition_label = "lua_fs",
        .format_if_mount_failed = 1,
        .read_only = 0,
        .dont_mount = 0,
        .grow_on_mount = 1,
      };
      esp_err_t rc = esp_vfs_littlefs_register (&conf);
      if (rc == ESP_OK)
        {
          if (stat ("/lfs/scripts/post_soft.lua", &st) == 0 && st.st_size > 0)
            {
              FILE *f = fopen ("/lfs/scripts/post_soft.lua", "rb");
              if (f)
                {
                  file_buf = pvPortMalloc (st.st_size + 1);
                  if (file_buf)
                    {
                      size_t r = fread (file_buf, 1, st.st_size, f);
                      file_buf[r] = '\0';
                      script_to_use = file_buf;
                    }
                  fclose (f);
                }
            }
          esp_vfs_littlefs_unregister ("lua_fs");
        }
#else
#error "No filesystem support compiled in"
#endif

      if (!script_to_use)
        {
          ESP_LOGW (TAG, "No Lua script found in filesystem");
          script_to_use = NULL;
        }

      lua_task_arg_t *arg = pvPortMalloc (sizeof (lua_task_arg_t));
      if (arg == NULL)
        {
          ESP_LOGE (TAG, "Failed to allocate lua task arg");
          if (file_buf)
            vPortFree (file_buf);
        }
      else
        {
          arg->script = script_to_use;

          BaseType_t ok
              = xTaskCreatePinnedToCore (lhos_lua_task, "lhos_lua", 8192, arg,
                                         tskIDLE_PRIORITY + 5, NULL, 1);

          if (ok != pdPASS)
            {
              ESP_LOGW (
                  TAG,
                  "Failed to create Lua task; running init in main instead");
              /* fallback: run inline but be aware of WDT */
              lhos_lua_init ();
              lhos_lua_run_script (arg->script);
              if (arg->script)
                vPortFree ((void *)arg->script);
              vPortFree (arg);
            }
        }
    }
}
