idf_component_register(SRCS "AjustarSleep.c" "AjustarCheiro.c" "configure_wifi.c" "AjustarSwing.c" "configure_pin.c" "blink_example_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_ledc esp_driver_gpio esp_timer esp_wifi esp_event esp_netif lwip nvs_flash
                        PRIV_REQUIRES spi_flash)
