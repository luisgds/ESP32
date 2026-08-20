#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdbool.h>

#include "configure_wifi_mqtt.h"

static const char *TAG_MQTT = "MQTT";

#define MQTT_BROKER "mqtt://broker.hivemq.com:1883"
#define TOPIC_COMANDOS  "esp32/ac/comandos"
#define TOPIC_STATUS    "esp32/ac/status"

static esp_mqtt_client_handle_t mqtt_client = NULL;

uint8_t deve_dormir  = 0;
bool    swing        = false;
bool    buzzer_ativo = false;

static void processar_comando(const char *cmd) {
    if      (strcmp(cmd, "SLEEP0") == 0) { deve_dormir = 0; }
    else if (strcmp(cmd, "SLEEP1") == 0) { deve_dormir = 1; }
    else if (strcmp(cmd, "SLEEP2") == 0) { deve_dormir = 2; }
    else if (strcmp(cmd, "SLEEP3") == 0) { deve_dormir = 3; }
    else if (strcmp(cmd, "SWING")  == 0) {
        swing = !swing;
        esp_mqtt_client_publish(mqtt_client, TOPIC_STATUS,
                                swing ? "swing:on" : "swing:off", 0, 1, 0);
    }
    else if (strcmp(cmd, "BUZZER") == 0) {
        buzzer_ativo = !buzzer_ativo;
        esp_mqtt_client_publish(mqtt_client, TOPIC_STATUS,
                                buzzer_ativo ? "buzzer:on" : "buzzer:off", 0, 1, 0);
    }
    else {
        ESP_LOGW(TAG_MQTT, "Comando desconhecido: %s", cmd);
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_MQTT, "MQTT conectado!");
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_COMANDOS, 1);
            esp_mqtt_client_publish(mqtt_client, TOPIC_STATUS, "online", 0, 1, 0);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG_MQTT, "MQTT desconectado! Reconectando...");
            break;

        case MQTT_EVENT_DATA: {
            char payload[64] = {0};
            int  len = event->data_len < 63 ? event->data_len : 63;
            strncpy(payload, event->data, len);
            ESP_LOGI(TAG_MQTT, "Tópico: %.*s | Payload: %s",
                     event->topic_len, event->topic, payload);
            processar_comando(payload);
            break;
        }

        default:
            break;
    }
}

void mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    ESP_LOGI(TAG_MQTT, "MQTT iniciado! Broker: %s", MQTT_BROKER);
}