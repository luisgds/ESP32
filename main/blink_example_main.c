#include "configure_pin.h"
#include "AjustarSwing.h"
#include "AjustarSleep.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "LED Ligar/Desligar";

static uint8_t s_led_state = 0;

void app_main(void)
{
    /* Configurando pinos inicialmente */
    configure_pin(2, GPIO_MODE_OUTPUT);     // Botão de ligar o ar condicionado
    ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    AjustarPinSwing(3, GPIO_MODE_OUTPUT, 869);
    //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");

    /* loop de funcionamento*/
    while (1) {
        /*
        Ajustar swing do ar condicionado
        */
        AjustarSwing();
        //xTaskCreate(&AjustarSwing, "AjustarSwing", 2048, NULL, 5, NULL);


        /*
        Desligar ou ligar o ar condicionado (No uc é um "ultra" sleep)
        */

       /* Toggle the LED state */
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");

        s_led_state = !s_led_state;
        gpio_set_level(2, s_led_state);
        /*
        
        */
        gpio_set_level(2, s_led_state);
    }
}
