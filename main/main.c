#include "configure_pin.h"
#include "AjustarSwing.h"
#include "AjustarSleep.h"
#include "configure_wifi.h"
#include "AjustarUmidade.h"
#include "ble_provisioning.h"
//#include "ir_controle.h"
#include "buzzer.h"
#include "rgb_led.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h" 
#include "esp_bt.h"
#include "driver/gpio.h"
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "MAIN";
//static uint8_t s_led_state = 0;

void app_main(void) {
    nvs_flash_init();
    esp_netif_init();
    rgb_led_init();
    esp_event_loop_create_default();

    ble_prov_iniciar();

    if (ble_prov_credenciais_salvas()) {
        ble_prov_conectar_wifi(); // usa NVS
        rgb_verde();
    } else {
        // primeira vez — sem credenciais
        ESP_LOGW("MAIN", "Sem WiFi configurado!");
        ESP_LOGW("MAIN", "Abra nRF Connect e procure 'ESP32-AC'");
        rgb_amarelo();
    }

    //wifi_init();

    // Inicia task de escuta UDP em paralelo
    xTaskCreate(udp_listener_task, "udp_listener", 4096, NULL, 5, NULL);

    // Inicia task de ajuste de umidade
    xTaskCreate((TaskFunction_t)AjustarUmidade, "umidade_task", 4096, NULL, 4, NULL);

    // Pinos
    configure_pin(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 1); // Liga LED verde para indicar que o sistema iniciou
    
    AjustarPinSwing(3, GPIO_MODE_OUTPUT);
    AjustarPinUmidade(5, GPIO_MODE_OUTPUT);
    AjustarPinBuzzer(6, GPIO_MODE_OUTPUT);
    while (1) {

        // ── pausa se estiver reconectando WiFi via BLE ──
        if (ble_prov_reconectando()) {
            rgb_azul(); // indica reconectando
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;   // pula o resto do loop
        }
        // ── configura sleep ──
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
