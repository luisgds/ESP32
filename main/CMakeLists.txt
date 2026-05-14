idf_component_register(SRCS "sleep/AjustarSleep.c" 
                        "wifi/configure_wifi.c" 
                        "swing/AjustarSwing.c" 
                        "configure_pin/configure_pin.c" 
                        "main.c"
                        "umidade/AjustarUmidade.c" 
                        "AjustarCheiro/AjustarCheiro.c" 
                        #"ir/ir_controle.c"
                        "componentes/buzzer.c"
                        "componentes/rgb_led.c"
                       INCLUDE_DIRS "."
                                    "sleep"
                                    "wifi"
                                    "swing"
                                    "umidade"
                                    #"ir"
                                    "componentes"
                                    "configure_pin"
                                    "AjustarCheiro"
                       REQUIRES esp_driver_ledc 
                                esp_driver_gpio esp_timer 
                                esp_wifi esp_event 
                                esp_netif 
                                lwip 
                                nvs_flash
                                #esp_driver_rmt
                        PRIV_REQUIRES spi_flash)
