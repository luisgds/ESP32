#include "rgb_led.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"

#define PINO_RGB 8
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000

static rmt_channel_handle_t led_channel = NULL;
static rmt_encoder_handle_t led_encoder = NULL;

static const rmt_bytes_encoder_config_t bytes_encoder_config = {
    .bit0 = {
        .level0    = 1, .duration0 = 3,
        .level1    = 0, .duration1 = 9,
    },
    .bit1 = {
        .level0    = 1, .duration0 = 9,
        .level1    = 0, .duration1 = 3,
    },
    .flags.msb_first = 1
};

void rgb_led_init(void) {
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num          = PINO_RGB,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = RMT_LED_STRIP_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_cfg, &led_channel));
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder));
    ESP_ERROR_CHECK(rmt_enable(led_channel));
}

void rgb_led_set(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t payload[3] = {g, r, b}; // WS2812 recebe GRB
    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    ESP_ERROR_CHECK(rmt_transmit(led_channel, led_encoder,
                                 payload, sizeof(payload), &tx_cfg));
    rmt_tx_wait_all_done(led_channel, 100);
}

void rgb_verde(void)    { rgb_led_set(0,   255, 0);   }
void rgb_vermelho(void) { rgb_led_set(255, 0,   0);   }
void rgb_azul(void)     { rgb_led_set(0,   0,   255); }
void rgb_amarelo(void)  { rgb_led_set(255, 255, 0);   }
void rgb_roxo(void)     { rgb_led_set(128, 0,   128); }
void rgb_apagado(void)  { rgb_led_set(0,   0,   0);   }