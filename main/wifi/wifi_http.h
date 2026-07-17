/*   - mDNS  → o dispositivo aparece em http://esp32-ac.local na rede
 *   - HTTP  → comandos chegam via POST /comando (JSON)
 * ┌────────────────────────────────────────────────────────────┐
 * │ GET  /           → página HTML de apresentação            │
 * │ GET  /status     → estado atual em JSON                   │
 * │ POST /comando    → recebe comando JSON e aplica           │
 * └────────────────────────────────────────────────────────────┘
 * EXEMPLOS DE COMANDOS (body JSON no POST /comando):
 *   {"cmd": "swing",   "valor": 1}    // 1 = ativa, 0 = desativa
 *   {"cmd": "sleep",   "valor": 2}    // 0 = cancela | 1=4h 2=8h 3=12h
 *   {"cmd": "buzzer",  "valor": 1}    // 1 = ativa, 0 = desativa
 *   {"cmd": "umidade", "valor": 1}    // 1 = ativa, 0 = desativa
 */

#pragma once

#include "esp_err.h"

/**
 * Inicia o mDNS e o servidor HTTP.
 *        Chame APÓS o WiFi estar conectado.
 * retorna ESP_OK em sucesso, ESP_FAIL se não conseguir subir o servidor.
 */
esp_err_t http_server_iniciar(void);

/**
 *  Para o servidor HTTP e libera o mDNS.
 *  Útil antes de entrar em deep sleep ou reconectar WiFi.
 */
void http_server_parar(void);