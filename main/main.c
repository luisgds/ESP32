#include "configure_pin.h"
#include "AjustarSwing.h"
#include "AjustarSleep.h"
#include "configure_wifi.h"
#include "AjustarUmidade.h"
//#include "ir_controle.h"
#include "buzzer.h"
#include "rgb_led.h"

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
    gpio_set_level(2, 1); // Liga LED verde para indicar que o sistema iniciou
    rgb_led_init();
    AjustarPinSwing(3, GPIO_MODE_OUTPUT);
    AjustarPinUmidade(5, GPIO_MODE_OUTPUT);
    AjustarPinBuzzer(6, GPIO_MODE_OUTPUT);
    while (1) {
        if (deve_dormir != 0 && !sleep_ativo()) { // ← só configura se não tiver ativo
            switch (deve_dormir) {
                case 1: sleep_iniciar(SLEEP_4H);  break;
                case 2: sleep_iniciar(SLEEP_8H);  break;
                case 3: sleep_iniciar(SLEEP_12H); break;
            }
            ESP_LOGI(TAG, "Timer configurado!");
            rgb_azul();
            if (buzzer_ativo) tocar_buzzer();
        }

        // ── cancela sleep ──
        if (deve_dormir == 0 && sleep_ativo()) {
            sleep_cancelar();
            rgb_vermelho();
            ESP_LOGI(TAG, "Timer cancelado!");
            //rgb_verde();
        }

        // ── timer acabou ──
        if (sleep_acabou) {
            sleep_acabou = false;
            ESP_LOGI(TAG, "Timer acabou! Desligando AR...");
            rgb_vermelho();
            if (buzzer_ativo) tocar_buzzer();
            gpio_set_level(2, 0); // Desliga LED verde para indicar que o timer acabou
        }

        // ── log tempo restante ──
        if (sleep_ativo()) {
            ESP_LOGI(TAG, "Sleep: %d minutos restantes",
                    sleep_tempo_restante_min());
        }

        //if (ligado) gpio_set_level(10, 1); // Liga LED para indicar que o sistema iniciou
        if (swing) {
            AjustarSwing();
            if (buzzer_ativo) tocar_buzzer();
            rgb_verde();
        }
        
       vTaskDelay(pdMS_TO_TICKS(1000)); // verifica 1x por segundo
    }
}
