#include "AjustarSwing.h"
#include "configure_pin.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_err.h"
#include <stdbool.h>

#define SERVO_ABERTO  1638  // posição aberto
#define SERVO_FECHADO 869   // posição fechado

static const char *TAGswing = "AjustarSwing";

void AjustarPinSwing(int pin, int direction){
    configure_pin(pin, direction); // Configura o pino especificado como saída
    ESP_LOGI(TAGswing, "Swing ajustado para o pino %i", pin);

    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_14_BIT,
        .freq_hz         = 50, 
        .clk_cfg         = LEDC_AUTO_CLK
    };

    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num   = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_0,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = SERVO_FECHADO,
        .hpoint     = 0
    };

    ledc_channel_config(&ledc_channel);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_FECHADO);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(500)); // espera chegar na posição
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
}

void AjustarSwing() {
    int duty = SERVO_FECHADO; // 1638 maximo
    int step = 7;
    int total_cycles = 234;
    bool pos_direction = true;
    int i;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, SERVO_FECHADO);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(200));

    for (i=0; i<total_cycles; i++) {
        if (pos_direction) {
            duty += step;
            if (duty >= SERVO_ABERTO) {
                duty = SERVO_ABERTO;
                pos_direction = false;
            }
        } else {
            duty -= step;
            if (duty <= SERVO_FECHADO) {
                duty = SERVO_FECHADO;
                pos_direction = true;
            }
        }
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay(pdMS_TO_TICKS(10)); // Aguarda o tempo definido para a próxima iteração
    }
    vTaskDelay(pdMS_TO_TICKS(500)); // espera chegar na posição final
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
}