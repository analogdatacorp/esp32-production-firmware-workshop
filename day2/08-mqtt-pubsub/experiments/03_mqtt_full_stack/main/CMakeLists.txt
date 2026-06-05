idf_component_register(
    SRCS "main.c"
    PRIV_REQUIRES
        esp_driver_gpio
        esp_event
        esp_netif
        esp_wifi
        mqtt
        nvs_flash
    INCLUDE_DIRS "."
)
