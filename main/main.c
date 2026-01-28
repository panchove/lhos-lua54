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
#include "lhos_post.h"
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

/* Blocking LED sanity loop used for on-device diagnosis (option C). */
static void
lhos_led_sanity_loop (void)
{
  ESP_LOGI (TAG, "Starting LED sanity loop (option C)");
  for (int i = 0; i < 10; ++i)
    {
      ESP_LOGI (TAG, "Sanity cycle %d/10: BLUE -> OFF -> GREEN", i + 1);
      /* Blue (0xRRGGBB) */
      lhos_ws2812b_set_color (0x0000FF);
      vTaskDelay (pdMS_TO_TICKS (2000));
      lhos_ws2812b_set_color (0x000000);
      vTaskDelay (pdMS_TO_TICKS (500));
      /* Green */
      lhos_ws2812b_set_color (0x00FF00);
      vTaskDelay (pdMS_TO_TICKS (300));
      lhos_ws2812b_set_color (0x000000);
      vTaskDelay (pdMS_TO_TICKS (200));
    }
  ESP_LOGI (TAG, "LED sanity loop complete");
}

/* Quick GPIO toggle diagnostic on WS2812B data pin to check wiring/power.
 * This toggles the pin as a plain GPIO (not RMT) a few times, then returns
 * the pin to input so the RMT peripheral can later claim it. */
static void
lhos_ws2812b_gpio_toggle (void)
{
  ESP_LOGI (TAG, "GPIO toggle diagnostic on pin %d", WS2812B_GPIO_PIN);
  gpio_config_t cfg = {
    .pin_bit_mask = (1ULL << WS2812B_GPIO_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config (&cfg);

  for (int i = 0; i < 4; ++i)
    {
      gpio_set_level (WS2812B_GPIO_PIN, 1);
      vTaskDelay (pdMS_TO_TICKS (150));
      gpio_set_level (WS2812B_GPIO_PIN, 0);
      vTaskDelay (pdMS_TO_TICKS (150));
    }

  /* Drive low briefly then release to input so RMT can use the pin */
  gpio_set_level (WS2812B_GPIO_PIN, 0);
  gpio_config_t cfg_in = {
    .pin_bit_mask = (1ULL << WS2812B_GPIO_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config (&cfg_in);
  ESP_LOGI (TAG, "GPIO toggle diagnostic complete");
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

  /* Quick GPIO diagnostic to validate wiring/power on WS2812B data pin (48) */
  lhos_ws2812b_gpio_toggle ();

  /* Initialize LED early */
  ESP_LOGI (TAG, "Initializing WS2812B LED...");
  if (lhos_ws2812b_init () != ESP_OK)
    {
      ESP_LOGW (TAG, "WS2812B init failed or disabled");
    }
  else
    {
      ESP_LOGI (TAG, "WS2812B LED initialized");
      /* Run WS2812B self-test (includes full diagnostic) for on-device
       * verification */
      esp_err_t st = lhos_ws2812b_selftest ();
      ESP_LOGI (TAG, "ws2812 selftest result: %s", esp_err_to_name (st));
    }

  /* Quick hardware sanity test: set LED to bright yellow for 1s then off */
  lhos_ws2812b_set_color (0xFFC800);
  vTaskDelay (pdMS_TO_TICKS (1000));
  lhos_ws2812b_set_color (0x000000);
  /* Run Power-On Self Test (POST) and indicate status via LED */
  {
    lhos_post_status_t pst;
    ESP_LOGI (TAG, "Running POST hardware checks...");
    esp_err_t perr = lhos_post_hardware (&pst);
    if (perr == ESP_OK)
      {
        ESP_LOGI (TAG, "POST: all checks passed");
      }
    else
      {
        ESP_LOGW (TAG, "POST: some checks failed (status logged)");
      }
    /* Indicate via LED */
    lhos_post_led_indicate (&pst);
  }

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
          /* Prefer boot.lua if present, otherwise fallback to post_soft.lua */
          if (stat ("/lfs/scripts/boot.lua", &st) == 0 && st.st_size > 0)
            {
              FILE *f = fopen ("/lfs/scripts/boot.lua", "rb");
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
          else if (stat ("/lfs/scripts/post_soft.lua", &st) == 0
                   && st.st_size > 0)
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
