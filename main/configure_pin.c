#include "configure_pin.h"

#include "driver/gpio.h"
#include "esp_log.h"

const char *TAG1 = "TESTE";

void configure_pin(int pin)
{
    ESP_LOGI(TAG1, "%i configured to blink GPIO LED!", pin);
    gpio_reset_pin(pin);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}