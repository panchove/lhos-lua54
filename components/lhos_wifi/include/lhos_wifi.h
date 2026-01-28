/* Lightweight WiFi helper for LHOS
 */
#ifndef LHOS_WIFI_H
#define LHOS_WIFI_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t lhos_wifi_init (void);
esp_err_t lhos_wifi_connect (const char *ssid, const char *pwd);
esp_err_t lhos_wifi_disconnect (void);
bool lhos_wifi_is_connected (void);

#endif /* LHOS_WIFI_H */
