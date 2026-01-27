#ifndef LHOS_WS2812B_H
#define LHOS_WS2812B_H

#include "esp_err.h"

// WS2812B LED configuration
#define WS2812B_LED_COUNT 1 // Single LED
#define WS2812B_GPIO_PIN 48 // GPIO pin for data (using LHOS_GPIO_LED)

// Color structure for WS2812B (GRB order)
typedef struct
{
  uint8_t g;
  uint8_t r;
  uint8_t b;
} ws2812b_color_t;

// Initialize WS2812B LED
esp_err_t lhos_ws2812b_init (void);

// Set color for the LED
esp_err_t lhos_ws2812b_set_color (uint8_t r, uint8_t g, uint8_t b);

// Turn off the LED
esp_err_t lhos_ws2812b_off (void);

// Set predefined color
esp_err_t lhos_ws2812b_set_color_enum (int color_index);

// Flash the LED a number of times
esp_err_t lhos_ws2812b_flash (uint8_t flashes, uint32_t on_ms,
                              uint32_t off_ms);

#endif // LHOS_WS2812B_H