idf_component_register(SRCS "AjustarSleep.cpp" "AjustarCheiro.cpp" "AjustarSwing.c" "configure_pin.c" "blink_example_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_ledc esp_driver_gpio esp_timer
                        PRIV_REQUIRES spi_flash)
