#include <nvs_flash.h>
#include <string.h>
#include <rom/ets_sys.h>
#include <sys/unistd.h>

#include "constant.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_now.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_http_server.h"  // Include HTTP server


#include "helper.h"
#include "esp_private/wifi.h"


static void add_esp_now_peer(const uint8_t *mac_addr, wifi_interface_t ifidx, wifi_tx_rate_config_t tx_config) {
    esp_now_peer_info_t peer = {
        .channel = MAIN_CHANNEL,
        .ifidx = WIFI_IF_STA,
        .encrypt = false,
    };

    peer.ifidx = ifidx;

    memcpy(peer.peer_addr, mac_addr, sizeof(peer.peer_addr));

    if (esp_now_is_peer_exist(peer.peer_addr)) {
        esp_now_del_peer(peer.peer_addr);
    }

    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    ESP_ERROR_CHECK(esp_now_set_peer_rate_config(peer.peer_addr, &tx_config));
}


void init_wifi_basics() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_netif_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void init_wifi_ap() {
    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_mac(WIFI_IF_AP, MAC_AP_FTM));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "DEBUG",
            .ssid_len = 5,
            .channel = MAIN_CHANNEL,
            .authmode = WIFI_AUTH_OPEN,
            .ftm_responder = true,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));
}

void init_wifi_sta() {
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
}

void init_wifi_settings() {
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(MAIN_CHANNEL, WIFI_SECOND_CHAN_BELOW));
    ESP_ERROR_CHECK(esp_now_init());

    wifi_csi_config_t csi_config = {
        .lltf_en = true,
        .htltf_en = true,
        .stbc_htltf2_en = true,
        .ltf_merge_en = true,
        .channel_filter_en = true,
        .dump_ack_en = true,
    };
    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(84));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    wifi_promiscuous_filter_t prom_filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL,
    };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&prom_filter));
    wifi_promiscuous_filter_t prom_ctrl_filter = {
        .filter_mask = WIFI_PROMIS_CTRL_FILTER_MASK_ALL,
    };
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_ctrl_filter(&prom_ctrl_filter));
    ESP_ERROR_CHECK(esp_wifi_set_csi(true));
}

void init_wifi(bool is_ap, bool use_csi_for_esp_now) {
    init_wifi_basics();

    esp_log_level_set("wifi", ESP_LOG_ERROR);

    if (is_ap) {
        init_wifi_ap();
    } else {
        init_wifi_sta();
    }
    init_wifi_settings();

    wifi_interface_t ifidx;
    if (is_ap) {
        ifidx = WIFI_IF_AP;
    } else {
        ifidx = WIFI_IF_STA;
    }

    wifi_tx_rate_config_t tx_config;
    if (use_csi_for_esp_now) {
        tx_config.phymode = WIFI_PHY_MODE_HT20;
        tx_config.rate = WIFI_PHY_RATE_MCS7_SGI;
    } else {
        tx_config.phymode = WIFI_PHY_MODE_11B;
        tx_config.rate = WIFI_PHY_RATE_1M_L;
    }


    add_esp_now_peer(BROADCAST, ifidx, tx_config);
    //add_esp_now_peer(MAC_AP_FTM, ifidx);
}


static void IRAM_ATTR gpio_isr_handler(void *arg) {
    int new_state = gpio_get_level(GPIO_NUM_9);
    for (int i = 0; i < 6; i++) {
        ets_printf("Boot button state changed (%d -> %d)\n", !new_state, new_state);
    }
}

void init_gpio9_interrupt() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_9);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_NUM_9, gpio_isr_handler, (void *) GPIO_NUM_9);
}
