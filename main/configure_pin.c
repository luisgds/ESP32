#include "configure_pin.h"

#include "driver/gpio.h"
#include "esp_log.h"

const char *TAG1 = "ConfigurePin";

void configure_pin(int pin, int direction) {
    ESP_LOGI(TAG1, "%i configured to blink GPIO LED!", pin);
    gpio_reset_pin(pin);
    gpio_set_direction(pin, direction);
}