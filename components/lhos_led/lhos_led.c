#include "lhos_led.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LHOS_LED";

// GPIO pins for RGB LED (common anode)
#define LED_R_GPIO 2
#define LED_G_GPIO 4
#define LED_B_GPIO 5

esp_err_t
lhos_led_init (void)
{
  ESP_LOGI (TAG, "Initializing RGB LED on GPIOs %d(R), %d(G), %d(B)", 2, 4, 5);

  gpio_config_t io_conf
      = { .pin_bit_mask = (1ULL << 2) | (1ULL << 4) | (1ULL << 5),
          .mode = GPIO_MODE_OUTPUT,
          .pull_up_en = GPIO_PULLUP_DISABLE,
          .pull_down_en = GPIO_PULLDOWN_DISABLE,
          .intr_type = GPIO_INTR_DISABLE };
  ESP_ERROR_CHECK (gpio_config (&io_conf));

  // For common anode, set high to turn off
  ESP_ERROR_CHECK (gpio_set_level (2, 1));
  ESP_ERROR_CHECK (gpio_set_level (4, 1));
  ESP_ERROR_CHECK (gpio_set_level (5, 1));

  return ESP_OK;
}

esp_err_t
lhos_led_set_color (lhos_led_color_t color)
{
  if (color >= LHOS_LED_COLOR_COUNT)
    {
      ESP_LOGE (TAG, "Invalid color: %d", color);
      return ESP_ERR_INVALID_ARG;
    }

  const lhos_led_rgb_t *rgb = &lhos_led_color_table[color];

  ESP_ERROR_CHECK (gpio_set_level (2, rgb->r));
  ESP_ERROR_CHECK (gpio_set_level (4, rgb->g));
  ESP_ERROR_CHECK (gpio_set_level (5, rgb->b));

  ESP_LOGI (TAG, "LED set to color: %d (R:%d G:%d B:%d)", color, rgb->r,
            rgb->g, rgb->b);
  return ESP_OK;
}

esp_err_t
lhos_led_off (void)
{
  return lhos_led_set_color (LHOS_LED_OFF);
}