#include "AjustarUmidade.h"
#include "configure_pin.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdlib.h> // rand()

static const char *TAG_UMIDADE = "UMIDADE";

// ── Pinos ──
#define PINO_SERVO 5

#define UMIDADE_MIN 40
#define UMIDADE_MAX 60

// Duty do servo (igual ao AjustarSwing)
#define SERVO_ABERTO  1638  // posição aberto
#define SERVO_FECHADO 869   // posição fechado

static bool umidificador_ligado = false;

// ── Simula leitura de sensor ──
// Quando tiver o sensor DHT11/DHT22 real, substitui essa função
// Simulado:
static int ler_umidade_simulada(void) {
    return 20 + (rand() % 60);
}

static int ler_temperatura_simulada(void) {
    return 10 + (rand() % 50);
}
/*
// Real (com biblioteca DHT):
static int ler_umidade_real(void) {
    float umidade, temperatura;
    dht_read_float_data(DHT_TYPE_DHT22, PINO_SENSOR, &umidade, &temperatura);
    return (int)umidade;
}
*/

static void ligar_umidificador(void) {
    ESP_LOGI(TAG_UMIDADE, "Umidade baixa! Ligando umidificador...");

    // Servo abre
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, SERVO_ABERTO);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    umidificador_ligado = true;
}

static void desligar_umidificador(void) {
    ESP_LOGI(TAG_UMIDADE, "Umidade ok! Desligando umidificador...");

    // Servo fecha
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, SERVO_FECHADO);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    umidificador_ligado = false;
}

void AjustarPinUmidade(int pin, int direction) {
    configure_pin(pin, direction); // Configura o pino especificado como saída

    // Configura servo no canal 1 (canal 0 já é usado pelo AjustarSwing)
    ledc_channel_config_t ledc_channel = {
        .gpio_num   = PINO_SERVO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL_1,  // ← canal diferente do swing!
        .timer_sel  = LEDC_TIMER_0,    // mesmo timer tá ok
        .duty       = SERVO_FECHADO,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ESP_LOGI(TAG_UMIDADE, "Pino de umidade configurado: %i", pin);
}

void AjustarUmidade(void) {
    ESP_LOGI(TAG_UMIDADE, "Monitorando umidade...");

    while (1) {
        int umidade = ler_umidade_simulada();
        ESP_LOGI(TAG_UMIDADE, "Umidade atual: %d%%", umidade);
        if (umidade < UMIDADE_MIN && !umidificador_ligado) {
            ligar_umidificador();
        } else if (umidade > UMIDADE_MAX && umidificador_ligado) {
            desligar_umidificador();
        } else {
            ESP_LOGI(TAG_UMIDADE, "Umidade estavel, mantendo estado.");
        }

        vTaskDelay(pdMS_TO_TICKS(3000)); // verifica a cada 3 segundos
    }
}