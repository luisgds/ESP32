#include "configure_pin.h"
#include "AjustarSwing.h"
#include "AjustarSleep.h"
#include "configure_wifi.h"
#include "AjustarUmidade.h" 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>
#include "nvs_flash.h"

static const char *TAG = "MAIN";
static uint8_t s_led_state = 0;

void app_main(void) {
    nvs_flash_init();
    wifi_init();

    // Inicia task de escuta UDP em paralelo
    xTaskCreate(udp_listener_task, "udp_listener", 4096, NULL, 5, NULL);

    // Inicia task de ajuste de umidade
    xTaskCreate((TaskFunction_t)AjustarUmidade, "umidade_task", 4096, NULL, 4, NULL);
    
    // Pinos
    configure_pin(2, GPIO_MODE_OUTPUT);
    AjustarPinSwing(3, GPIO_MODE_OUTPUT, 869);
    while (1) {
        if (deve_dormir) {
            deve_dormir = false;
            ESP_LOGI(TAG, "Sinal recebido! Dormindo por 10 segundos...");
            vTaskDelay(pdMS_TO_TICKS(10000));
            ESP_LOGI(TAG, "Acordou! Voltando ao funcionamento.");
        }

        AjustarSwing();

        ESP_LOGI(TAG, "LED %s!", s_led_state ? "ON" : "OFF");
        s_led_state = !s_led_state;
        gpio_set_level(2, s_led_state);
    }
}

/*

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
    configure_pin(2, GPIO_MODE_OUTPUT);     // Botão de ligar o ar condicionado
    ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
    AjustarPinSwing(3, GPIO_MODE_OUTPUT, 869);
    //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");

    while (1) {
        AjustarSwing();
        //xTaskCreate(&AjustarSwing, "AjustarSwing", 2048, NULL, 5, NULL);



        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");

        s_led_state = !s_led_state;
        gpio_set_level(2, s_led_state);

        gpio_set_level(2, s_led_state);
    }
}


*/