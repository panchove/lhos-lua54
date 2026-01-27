idf_component_register(SRCS "main.c" INCLUDE_DIRS "." REQUIRES littlefs vfs lhos_lua lhos_config lhos_post lhos_ws2812b esp_adc driver)

littlefs_create_partition_image(main lua_fs)


