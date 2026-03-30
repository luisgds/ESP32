#include "configure_pin.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "example";

static uint8_t s_led_state = 0;

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_pin(2);

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        gpio_set_level(2, s_led_state);
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    }
}
