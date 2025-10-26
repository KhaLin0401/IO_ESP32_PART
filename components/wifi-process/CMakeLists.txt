idf_component_register(
    SRCS "wifi-process.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif esp32-wifi-manager
)