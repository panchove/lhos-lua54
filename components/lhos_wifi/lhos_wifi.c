#include "lhos_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "LHOS_WIFI";

esp_err_t
lhos_wifi_init (void)
{
  ESP_LOGI (TAG, "Initializing WiFi");
  esp_err_t ret = nvs_flash_init ();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK (nvs_flash_erase ());
      ret = nvs_flash_init ();
    }
  ESP_ERROR_CHECK (ret);

  ESP_ERROR_CHECK (esp_netif_init ());
  ESP_ERROR_CHECK (esp_event_loop_create_default ());
  esp_netif_create_default_wifi_sta ();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT ();
  ESP_ERROR_CHECK (esp_wifi_init (&cfg));

  return ESP_OK;
}

esp_err_t
lhos_wifi_deinit (void)
{
  ESP_LOGI (TAG, "Deinitializing WiFi");
  ESP_ERROR_CHECK (esp_wifi_deinit ());
  ESP_ERROR_CHECK (esp_event_loop_delete_default ());
  ESP_ERROR_CHECK (esp_netif_deinit ());
  return ESP_OK;
}

esp_err_t
lhos_wifi_connect (const char *ssid, const char *password)
{
  ESP_LOGI (TAG, "Connecting to WiFi SSID: %s", ssid);

  wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
  strcpy ((char *)wifi_config.sta.ssid, ssid);
  strcpy ((char *)wifi_config.sta.password, password);

  ESP_ERROR_CHECK (esp_wifi_set_mode (WIFI_MODE_STA));
  ESP_ERROR_CHECK (esp_wifi_set_config (WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK (esp_wifi_start ());
  ESP_ERROR_CHECK (esp_wifi_connect ());

  return ESP_OK;
}

esp_err_t
lhos_wifi_disconnect (void)
{
  ESP_LOGI (TAG, "Disconnecting from WiFi");
  ESP_ERROR_CHECK (esp_wifi_disconnect ());
  ESP_ERROR_CHECK (esp_wifi_stop ());
  return ESP_OK;
}

bool
lhos_wifi_is_connected (void)
{
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info (&ap_info) == ESP_OK)
    {
      return true;
    }
  return false;
}