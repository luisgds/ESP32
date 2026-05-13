#include "buzzer.h"
#include "configure_pin.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define FREQ_BUZZER 2500  // 2.5kHz — frequência audível boa

void AjustarPinBuzzer(int pino_buzzer, int direction) {
    configure_pin(pino_buzzer, direction);
    // configura timer PWM
    ledc_timer_config_t timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num       = LEDC_TIMER_1,  // timer diferente do servo
        .freq_hz         = FREQ_BUZZER,
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // configura canal PWM
    ledc_channel_config_t canal = {
        .gpio_num   = pino_buzzer,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_2,  // canal diferente do servo
        .timer_sel  = LEDC_TIMER_1,
        .duty       = 0,  // começa desligado
        .hpoint     = 0
    };
    ledc_channel_config(&canal);
}

void buzzer_ligar(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 512); // 50% duty
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

void buzzer_desligar(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}

void tocar_buzzer(void) {
    for (int i = 0; i < 3; i++) {
        buzzer_ligar();
        vTaskDelay(pdMS_TO_TICKS(100));
        buzzer_desligar();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}