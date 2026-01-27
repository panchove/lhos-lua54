idf_component_register(SRCS "lhos_wifi.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi nvs_flash)