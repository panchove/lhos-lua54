#ifndef LHOS_POST_H
#define LHOS_POST_H

#include "esp_err.h"
#include <stdbool.h>

typedef struct
{
  bool ram_ok;
  bool flash_ok;
  bool gpio_ok;
  bool psram_ok;
  bool nvs_ok;
  bool wifi_ok;
  bool ble_ok;
} lhos_post_status_t;

/**
 * @brief Perform hardware POST (Power-On Self-Test)
 * @return ESP_OK on success, ESP_FAIL on failure
 */
esp_err_t lhos_post_hardware (lhos_post_status_t *status);

/**
 * @brief Get current hardware status
 * @param status Pointer to status structure to fill
 * @return ESP_OK on success
 */
esp_err_t lhos_post_get_status (lhos_post_status_t *status);

/**
 * @brief Indicate POST status via RGB LED
 * Uses GPIO pins for RGB LED to indicate overall POST status
 * GPIO 2: Red, GPIO 4: Green, GPIO 5: Blue
 * Colors: Green = PASS, Red = FAIL, Blue = In Progress
 * @param status POST status (NULL for in progress)
 * @return ESP_OK on success
 */
esp_err_t lhos_post_led_indicate (lhos_post_status_t *status);

#endif /* LHOS_POST_H */