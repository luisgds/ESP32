#include "configure_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/sockets.h"
#include <string.h>
#include <stdbool.h>
//#include "ir_controle.h"

static const char *TAG_WIFI = "WIFI";

#define WIFI_SSID          "VIVOFIBRA-BF61"
#define WIFI_PASSWORD      "lauracatluis"
#define UDP_PORT           12345
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;

uint8_t deve_dormir = 0; // acessível pela main
bool swing = false; // acessível pela main
bool buzzer_ativo = false; // acessível pela main

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    }
}

void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG_WIFI, "WiFi conectado!");
}

void udp_listener_task(void *pvParameters) {
    char rx_buffer[64];

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(UDP_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    ESP_LOGI(TAG_WIFI, "Aguardando pacote UDP na porta %d...", UDP_PORT);

    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len > 0) {
            rx_buffer[len] = '\0';
            ESP_LOGI(TAG_WIFI, "Pacote recebido: %s", rx_buffer);
            if (strcmp(rx_buffer, "SLEEP0") == 0) {
                deve_dormir = 0;
            }
            else if (strcmp(rx_buffer, "SLEEP1") == 0) {
                deve_dormir = 1;
            }
            else if (strcmp(rx_buffer, "SLEEP2") == 0) {
                deve_dormir = 2;
            }
            else if (strcmp(rx_buffer, "SLEEP3") == 0) {
                deve_dormir = 3;
            }
            else if (strcmp(rx_buffer, "SWING") == 0) {
                swing = !swing;
                ESP_LOGI(TAG_WIFI, "Swing %s!", swing ? "ligado" : "desligado");
            }
            else if (strcmp(rx_buffer, "BUZZER") == 0) {
                buzzer_ativo = !buzzer_ativo;
                ESP_LOGI(TAG_WIFI, "Buzzer %s!", buzzer_ativo ? "ligado" : "desligado");
            }
            else {
                ESP_LOGW(TAG_WIFI, "Comando desconhecido: %s", rx_buffer);
            }
        }
    }

    close(sock);
    vTaskDelete(NULL);
}