idf_component_register(SRCS "lhos_post.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_common esp_hw_support driver spi_flash esp_wifi nvs_flash esp_psram lhos_ws2812b)