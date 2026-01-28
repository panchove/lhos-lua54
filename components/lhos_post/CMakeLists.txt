idf_component_register(SRCS "lhos_post.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_system esp_driver_gpio spi_flash esp_psram nvs_flash lhos_ws2812b esp_wifi)