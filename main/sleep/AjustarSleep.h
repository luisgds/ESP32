#pragma once
#include <stdbool.h>

typedef enum {
    SLEEP_4H  = 0,
    SLEEP_8H  = 1,
    SLEEP_12H = 2,
    SLEEP_OFF = 3
} SleepMode;

extern bool sleep_acabou; // ← main verifica isso

void sleep_iniciar(SleepMode modo);
void sleep_cancelar(void);
bool sleep_ativo(void);
int  sleep_tempo_restante_min(void);
