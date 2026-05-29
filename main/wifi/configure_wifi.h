#pragma once

#include <stdbool.h>
#include <stdint.h>

extern uint8_t deve_dormir;
extern bool swing;
extern bool buzzer_ativo;

void wifi_init(void);
void udp_listener_task(void *pvParameters);
