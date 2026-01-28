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

// Set color for the LED using packed RGB24 (0xRRGGBB)
esp_err_t lhos_ws2812b_set_color (uint32_t rgb24);

// Turn off the LED
esp_err_t lhos_ws2812b_off (void);

// Set predefined color
esp_err_t lhos_ws2812b_set_color_enum (int color_index);

// Flash the LED a number of times
esp_err_t lhos_ws2812b_flash (uint8_t flashes, uint32_t on_ms,
                              uint32_t off_ms);

// Run full diagnostics: cycle colors, flash patterns, and log RMT results
esp_err_t lhos_ws2812b_test_full (void);

// Run a self-test that validates configuration and runs the full diagnostic
// Returns ESP_OK on success or an esp_err_t on failure
esp_err_t lhos_ws2812b_selftest (void);

#endif // LHOS_WS2812B_H