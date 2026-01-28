idf_component_register(SRCS "lhos_net.c"
                       INCLUDE_DIRS "include"
                       REQUIRES lwip log
                       PRIV_REQUIRES lua54)
