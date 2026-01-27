#ifndef LHOS_LED_H
#define LHOS_LED_H

#include "esp_err.h"
#include "lhos_config.h"

// Use color enum from lhos_config

#define LHOS_LED_COLOR_COUNT 8

// Initialize LED (RGB on GPIOs 2,4,5 for common anode)
esp_err_t lhos_led_init (void);

// Set LED color
esp_err_t lhos_led_set_color (lhos_led_color_t color);

// Turn off LED
esp_err_t lhos_led_off (void);

#endif // LHOS_LED_H