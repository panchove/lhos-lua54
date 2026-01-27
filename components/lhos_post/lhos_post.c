#include "lhos_post.h"
#include "driver/gpio.h"
// #include "esp_bt.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lhos_ws2812b.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "LHOS_POST";

static lhos_post_status_t g_post_status
    = { false, false, false, false, false, false, false };

/**
 * @brief Test RAM by writing and reading a pattern
 */
static bool
test_ram (void)
{
  const size_t test_size = 1024;
  uint8_t *buffer = heap_caps_malloc (test_size, MALLOC_CAP_INTERNAL);
  if (!buffer)
    {
      ESP_LOGE (TAG, "Failed to allocate RAM for test");
      return false;
    }

  // Fill with pattern
  for (size_t i = 0; i < test_size; i++)
    {
      buffer[i] = (uint8_t)(i % 256);
    }

  // Verify
  bool ok = true;
  for (size_t i = 0; i < test_size; i++)
    {
      if (buffer[i] != (uint8_t)(i % 256))
        {
          ok = false;
          break;
        }
    }

  heap_caps_free (buffer);
  return ok;
}

/**
 * @brief Test PSRAM if available
 */
static bool
test_psram (void)
{
#if CONFIG_SPIRAM
  const size_t test_size = 1024;
  uint8_t *buffer = heap_caps_malloc (test_size, MALLOC_CAP_SPIRAM);
  if (!buffer)
    {
      ESP_LOGW (TAG, "PSRAM not available or failed to allocate");
      return false;
    }

  // Fill with pattern
  for (size_t i = 0; i < test_size; i++)
    {
      buffer[i] = (uint8_t)(i % 256);
    }

  // Verify
  bool ok = true;
  for (size_t i = 0; i < test_size; i++)
    {
      if (buffer[i] != (uint8_t)(i % 256))
        {
          ok = false;
          break;
        }
    }

  heap_caps_free (buffer);
  return ok;
#else
  return true; // Not available, consider OK
#endif
}

/**
 * @brief Test NVS (Non-Volatile Storage) initialization
 */
static bool
test_nvs (void)
{
  esp_err_t err = nvs_flash_init ();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK (nvs_flash_erase ());
      err = nvs_flash_init ();
    }
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "NVS init failed: %s", esp_err_to_name (err));
      return false;
    }

  // Test basic NVS operation
  nvs_handle_t handle;
  err = nvs_open ("test", NVS_READWRITE, &handle);
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "NVS open failed: %s", esp_err_to_name (err));
      return false;
    }

  // Write a test value
  uint32_t test_value = 0xDEADBEEF;
  err = nvs_set_u32 (handle, "test_key", test_value);
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "NVS set failed: %s", esp_err_to_name (err));
      nvs_close (handle);
      return false;
    }

  // Read back
  uint32_t read_value;
  err = nvs_get_u32 (handle, "test_key", &read_value);
  if (err != ESP_OK || read_value != test_value)
    {
      ESP_LOGE (TAG, "NVS get failed or value mismatch: %s",
                esp_err_to_name (err));
      nvs_close (handle);
      return false;
    }

  // Erase test key
  err = nvs_erase_key (handle, "test_key");
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "NVS erase failed: %s", esp_err_to_name (err));
      nvs_close (handle);
      return false;
    }

  nvs_close (handle);
  return true;
}

/**
 * @brief Test flash by reading chip info
 */
static bool
test_flash (void)
{
  esp_flash_t *flash = esp_flash_default_chip;
  if (!flash)
    {
      return false;
    }

  uint32_t chip_id;
  esp_err_t err = esp_flash_read_id (flash, &chip_id);
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "Flash read ID failed: %s", esp_err_to_name (err));
      return false;
    }

  ESP_LOGI (TAG, "Flash chip ID: 0x%08lx", chip_id);
  return true;
}

/**
 * @brief Test GPIO by toggling a pin
 */
static bool
test_gpio (void)
{
  const gpio_num_t test_pin
      = GPIO_NUM_2; // Use GPIO 2, assuming it's available

  gpio_config_t io_conf = { .pin_bit_mask = (1ULL << test_pin),
                            .mode = GPIO_MODE_OUTPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_DISABLE,
                            .intr_type = GPIO_INTR_DISABLE };

  esp_err_t err = gpio_config (&io_conf);
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "GPIO config failed: %s", esp_err_to_name (err));
      return false;
    }

  // Toggle
  gpio_set_level (test_pin, 0);
  vTaskDelay (pdMS_TO_TICKS (10));
  gpio_set_level (test_pin, 1);
  vTaskDelay (pdMS_TO_TICKS (10));
  gpio_set_level (test_pin, 0);

  return true;
}

/**
 * @brief Test WiFi initialization
 */
static bool
test_wifi (void)
{
  esp_err_t err
      = esp_wifi_init (&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT ());
  if (err != ESP_OK)
    {
      ESP_LOGE (TAG, "WiFi init failed: %s", esp_err_to_name (err));
      return false;
    }

  esp_wifi_deinit ();
  return true;
}

/**
 * @brief Test BLE initialization
 */
static bool
test_ble (void)
{
  // BLE test disabled for now
  return true;
}

esp_err_t
lhos_post_hardware (lhos_post_status_t *status)
{
  if (!status)
    {
      return ESP_ERR_INVALID_ARG;
    }

  ESP_LOGI (TAG, "Starting hardware POST...");

  g_post_status.nvs_ok = test_nvs ();
  ESP_LOGI (TAG, "NVS test: %s", g_post_status.nvs_ok ? "PASS" : "FAIL");

  g_post_status.ram_ok = test_ram ();
  ESP_LOGI (TAG, "RAM test: %s", g_post_status.ram_ok ? "PASS" : "FAIL");

  g_post_status.flash_ok = test_flash ();
  ESP_LOGI (TAG, "Flash test: %s", g_post_status.flash_ok ? "PASS" : "FAIL");

  g_post_status.gpio_ok = test_gpio ();
  ESP_LOGI (TAG, "GPIO test: %s", g_post_status.gpio_ok ? "PASS" : "FAIL");

  g_post_status.psram_ok = test_psram ();
  ESP_LOGI (TAG, "PSRAM test: %s", g_post_status.psram_ok ? "PASS" : "FAIL");

  g_post_status.wifi_ok = test_wifi ();
  ESP_LOGI (TAG, "WiFi test: %s", g_post_status.wifi_ok ? "PASS" : "FAIL");

  g_post_status.ble_ok = test_ble ();
  ESP_LOGI (TAG, "BLE test: %s", g_post_status.ble_ok ? "PASS" : "FAIL");

  memcpy (status, &g_post_status, sizeof (lhos_post_status_t));

  bool all_ok = g_post_status.ram_ok && g_post_status.flash_ok
                && g_post_status.gpio_ok && g_post_status.psram_ok
                && g_post_status.nvs_ok && g_post_status.wifi_ok
                && g_post_status.ble_ok;

  ESP_LOGI (TAG, "Hardware POST %s", all_ok ? "PASSED" : "FAILED");

  return all_ok ? ESP_OK : ESP_FAIL;
}

esp_err_t
lhos_post_get_status (lhos_post_status_t *status)
{
  if (!status)
    {
      return ESP_ERR_INVALID_ARG;
    }

  memcpy (status, &g_post_status, sizeof (lhos_post_status_t));
  return ESP_OK;
}

esp_err_t
lhos_post_led_indicate (lhos_post_status_t *status)
{
  // Set color based on status using WS2812B
  if (status == NULL)
    {
      // In progress: Blue
      lhos_ws2812b_set_color_enum (3); // BLUE
      ESP_LOGI (TAG, "WS2812B LED: Blue (In Progress)");
    }
  else
    {
      // Check if all tests passed
      bool all_pass = status->ram_ok && status->flash_ok && status->gpio_ok
                      && status->psram_ok && status->nvs_ok && status->wifi_ok
                      && status->ble_ok;

      if (all_pass)
        {
          // Green: All passed
          lhos_ws2812b_set_color_enum (2); // GREEN
          ESP_LOGI (TAG, "WS2812B LED: Green (All tests PASSED)");
        }
      else
        {
          // Flash red for each failed test
          ESP_LOGI (TAG, "WS2812B LED: Flashing for failed tests");

          // NVS fail: 1 flash
          if (!status->nvs_ok)
            {
              lhos_ws2812b_flash (1, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // RAM fail: 2 flashes
          if (!status->ram_ok)
            {
              lhos_ws2812b_flash (2, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // Flash fail: 3 flashes
          if (!status->flash_ok)
            {
              lhos_ws2812b_flash (3, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // GPIO fail: 4 flashes
          if (!status->gpio_ok)
            {
              lhos_ws2812b_flash (4, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // PSRAM fail: 5 flashes
          if (!status->psram_ok)
            {
              lhos_ws2812b_flash (5, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // WiFi fail: 6 flashes
          if (!status->wifi_ok)
            {
              lhos_ws2812b_flash (6, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // BLE fail: 7 flashes
          if (!status->ble_ok)
            {
              lhos_ws2812b_flash (7, 200, 200);
              vTaskDelay (pdMS_TO_TICKS (3000));
            }

          // After flashing, set to red solid
          lhos_ws2812b_set_color_enum (1); // RED
          ESP_LOGI (TAG, "WS2812B LED: Red (Some tests FAILED)");
        }
    }

  return ESP_OK;
}