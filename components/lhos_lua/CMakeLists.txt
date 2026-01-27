idf_component_register(SRCS "lhos.c" "lhos_lua.c" INCLUDE_DIRS "." REQUIRES lua54 esp_timer lhos_post driver esp_wifi esp_netif esp_event nvs_flash)
