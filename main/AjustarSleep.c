#include "AjustarSleep.h"

#include "esp_timer.h"
#include "esp_log.h"

static const char *TAGTimer = "TIMER";

void timer_callback(void* arg) {
    ESP_LOGI(TAGTimer, "Tempo acabou!");
}

void AjustarSleep(){

    esp_timer_handle_t meu_timer;

    const esp_timer_create_args_t timer_args = {
        .callback = &timer_callback,
        .name = "meu_timer"
    };

    esp_timer_create(&timer_args, &meu_timer);
    esp_timer_start_once(meu_timer, 5000000);
}