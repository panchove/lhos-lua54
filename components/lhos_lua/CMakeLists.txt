idf_component_register(
    SRCS
        "lhos.c"
        "lhos_lua.c"
        "lhos_lua_ble.c"
        "lhos_lua_posix.c"
        "lhos_lua_led.c"
    INCLUDE_DIRS "."
    REQUIRES log lua54 lhos_ble lhos_ws2812b)
