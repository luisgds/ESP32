#pragma once

#include <stdbool.h>

extern bool deve_dormir;

void wifi_init(void);
void udp_listener_task(void *pvParameters);