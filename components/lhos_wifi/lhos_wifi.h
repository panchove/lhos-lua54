#ifndef LHOS_WIFI_H
#define LHOS_WIFI_H

#include "esp_err.h"
#include <stdbool.h>

// Initialize WiFi
esp_err_t lhos_wifi_init (void);

// Deinitialize WiFi
esp_err_t lhos_wifi_deinit (void);

// Connect to WiFi AP
esp_err_t lhos_wifi_connect (const char *ssid, const char *password);

// Disconnect from WiFi
esp_err_t lhos_wifi_disconnect (void);

// Get connection status
bool lhos_wifi_is_connected (void);

#endif // LHOS_WIFI_H