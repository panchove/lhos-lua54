idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES littlefs vfs lhos_lua lhos_config lhos_ws2812b lhos_post esp_adc nvs_flash driver)

littlefs_create_partition_image(main lua_fs)


