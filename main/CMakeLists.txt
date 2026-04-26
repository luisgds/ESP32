idf_component_register(SRCS "sleep/AjustarSleep.c" 
                        "wifi/configure_wifi.c" 
                        "swing/AjustarSwing.c" 
                        "configure_pin.c" 
                        "blink_example_main.c"
                        "umidade/AjustarUmidade.c" 
                        "AjustarCheiro.c" 
                       INCLUDE_DIRS "."
                                    "sleep"
                                    "wifi"
                                    "swing"
                                    "umidade"
                       REQUIRES esp_driver_ledc 
                                esp_driver_gpio esp_timer 
                                esp_wifi esp_event 
                                esp_netif 
                                lwip 
                                nvs_flash
                        PRIV_REQUIRES spi_flash)
