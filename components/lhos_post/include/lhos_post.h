/* LHOS POST component - provides a basic Power On Self Test status structure
 */
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

esp_err_t lhos_post_get_status (lhos_post_status_t *status);
esp_err_t lhos_post_hardware (lhos_post_status_t *status);
esp_err_t lhos_post_led_indicate (lhos_post_status_t *status);

#endif /* LHOS_POST_H */
