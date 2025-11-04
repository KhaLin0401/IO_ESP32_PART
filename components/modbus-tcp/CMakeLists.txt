idf_component_register(
    SRCS "modbus-tcp.c" "modbus-tcp-map.c"
    INCLUDE_DIRS "include"
    REQUIRES esp-modbus main
)