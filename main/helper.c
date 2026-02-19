#include <string.h>
#include <rom/ets_sys.h>

#include "freertos/FreeRTOS.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "helper.h"
#include "constant.h"


MyType is_my_mac_address(void) {
    static uint8_t mac_addr[6];
    static MyType cache;
    static bool cache_is_set = false;
    if (!cache_is_set) {
        ESP_ERROR_CHECK(esp_read_mac(mac_addr, ESP_MAC_WIFI_STA));

        if (memcmp(mac_addr, MAC_STA_LEFT, sizeof(uint8_t) * 6) == 0) {
            cache = LEFT;
        } else if (memcmp(mac_addr, MAC_STA_RIGHT, sizeof(uint8_t) * 6) == 0) {
            cache = RIGHT;
        } else if (memcmp(mac_addr, MAC_STA_TOP, sizeof(uint8_t) * 6) == 0) {
            cache = TOP;
        } else if (memcmp(mac_addr, MAC_STA_BOTTOM, sizeof(uint8_t) * 6) == 0) {
            cache = BOTTOM;
        } else if (memcmp(mac_addr, MAC_STA_MIDDLE, sizeof(uint8_t) * 6) == 0) {
            cache = MIDDLE;
        } else {
            cache = UNKNOWN;
        }
        cache_is_set = true;
    }
    return cache;
}


const char *get_auto_tag(const char *file, int line) {
    static char tag_buffer[128];

    MyType type = is_my_mac_address();

    const char *type_str;

    switch (type) {
        case LEFT: type_str = "LEFT__";
            break;
        case RIGHT: type_str = "RIGHT_";
            break;
        case TOP: type_str = "TOP___";
            break;
        case BOTTOM: type_str = "BOTTOM";
            break;
        case MIDDLE: type_str = "MIDDLE";
            break;
        default: type_str = "????";
            break;
    }

    snprintf(tag_buffer, sizeof(tag_buffer), "[%s:%d] [%s]", file, line, type_str);
    return tag_buffer;
}


void print_wifi_mac(void) {
    static uint8_t mac_sta[6];
    static uint8_t mac_ap[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac_sta);
    esp_wifi_get_mac(WIFI_IF_AP, mac_ap);
    ESP_LOGI(GET_AUTO_TAG(), "STA MAC is {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}", mac_sta[0], mac_sta[1], mac_sta[2], mac_sta[3], mac_sta[4], mac_sta[5]);
    ESP_LOGI(GET_AUTO_TAG(), "AP MAC is {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}", mac_ap[0], mac_ap[1], mac_ap[2], mac_ap[3], mac_ap[4], mac_ap[5]);
}



