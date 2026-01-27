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
static rmt_symbol_word_t
    led_data[24 * WS2812B_LED_COUNT + 1]; // 24 bits per LED + reset

esp_err_t
lhos_ws2812b_init (void)
{
  ESP_LOGI (TAG, "Initializing WS2812B on GPIO %d", WS2812B_GPIO_PIN);

  rmt_tx_channel_config_t tx_config = {
    .gpio_num = WS2812B_GPIO_PIN,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = RMT_RESOLUTION_HZ,
    .mem_block_symbols = 64,
    .trans_queue_depth = 4,
  };

  ESP_ERROR_CHECK (rmt_new_tx_channel (&tx_config, &tx_channel));

  rmt_copy_encoder_config_t copy_encoder_config = {};
  ESP_ERROR_CHECK (rmt_new_copy_encoder (&copy_encoder_config, &copy_encoder));

  // Initialize to off
  return lhos_ws2812b_off ();
}

esp_err_t
lhos_ws2812b_set_color (uint8_t r, uint8_t g, uint8_t b)
{
  // WS2812B uses GRB order
  uint32_t grb = (g << 16) | (r << 8) | b;

  // Convert 24-bit GRB to RMT symbols
  for (int i = 0; i < 24; i++)
    {
      if (grb & (1 << (23 - i)))
        {
          // Bit 1: T1H + T1L
          led_data[i].level0 = 1;
          led_data[i].duration0 = WS2812B_T1H;
          led_data[i].level1 = 0;
          led_data[i].duration1 = WS2812B_T1L;
        }
      else
        {
          // Bit 0: T0H + T0L
          led_data[i].level0 = 1;
          led_data[i].duration0 = WS2812B_T0H;
          led_data[i].level1 = 0;
          led_data[i].duration1 = WS2812B_T0L;
        }
    }

  // Reset pulse
  led_data[24].level0 = 0;
  led_data[24].duration0 = WS2812B_RESET;
  led_data[24].level1 = 0;
  led_data[24].duration1 = 0;

  rmt_transmit_config_t transmit_config = {
    .loop_count = 0,
  };

  ESP_ERROR_CHECK (rmt_transmit (tx_channel, copy_encoder, led_data,
                                 sizeof (led_data), &transmit_config));
  ESP_ERROR_CHECK (rmt_tx_wait_all_done (tx_channel, -1));
  ESP_LOGI (TAG, "WS2812B set to R:%d G:%d B:%d", r, g, b);

  return ESP_OK;
}

esp_err_t
lhos_ws2812b_off (void)
{
  return lhos_ws2812b_set_color (0, 0, 0);
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
  return lhos_ws2812b_set_color (r, g, b);
}

esp_err_t
lhos_ws2812b_flash (uint8_t flashes, uint32_t on_ms, uint32_t off_ms)
{
  for (uint8_t i = 0; i < flashes; i++)
    {
      // Turn on red
      lhos_ws2812b_set_color (255, 0, 0);
      vTaskDelay (pdMS_TO_TICKS (on_ms));
      // Turn off
      lhos_ws2812b_off ();
      vTaskDelay (pdMS_TO_TICKS (off_ms));
    }
  return ESP_OK;
}