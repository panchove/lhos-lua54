idf_component_register(SRCS "lhos_ntp.c"
                       INCLUDE_DIRS "include"
                       REQUIRES lwip
                       PRIV_REQUIRES lua54 lhos_lua)
