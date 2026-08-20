#pragma once
#include <stdbool.h>
#include <stdint.h>

extern uint8_t deve_dormir;
extern bool    swing;
extern bool    buzzer_ativo;

void mqtt_init(void);