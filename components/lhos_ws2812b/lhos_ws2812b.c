#include "lhos_ws2812b.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "LHOS_WS2812B";

// RMT configuration for WS2812B
#define RMT_RESOLUTION_HZ 40000000 // 40MHz resolution

// WS2812B timing (in RMT ticks, 40MHz clock)
#define WS2812B_T0H 14      // 0.35us
#define WS2812B_T0L 26      // 0.8us
#define WS2812B_T1H 26      // 0.7us
#define WS2812B_T1L 14      // 0.6us
#define WS2812B_RESET 30000 // 300us reset

static rmt_channel_handle_t tx_channel = NULL;
static rmt_encoder_handle_t copy_encoder = NULL;
/* Buffer for RMT symbols: 24 bits per LED plus one reset symbol. */
static rmt_symbol_word_t led_data[24 * WS2812B_LED_COUNT + 1];

esp_err_t
lhos_ws2812b_init (void)
{
  ESP_LOGI (TAG, "Initializing WS2812B on GPIO %d", WS2812B_GPIO_PIN);

  rmt_tx_channel_config_t tx_config = {
    .gpio_num = WS2812B_GPIO_PIN,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = RMT_RESOLUTION_HZ,
    .mem_block_symbols = 256,
    .trans_queue_depth = 1,
  };

  ESP_ERROR_CHECK (rmt_new_tx_channel (&tx_config, &tx_channel));
  ESP_LOGI (TAG, "RMT TX channel created\n");

  /* Use dedicated WS2812 encoder if available */
#ifdef CONFIG_RMT_WS2812_ENCODER
  ESP_LOGI (TAG, "Creating WS2812 encoder");
  ESP_ERROR_CHECK (rmt_new_ws2812_encoder (NULL, &copy_encoder));
  ESP_LOGI (TAG, "RMT WS2812 encoder created\n");
#else
  rmt_copy_encoder_config_t copy_encoder_config = {};
  ESP_ERROR_CHECK (rmt_new_copy_encoder (&copy_encoder_config, &copy_encoder));
  ESP_LOGI (TAG, "RMT copy encoder created\n");
#endif

  // Initialize to off
  ESP_LOGI (TAG, "Setting initial LED OFF");
  return lhos_ws2812b_off ();
}

esp_err_t
lhos_ws2812b_set_color (uint32_t rgb24)
{
  // Unpack RGB24 (0xRRGGBB) and convert to GRB order used by WS2812B
  uint8_t r = (uint8_t)((rgb24 >> 16) & 0xFF);
  uint8_t g = (uint8_t)((rgb24 >> 8) & 0xFF);
  uint8_t b = (uint8_t)(rgb24 & 0xFF);
  uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;

  /* Convert 24-bit GRB to RMT symbols for each LED (supports multiple LEDs)
   * We populate 24 symbols per LED and then a single reset symbol. */
  for (int led = 0; led < WS2812B_LED_COUNT; ++led)
    {
      int base = led * 24;
      for (int i = 0; i < 24; i++)
        {
          if (grb & (1 << (23 - i)))
            {
              /* Bit 1: T1H + T1L */
              led_data[base + i].level0 = 1;
              led_data[base + i].duration0 = WS2812B_T1H;
              led_data[base + i].level1 = 0;
              led_data[base + i].duration1 = WS2812B_T1L;
            }
          else
            {
              /* Bit 0: T0H + T0L */
              led_data[base + i].level0 = 1;
              led_data[base + i].duration0 = WS2812B_T0H;
              led_data[base + i].level1 = 0;
              led_data[base + i].duration1 = WS2812B_T0L;
            }
        }
    }

  /* Reset pulse after all LEDs */
  size_t reset_index = 24 * WS2812B_LED_COUNT;
  led_data[reset_index].level0 = 0;
  led_data[reset_index].duration0 = WS2812B_RESET;
  led_data[reset_index].level1 = 0;
  led_data[reset_index].duration1 = 0;

  rmt_transmit_config_t transmit_config = {
    .loop_count = 0,
  };

  /* Only transmit the symbols we populated: 24 symbols per LED + 1 reset */
  size_t symbol_count = 24 * WS2812B_LED_COUNT + 1;
  ESP_LOGI (TAG, "Transmitting %d RMT symbols", (int)symbol_count);
  esp_err_t tx_err = ESP_OK;
  /* Retry transmit a few times if we get timeouts */
  for (int attempt = 0; attempt < 3; ++attempt)
    {
      tx_err = rmt_transmit (tx_channel, copy_encoder, led_data, symbol_count,
                             &transmit_config);
      if (tx_err != ESP_OK)
        {
          ESP_LOGW (TAG, "rmt_transmit attempt %d returned %s", attempt,
                    esp_err_to_name (tx_err));
          vTaskDelay (pdMS_TO_TICKS (50));
          continue;
        }

      TickType_t wait_ticks = pdMS_TO_TICKS (2000);
      esp_err_t wr = rmt_tx_wait_all_done (tx_channel, wait_ticks);
      if (wr == ESP_OK)
        {
          ESP_LOGI (TAG, "RMT transmit completed on attempt %d", attempt + 1);
          break;
        }
      ESP_LOGW (TAG, "rmt_tx_wait_all_done on attempt %d returned %s", attempt,
                esp_err_to_name (wr));
      /* allow a short pause before retrying */
      vTaskDelay (pdMS_TO_TICKS (100));
    }

  if (tx_err != ESP_OK)
    {
      ESP_LOGW (TAG, "rmt_transmit failed after retries: %s",
                esp_err_to_name (tx_err));
      return tx_err;
    }

  TickType_t wait_ticks = pdMS_TO_TICKS (2000);
  esp_err_t wr = rmt_tx_wait_all_done (tx_channel, wait_ticks);
  if (wr != ESP_OK)
    {
      ESP_LOGW (TAG, "rmt_tx_wait_all_done timeout or error: %s",
                esp_err_to_name (wr));
    }
  else
    {
      ESP_LOGI (TAG, "RMT transmit completed");
    }

  ESP_LOGI (TAG, "WS2812B set to R:%d G:%d B:%d", r, g, b);

  return ESP_OK;
}

esp_err_t
lhos_ws2812b_off (void)
{
  return lhos_ws2812b_set_color (0x000000);
}

esp_err_t
lhos_ws2812b_set_color_enum (int color_index)
{
  // Map color index to RGB (same as RGB LED table but inverted for common
  // anode)
  uint8_t r, g, b;
  switch (color_index)
    {
    case 0:
      r = 0;
      g = 0;
      b = 0;
      break; // OFF
    case 1:
      r = 255;
      g = 0;
      b = 0;
      break; // RED
    case 2:
      r = 0;
      g = 255;
      b = 0;
      break; // GREEN
    case 3:
      r = 0;
      g = 0;
      b = 255;
      break; // BLUE
    case 4:
      r = 255;
      g = 255;
      b = 255;
      break; // WHITE
    case 5:
      r = 255;
      g = 255;
      b = 0;
      break; // YELLOW
    case 6:
      r = 0;
      g = 255;
      b = 255;
      break; // CYAN
    case 7:
      r = 255;
      g = 0;
      b = 255;
      break; // MAGENTA
    default:
      r = 0;
      g = 0;
      b = 0;
      break;
    }
  uint32_t rgb = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
  return lhos_ws2812b_set_color (rgb);
}

esp_err_t
lhos_ws2812b_flash (uint8_t flashes, uint32_t on_ms, uint32_t off_ms)
{
  for (uint8_t i = 0; i < flashes; i++)
    {
      // Turn on red
      lhos_ws2812b_set_color (0xFF0000);
      vTaskDelay (pdMS_TO_TICKS (on_ms));
      // Turn off
      lhos_ws2812b_off ();
      vTaskDelay (pdMS_TO_TICKS (off_ms));
    }
  return ESP_OK;
}

esp_err_t
lhos_ws2812b_test_full (void)
{
  ESP_LOGI (TAG, "WS2812B: Starting full diagnostic test");
  const uint32_t colors[] = { 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF,
                              0xFFFF00, 0xFF00FF, 0x00FFFF, 0x000000 };
  const char *names[] = { "RED",    "GREEN",   "BLUE", "WHITE",
                          "YELLOW", "MAGENTA", "CYAN", "OFF" };

  for (size_t i = 0; i < sizeof (colors) / sizeof (colors[0]); ++i)
    {
      ESP_LOGI (TAG, "Test %d: %s (0x%06X)", (int)i + 1, names[i], colors[i]);
      esp_err_t err = lhos_ws2812b_set_color (colors[i]);
      ESP_LOGI (TAG, "set_color returned: %s", esp_err_to_name (err));
      vTaskDelay (pdMS_TO_TICKS (800));
    }

  ESP_LOGI (TAG, "WS2812B: Running flash pattern (3x)");
  esp_err_t ferr = lhos_ws2812b_flash (3, 300, 200);
  ESP_LOGI (TAG, "flash returned: %s", esp_err_to_name (ferr));

  /* Final off */
  lhos_ws2812b_off ();
  ESP_LOGI (TAG, "WS2812B: Full diagnostic test complete");
  return ESP_OK;
}

esp_err_t
lhos_ws2812b_selftest (void)
{
  ESP_LOGI (TAG, "WS2812B: Running self-test checks");

  if (tx_channel == NULL)
    {
      ESP_LOGE (TAG, "RMT TX channel not initialized");
      return ESP_ERR_INVALID_STATE;
    }

  if (copy_encoder == NULL)
    {
      ESP_LOGE (TAG, "RMT encoder not initialized");
      return ESP_ERR_INVALID_STATE;
    }

  ESP_LOGI (TAG, "Configured GPIO=%d, LED_COUNT=%d", WS2812B_GPIO_PIN,
            WS2812B_LED_COUNT);

  esp_err_t res = lhos_ws2812b_test_full ();
  if (res != ESP_OK)
    {
      ESP_LOGE (TAG, "Full diagnostic test failed: %s", esp_err_to_name (res));
      return res;
    }

  ESP_LOGI (TAG, "WS2812B: Self-test passed");
  return ESP_OK;
}