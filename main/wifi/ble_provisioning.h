#pragma once
#include <stdbool.h>

bool ble_prov_credenciais_salvas(void);
void ble_prov_iniciar(void);          
void ble_prov_parar(void);
bool ble_prov_reconectando(void);     // main pode checar se está reconectando
void ble_prov_conectar_wifi(void);