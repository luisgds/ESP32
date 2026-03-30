#include "AjustarSwing.h"

#include "driver/gpio.h"
#include "configure_pin.h"
#include "driver/ledc.h"
#include "esp_log.h"

const char *TAG1 = "AjustarSwing";

void AjustarSwing(gpio_num_t pin, gpio_mode_t direction) {
    configure_pin(pin, direction); // Configura o pino especificado como saída
    ESP_LOGI(TAG1, "Swing ajustado para o pino %i", pin);
    
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 5000,  // Frequência de 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
}