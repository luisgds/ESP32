#include "ble_provisioning.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG_BLE = "BLE_PROV";

#define NVS_NAMESPACE "wifi_creds"

// ── UUIDs ──
static const uint16_t PROV_SERVICE_UUID   = 0xFF01;
static const uint16_t CHAR_SSID_UUID      = 0xFF02;
static const uint16_t CHAR_PASS_UUID      = 0xFF03;
static const uint16_t CHAR_STATUS_UUID    = 0xFF04;

// ── propriedades das characteristics ──
static const uint8_t char_prop_write         = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_notify   = ESP_GATT_CHAR_PROP_BIT_READ |
                                               ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint16_t primary_service_uuid   = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration  = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t char_client_config     = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

// ── índices da tabela GATT ──
enum {
    IDX_SVC,
    IDX_SSID_DECL, IDX_SSID_VAL,
    IDX_PASS_DECL, IDX_PASS_VAL,
    IDX_STATUS_DECL, IDX_STATUS_VAL, IDX_STATUS_CFG,
    IDX_MAX
};

static uint16_t gatt_handles[IDX_MAX];

// ── estado interno ──
static uint16_t gatts_if_saved  = 0;
static uint16_t conn_id_saved   = 0;
static bool     cliente_conectado = false;
static bool     reconectando      = false;

static char ssid_recebido[32] = {0};
static char pass_recebido[64] = {0};
static bool ssid_ok = false;
static bool pass_ok = false;

// ── status que o nRF Connect pode ler ──
static char status_atual[32] = "aguardando";

// ── advertising ──
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// ── tabela GATT ──
static const esp_gatts_attr_db_t gatt_db[IDX_MAX] = {
    // Service
    [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid,
         ESP_GATT_PERM_READ,
         sizeof(uint16_t), sizeof(PROV_SERVICE_UUID),
         (uint8_t *)&PROV_SERVICE_UUID}
    },
    // SSID declaration
    [IDX_SSID_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration,
         ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(char_prop_write),
         (uint8_t *)&char_prop_write}
    },
    // SSID value
    [IDX_SSID_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_SSID_UUID,
         ESP_GATT_PERM_WRITE,
         32, 0, NULL}
    },
    // PASS declaration
    [IDX_PASS_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration,
         ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(char_prop_write),
         (uint8_t *)&char_prop_write}
    },
    // PASS value
    [IDX_PASS_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_PASS_UUID,
         ESP_GATT_PERM_WRITE,
         64, 0, NULL}
    },
    // STATUS declaration
    [IDX_STATUS_DECL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&character_declaration,
         ESP_GATT_PERM_READ,
         sizeof(uint8_t), sizeof(char_prop_read_notify),
         (uint8_t *)&char_prop_read_notify}
    },
    // STATUS value
    [IDX_STATUS_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&CHAR_STATUS_UUID,
         ESP_GATT_PERM_READ,
         sizeof(status_atual), 10,
         (uint8_t *)status_atual}
    },
    // STATUS CCCD (necessário para NOTIFY)
    [IDX_STATUS_CFG] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_client_config,
         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
         sizeof(uint16_t), 0, NULL}
    },
};

// ── atualiza status e notifica cliente BLE ──
static void atualizar_status(const char *msg) {
    strncpy(status_atual, msg, sizeof(status_atual) - 1);
    ESP_LOGI(TAG_BLE, "Status: %s", status_atual);

    if (cliente_conectado) {
        esp_ble_gatts_send_indicate(gatts_if_saved, conn_id_saved,
                                     gatt_handles[IDX_STATUS_VAL],
                                     strlen(status_atual),
                                     (uint8_t *)status_atual, false);
    }
}

static bool provisioning_em_andamento = false;

// ── reconecta WiFi sem reiniciar ──
static void reconectar_wifi_task(void *pvParameters) {
    reconectando = true;
    provisioning_em_andamento = true;
    atualizar_status("desconectando...");
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_wifi_stop();

    // salva novas credenciais na NVS
    nvs_handle_t handle;
    nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    nvs_set_str(handle, "ssid", ssid_recebido);
    nvs_set_str(handle, "pass", pass_recebido);
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG_BLE, "Credenciais salvas! SSID: %s", ssid_recebido);

    // reconfigura WiFi com novas credenciais
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid,     ssid_recebido, 32);
    strncpy((char *)wifi_config.sta.password, pass_recebido, 64);

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    atualizar_status("conectando...");
    esp_wifi_connect();

    // aguarda resultado (máximo 10 segundos)
    int tentativas = 0;
    wifi_ap_record_t ap_info;
    while (tentativas < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            // conectou!
            atualizar_status("conectado!");
            ESP_LOGI(TAG_BLE, "WiFi reconectado em: %s", ssid_recebido);
            provisioning_em_andamento = false;
            reconectando = false;
            ssid_ok = false;
            pass_ok = false;
            vTaskDelete(NULL);
            return;
        }
        tentativas++;
    }

    // falhou
    atualizar_status("erro: verifique SSID/senha");
    ESP_LOGE(TAG_BLE, "Falha ao conectar em: %s", ssid_recebido);
    provisioning_em_andamento = false;
    reconectando = false;
    ssid_ok = false;
    pass_ok = false;
    vTaskDelete(NULL);
}

// ── callback GATT ──
static void gatts_event_handler(esp_gatts_cb_event_t event,
                                  esp_gatt_if_t gatts_if,
                                  esp_ble_gatts_cb_param_t *param) {
    switch (event) {

        case ESP_GATTS_REG_EVT:
            esp_ble_gap_set_device_name("ESP32-AC");
            esp_ble_gap_config_adv_data(&adv_data);
            esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_MAX, 0);
            gatts_if_saved = gatts_if;
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status == ESP_GATT_OK &&
                param->add_attr_tab.num_handle == IDX_MAX) {
                memcpy(gatt_handles, param->add_attr_tab.handles,
                       sizeof(gatt_handles));
                esp_ble_gatts_start_service(gatt_handles[IDX_SVC]);
                ESP_LOGI(TAG_BLE, "Serviço BLE pronto!");
            }
            break;

        case ESP_GATTS_CONNECT_EVT:
            conn_id_saved     = param->connect.conn_id;
            cliente_conectado = true;
            ESP_LOGI(TAG_BLE, "Cliente BLE conectou!");
            // para advertising enquanto tem cliente conectado
            esp_ble_gap_stop_advertising();
            atualizar_status("cliente conectado");
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            cliente_conectado = false;
            ESP_LOGI(TAG_BLE, "Cliente BLE desconectou.");
            atualizar_status("aguardando");
            // retoma advertising para aceitar nova conexão
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GATTS_WRITE_EVT: {
            uint16_t handle = param->write.handle;
            uint8_t *data   = param->write.value;
            uint16_t len    = param->write.len;

            if (handle == gatt_handles[IDX_SSID_VAL]) {
                memcpy(ssid_recebido, data, len);
                ssid_recebido[len] = '\0';
                ssid_ok = true;
                ESP_LOGI(TAG_BLE, "SSID recebido: %s", ssid_recebido);
                atualizar_status("ssid recebido");

            } else if (handle == gatt_handles[IDX_PASS_VAL]) {
                memcpy(pass_recebido, data, len);
                pass_recebido[len] = '\0';
                pass_ok = true;
                ESP_LOGI(TAG_BLE, "Senha recebida!");
                atualizar_status("senha recebida");
            }

            // recebeu os dois → reconecta sem reiniciar
            if (ssid_ok && pass_ok && !reconectando) {
                xTaskCreate(reconectar_wifi_task, "wifi_recon",
                            4096, NULL, 5, NULL);
            }
            break;
        }

        default:
            break;
    }
}

// ── callback GAP ──
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                               esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_gap_start_advertising(&adv_params);
        ESP_LOGI(TAG_BLE, "BLE advertising! Procure 'ESP32-AC' no nRF Connect");
    }
}

// ── API pública ──
bool ble_prov_credenciais_salvas(void) {
    char ssid[32], pass[64];
    size_t sl = sizeof(ssid), pl = sizeof(pass);
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;
    bool ok = (nvs_get_str(h, "ssid", ssid, &sl) == ESP_OK) &&
              (nvs_get_str(h, "pass", pass, &pl) == ESP_OK);
    nvs_close(h);
    return ok;
}

void ble_prov_iniciar(void) {
    ESP_LOGI(TAG_BLE, "Iniciando BLE...");

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
    ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(512));

    ESP_LOGI(TAG_BLE, "BLE iniciado! Sempre visível no nRF Connect.");
}

void ble_prov_parar(void) {
    esp_ble_gap_stop_advertising();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
}

bool ble_prov_reconectando(void) {
    return reconectando;
}

static void ble_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (!provisioning_em_andamento) {
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_BLE, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void ble_prov_conectar_wifi(void) {
    char ssid[32] = {0};
    char pass[64] = {0};
    size_t ssid_len = sizeof(ssid), pass_len = sizeof(pass);

    nvs_handle_t handle;
    nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    nvs_get_str(handle, "ssid", ssid, &ssid_len);
    nvs_get_str(handle, "pass", pass, &pass_len);
    nvs_close(handle);

    ESP_LOGI(TAG_BLE, "Conectando WiFi: %s", ssid);
    static bool wifi_iniciado = false;

    if (!wifi_iniciado) {
        wifi_iniciado = true;
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ble_wifi_event_handler, NULL);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ble_wifi_event_handler, NULL);

        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
    }
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid,     ssid, 32);
    strncpy((char *)wifi_config.sta.password, pass, 64);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}
