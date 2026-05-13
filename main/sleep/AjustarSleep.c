#include "AjustarSleep.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG_SLEEP = "SLEEP";

// tempos em microsegundos
#define TEMPO_4H  (4ULL  * 60 * 60 * 1000000)
#define TEMPO_8H  (8ULL  * 60 * 60 * 1000000)
#define TEMPO_12H (12ULL * 60 * 60 * 1000000)

static esp_timer_handle_t sleep_timer = NULL;
static int64_t            tempo_inicio = 0;
static int64_t            tempo_total  = 0;

bool sleep_acabou = false; // ← main verifica

static void sleep_callback(void *arg) {
    sleep_acabou = true;
    ESP_LOGI(TAG_SLEEP, "Timer acabou! Desligando AR...");
}

void sleep_iniciar(SleepMode modo) {
    // cancela timer anterior se existir
    sleep_cancelar();

    uint64_t tempo_us = 0;
    const char *nome  = "";

    switch (modo) {
        case SLEEP_4H:  tempo_us = TEMPO_4H;  nome = "4 horas";  break;
        case SLEEP_8H:  tempo_us = TEMPO_8H;  nome = "8 horas";  break;
        case SLEEP_12H: tempo_us = TEMPO_12H; nome = "12 horas"; break;
        default: return;
    }

    const esp_timer_create_args_t timer_args = {
        .callback = &sleep_callback,
        .name     = "sleep_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &sleep_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(sleep_timer, tempo_us));

    tempo_inicio = esp_timer_get_time();
    tempo_total  = (int64_t)tempo_us;

    ESP_LOGI(TAG_SLEEP, "Timer iniciado: %s", nome);
}

void sleep_cancelar(void) {
    if (sleep_timer != NULL) {
        esp_timer_stop(sleep_timer);
        esp_timer_delete(sleep_timer);
        sleep_timer  = NULL;
        tempo_inicio = 0;
        tempo_total  = 0;
        ESP_LOGI(TAG_SLEEP, "Timer cancelado.");
    }
}

bool sleep_ativo(void) {
    return sleep_timer != NULL;
}

int sleep_tempo_restante_min(void) {
    if (!sleep_ativo()) return 0;
    int64_t passado   = esp_timer_get_time() - tempo_inicio;
    int64_t restante  = tempo_total - passado;
    return (int)(restante / 60000000);
}