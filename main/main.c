#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <rom/ets_sys.h>
#include <soc/rtc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_rtc_time.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "esp_timer.h"

#include "helper.h"
#include "helper_init.h"
#include "perf.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#include "i2c_helper.h"


// https://github.com/espressif/esp-csi/blob/master/examples/get-started/csi_send/main/app_main.c
// https://github.com/espressif/esp-idf/issues/9843


void app_main(void) {
    //wifi_init();

    print_wifi_mac();

    init_gpio9_interrupt();

    //xTaskCreate(hello_task, "hello_task", 2048, NULL, 5, NULL);

    ESP_LOGI(GET_AUTO_TAG(), "Here is myself");
    //i2c_helper_test_all();

    vTaskDelay(pdMS_TO_TICKS(1000));

    i2c_check_hardware();
    i2c_helper_oled_init();
    i2c_helper_oled_print_string("Hello");
    i2c_set_dac(true, 0); // Reset TCXO
    i2c_set_dac(false, 0); // Reset TCXO

    struct RoleConfig {
        bool use_csi_for_esp_now;
        bool is_ap;
        const char *log_msg;
        const char *display;

        void (*init_fn)(void);

        esp_now_recv_cb_t now_cb;
        void *sent_now_cb;
        wifi_promiscuous_cb_t promi_cb;
        wifi_csi_cb_t csi_cb;

        void (*loop_fn)(void);
    };

    static const struct RoleConfig cfg_ftm = {
        .use_csi_for_esp_now = false,
        .is_ap = false,
        .log_msg = "I will do FTM",
        .display = "FTM",
        .init_fn = NULL,

        .now_cb = NULL,
        .sent_now_cb = NULL,
        .promi_cb = promi_ftm_cb,
        .csi_cb = csi_send_summary,
        .loop_fn = infinite_ftm,
    };

    static const struct RoleConfig cfg_ap = {
        .use_csi_for_esp_now = true,
        .is_ap = true,
        .log_msg = "I will do AP and respond CSI",
        .display = "AP",
        .init_fn = NULL,

        .now_cb = NULL,
        .sent_now_cb = NULL,
        .promi_cb = NULL,
        .csi_cb = csi_ping_pong,
        .loop_fn = NULL,
    };

    static const struct RoleConfig cfg_default = {
        .use_csi_for_esp_now = true,
        .is_ap = false,
        .log_msg = "I will client",
        .display = NULL,

        .init_fn = NULL,
        .now_cb = print_now_recv,
        .sent_now_cb = simple_send_cb,
        .promi_cb = NULL,
        .csi_cb = NULL,
        .loop_fn = infinite_send,
    };

    const struct RoleConfig *cfg = &cfg_default;
    switch (is_my_mac_address()) {
        case LEFT:
        case RIGHT:
        case TOP:
        case BOTTOM:
            cfg = &cfg_ftm;
            break;
        case MIDDLE:
            cfg = &cfg_ap;
            break;
        default:
            break;
    }


    init_wifi(cfg->is_ap, cfg->use_csi_for_esp_now);
    ESP_LOGI(GET_AUTO_TAG(), "%s", cfg->log_msg);
    if (cfg->display != NULL) {
        i2c_helper_oled_print_string(cfg->display);
    }
    if (cfg->init_fn != NULL) {
        cfg->init_fn();
    }
    if (cfg->now_cb != NULL) {
        ESP_ERROR_CHECK(esp_now_register_recv_cb(cfg->now_cb));
    }
    if (cfg->sent_now_cb != NULL) {
        ESP_ERROR_CHECK(esp_now_register_send_cb(cfg->sent_now_cb));
    }
    if (cfg->promi_cb != NULL) {
        ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(cfg->promi_cb));
    }
    if (cfg->csi_cb != NULL) {
        ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(cfg->csi_cb, NULL));
    }
    if (cfg->loop_fn != NULL) {
        cfg->loop_fn();
    }
}
