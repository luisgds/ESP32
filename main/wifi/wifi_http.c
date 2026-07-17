#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "mdns.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "wifi_http.h"
/* ══════════════════════════════════════════════════════════════════
 *  VARIÁVEIS EXTERNAS
 *  Definidas nos módulos de controle e compartilhadas com main.c.
 *  "extern" = "existe em outro .c, não me cria uma nova cópia".
 * ══════════════════════════════════════════════════════════════════ */

/* De AjustarSwing.c (ou AjustarSwing.h se for .h com definição) */
extern bool swing;

/* De AjustarSleep.c */
extern int  deve_dormir;    // 0=nenhum 1=4h 2=8h 3=12h
extern bool sleep_acabou;

/* De buzzer.c */
extern bool buzzer_ativo;

/* Funções de AjustarSleep.c — declaração sem include do .h */
extern bool sleep_ativo(void);
extern int  sleep_tempo_restante_min(void);

/* ── Constantes ───────────────────────────────────────────────── */
#define MDNS_HOSTNAME   "esp32-ac"
#define MDNS_INSTANCE   "ESP32 Ar Condicionado"
#define HTTP_PORT       80
#define BODY_MAX_LEN    512

static const char     *TAG      = "HTTP";
static httpd_handle_t  s_server = NULL;

/* ══════════════════════════════════════════════════════════════════
 *  UTILITÁRIOS INTERNOS
 * ══════════════════════════════════════════════════════════════════ */

static void adicionar_cors(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin",  "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
}

static void obter_ip(char *buf, size_t len) {
    esp_netif_ip_info_t info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &info) == ESP_OK)
        snprintf(buf, len, IPSTR, IP2STR(&info.ip));
    else
        snprintf(buf, len, "0.0.0.0");
}

/* ══════════════════════════════════════════════════════════════════
 *  HANDLER: GET /
 * ══════════════════════════════════════════════════════════════════ */
static esp_err_t handler_root(httpd_req_t *req)
{
    adicionar_cors(req);
    httpd_resp_set_type(req, "text/html; charset=utf-8");

    const char *html =
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>ESP32-AC</title><style>"
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{font-family:sans-serif;background:#f0f4f8;min-height:100vh;padding:20px}"
    ".card{background:#fff;border-radius:12px;padding:20px;margin-bottom:16px;"
    "box-shadow:0 2px 8px rgba(0,0,0,.1)}"
    "h1{color:#1a73e8;margin-bottom:4px;font-size:1.5rem}"
    "h2{color:#444;font-size:1rem;margin-bottom:12px;font-weight:600}"
    ".status-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}"
    ".status-item{background:#f8f9fa;border-radius:8px;padding:10px;text-align:center}"
    ".status-label{font-size:.75rem;color:#666;margin-bottom:4px}"
    ".status-value{font-size:1.1rem;font-weight:700;color:#1a73e8}"
    ".btn-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}"
    "button{padding:12px;border:none;border-radius:8px;font-size:.9rem;"
    "font-weight:600;cursor:pointer;transition:all .2s}"
    ".btn-on{background:#e8f5e9;color:#2e7d32}"
    ".btn-on:hover{background:#2e7d32;color:#fff}"
    ".btn-off{background:#fce4ec;color:#c62828}"
    ".btn-off:hover{background:#c62828;color:#fff}"
    ".btn-sleep{background:#e3f2fd;color:#1565c0}"
    ".btn-sleep:hover{background:#1565c0;color:#fff}"
    ".btn-sleep.active{background:#1565c0;color:#fff}"
    ".tag{display:inline-block;padding:2px 8px;border-radius:20px;"
    "font-size:.75rem;font-weight:700}"
    ".tag-on{background:#e8f5e9;color:#2e7d32}"
    ".tag-off{background:#fce4ec;color:#c62828}"
    "#msg{padding:8px 12px;border-radius:8px;font-size:.85rem;"
    "display:none;margin-top:8px}"
    ".msg-ok{background:#e8f5e9;color:#2e7d32}"
    ".msg-err{background:#fce4ec;color:#c62828}"
    "</style></head><body>"

    "<div class='card'>"
    "<h1>&#127970; ESP32-AC</h1>"
    "<p style='color:#666;font-size:.85rem'>esp32-ac.local</p>"
    "</div>"

    /* ── STATUS ── */
    "<div class='card'>"
    "<h2>&#128268; Status</h2>"
    "<div class='status-grid' id='status-grid'>"
    "<div class='status-item'><div class='status-label'>Swing</div>"
    "<div class='status-value' id='st-swing'>...</div></div>"
    "<div class='status-item'><div class='status-label'>Buzzer</div>"
    "<div class='status-value' id='st-buzzer'>...</div></div>"
    "<div class='status-item'><div class='status-label'>Sleep modo</div>"
    "<div class='status-value' id='st-sleep-modo'>...</div></div>"
    "<div class='status-item'><div class='status-label'>Restante</div>"
    "<div class='status-value' id='st-restante'>...</div></div>"
    "</div>"
    "<p style='font-size:.75rem;color:#999;margin-top:8px' id='st-update'></p>"
    "</div>"

    /* ── SWING ── */
    "<div class='card'>"
    "<h2>&#127744; Swing</h2>"
    "<div class='btn-grid'>"
    "<button class='btn-on' onclick='cmd(\"swing\",1)'>&#9654; Ligar</button>"
    "<button class='btn-off' onclick='cmd(\"swing\",0)'>&#9632; Desligar</button>"
    "</div></div>"

    /* ── BUZZER ── */
    "<div class='card'>"
    "<h2>&#128266; Buzzer</h2>"
    "<div class='btn-grid'>"
    "<button class='btn-on' onclick='cmd(\"buzzer\",1)'>&#9654; Ligar</button>"
    "<button class='btn-off' onclick='cmd(\"buzzer\",0)'>&#9632; Desligar</button>"
    "</div></div>"

    /* ── SLEEP ── */
    "<div class='card'>"
    "<h2>&#128336; Timer Sleep</h2>"
    "<div class='btn-grid'>"
    "<button class='btn-off' onclick='cmd(\"sleep\",0)'>&#10006; Cancelar</button>"
    "<button class='btn-sleep' onclick='cmd(\"sleep\",1)'>4 horas</button>"
    "<button class='btn-sleep' onclick='cmd(\"sleep\",2)'>8 horas</button>"
    "<button class='btn-sleep' onclick='cmd(\"sleep\",3)'>12 horas</button>"
    "</div></div>"

    /* ── FEEDBACK ── */
    "<div id='msg'></div>"

    "<script>"
    "function cmd(c,v){"
    "  fetch('/comando',{method:'POST',"
    "    headers:{'Content-Type':'application/json'},"
    "    body:JSON.stringify({cmd:c,valor:v})})"
    "  .then(r=>r.json()).then(d=>{"
    "    showMsg(d.ok?'OK: '+c+' = '+v:'Erro!', d.ok);"
    "    fetchStatus();"
    "  }).catch(()=>showMsg('Erro de conexão',false));"
    "}"
    "function showMsg(txt,ok){"
    "  var m=document.getElementById('msg');"
    "  m.textContent=txt;"
    "  m.className=ok?'msg-ok':'msg-err';"
    "  m.style.display='block';"
    "  setTimeout(()=>m.style.display='none',2000);"
    "}"
    "var modos=['OFF','4h','8h','12h'];"
    "function fetchStatus(){"
    "  fetch('/status').then(r=>r.json()).then(d=>{"
    "    var tag=function(v){return v"
    "      ?'<span class=\"tag tag-on\">ON</span>'"
    "      :'<span class=\"tag tag-off\">OFF</span>';};"
    "    document.getElementById('st-swing').innerHTML=tag(d.swing);"
    "    document.getElementById('st-buzzer').innerHTML=tag(d.buzzer);"
    "    document.getElementById('st-sleep-modo').textContent=modos[d.sleep_modo]||'-';"
    "    document.getElementById('st-restante').textContent="
    "      d.sleep_ativo?(d.sleep_restante_min+' min'):'—';"
    "    document.getElementById('st-update').textContent="
    "      'Atualizado: '+new Date().toLocaleTimeString();"
    "  }).catch(()=>{});"
    "}"
    "fetchStatus();"
    "setInterval(fetchStatus,3000);"  /* atualiza a cada 3s */
    "</script></body></html>";

    httpd_resp_sendstr(req, html);
    return ESP_OK;
}

/* ══════════════════════════════════════════════════════════════════
 *  HANDLER: GET /status
 * ══════════════════════════════════════════════════════════════════ */
static esp_err_t handler_status(httpd_req_t *req) {
    adicionar_cors(req);
    httpd_resp_set_type(req, "application/json");

    char ip[24];
    obter_ip(ip, sizeof(ip));

    bool  s_ativo    = sleep_ativo();
    int   s_restante = s_ativo ? sleep_tempo_restante_min() : 0;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device",            "esp32-ac");
    cJSON_AddStringToObject(root, "mdns",              MDNS_HOSTNAME ".local");
    cJSON_AddStringToObject(root, "ip",                ip);
    cJSON_AddBoolToObject  (root, "swing",             swing);
    cJSON_AddNumberToObject(root, "sleep_modo",        deve_dormir);
    cJSON_AddBoolToObject  (root, "sleep_ativo",       s_ativo);
    cJSON_AddNumberToObject(root, "sleep_restante_min",s_restante);
    cJSON_AddBoolToObject  (root, "buzzer",            buzzer_ativo);

    char *json_str = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, json_str ? json_str : "{}");

    free(json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/* ══════════════════════════════════════════════════════════════════
 *  HANDLER: POST /comando
 * ══════════════════════════════════════════════════════════════════ */
static esp_err_t handler_comando(httpd_req_t *req) {
    adicionar_cors(req);
    httpd_resp_set_type(req, "application/json");

    if (req->content_len == 0 || req->content_len >= BODY_MAX_LEN) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "{\"erro\":\"body ausente ou muito grande\"}");
        return ESP_FAIL;
    }

    char body[BODY_MAX_LEN];
    int  lido = httpd_req_recv(req, body, req->content_len);
    if (lido <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "{\"erro\":\"falha ao ler body\"}");
        return ESP_FAIL;
    }
    body[lido] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "{\"erro\":\"JSON invalido\"}");
        return ESP_FAIL;
    }

    cJSON *j_cmd   = cJSON_GetObjectItem(json, "cmd");
    cJSON *j_valor = cJSON_GetObjectItem(json, "valor");

    if (!cJSON_IsString(j_cmd) || !cJSON_IsNumber(j_valor)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "{\"erro\":\"'cmd' (string) e 'valor' (numero) obrigatorios\"}");
        return ESP_FAIL;
    }

    const char *cmd = j_cmd->valuestring;
    int         val = (int)j_valor->valuedouble;

    /* ── Tabela de comandos ─────────────────────────────────────── */
    if (strcmp(cmd, "swing") == 0) {
        swing = (val != 0);
        ESP_LOGI(TAG, "swing → %s", swing ? "ON" : "OFF");

    } else if (strcmp(cmd, "sleep") == 0) {
        if (val < 0 || val > 3) {
            cJSON_Delete(json);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "{\"erro\":\"sleep: 0=cancela 1=4h 2=8h 3=12h\"}");
            return ESP_FAIL;
        }
        deve_dormir = val;
        ESP_LOGI(TAG, "sleep → %d", deve_dormir);

    } else if (strcmp(cmd, "buzzer") == 0) {
        buzzer_ativo = (val != 0);
        ESP_LOGI(TAG, "buzzer → %s", buzzer_ativo ? "ON" : "OFF");

    } else if (strcmp(cmd, "umidade") == 0) {
        /* TODO: adicione "extern bool umidade_ativo;" acima quando
                 a variável estiver declarada em AjustarUmidade.c   */
        ESP_LOGW(TAG, "umidade: variavel nao ligada ainda (val=%d)", val);

    } else {
        cJSON_Delete(json);
        ESP_LOGW(TAG, "Comando desconhecido: %s", cmd);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "{\"erro\":\"comando desconhecido\"}");
        return ESP_FAIL;
    }

    cJSON_Delete(json);

    char resp[80];
    snprintf(resp, sizeof(resp),
             "{\"ok\":true,\"cmd\":\"%s\",\"valor\":%d}", cmd, val);
    httpd_resp_sendstr(req, resp);
    return ESP_OK;
}

/* ══════════════════════════════════════════════════════════════════
 *  HANDLER: OPTIONS (CORS pre-flight)
 * ══════════════════════════════════════════════════════════════════ */
static esp_err_t handler_options(httpd_req_t *req) {
    adicionar_cors(req);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* ══════════════════════════════════════════════════════════════════
 *  mDNS — anuncia o dispositivo na rede local
 * ══════════════════════════════════════════════════════════════════ */
static void mdns_iniciar(void) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set(MDNS_INSTANCE));

    mdns_txt_item_t txt[] = {
        {"path",    "/"},
        {"version", "1"},
    };
    ESP_ERROR_CHECK(mdns_service_add(
        NULL, "_http", "_tcp", HTTP_PORT,
        txt, sizeof(txt) / sizeof(txt[0])
    ));

    ESP_LOGI(TAG, "mDNS ok → http://" MDNS_HOSTNAME ".local");
}

/* ══════════════════════════════════════════════════════════════════
 *  API PÚBLICA
 * ══════════════════════════════════════════════════════════════════ */

esp_err_t http_server_iniciar(void) {
    if (s_server) {
        ESP_LOGW(TAG, "Servidor já rodando.");
        return ESP_OK;
    }

    mdns_iniciar();

    httpd_config_t cfg   = HTTPD_DEFAULT_CONFIG();
    cfg.server_port      = HTTP_PORT;
    cfg.max_uri_handlers = 8;
    cfg.lru_purge_enable = true;

    if (httpd_start(&s_server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar HTTP server");
        return ESP_FAIL;
    }

    static const httpd_uri_t rotas[] = {
        { .uri = "/",        .method = HTTP_GET,     .handler = handler_root    },
        { .uri = "/status",  .method = HTTP_GET,     .handler = handler_status  },
        { .uri = "/comando", .method = HTTP_POST,    .handler = handler_comando },
        { .uri = "/status",  .method = HTTP_OPTIONS, .handler = handler_options },
        { .uri = "/comando", .method = HTTP_OPTIONS, .handler = handler_options },
    };

    for (int i = 0; i < (int)(sizeof(rotas) / sizeof(rotas[0])); i++)
        httpd_register_uri_handler(s_server, &rotas[i]);

    char ip[24];
    obter_ip(ip, sizeof(ip));
    ESP_LOGI(TAG, "HTTP ok → http://%s  ou  http://" MDNS_HOSTNAME ".local", ip);

    return ESP_OK;
}

void http_server_parar(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server parado.");
    }
    mdns_free();
}