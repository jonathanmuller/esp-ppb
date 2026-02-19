#ifndef HELPER_H
#define HELPER_H
#include "esp_now.h"
#include "esp_wifi_types.h"

static const uint8_t ESP_NOW_HEADER[] = {0x7F, 0x18, 0xFE, 0x34}; //Vendor specific : Espressif
static const uint8_t BROADCAST[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


static const uint8_t MAC_STA_RIGHT[6] = {0x10, 0x00, 0x3B, 0xD2, 0x35, 0x94};
static const uint8_t MAC_STA_TOP[6] = {0x10, 0x00, 0x3B, 0xD2, 0x41, 0x6C};
static const uint8_t MAC_STA_BOTTOM[6] = {0x10, 0x00, 0x3B, 0xD2, 0x52, 0xE0};
static const uint8_t MAC_STA_LEFT[6] = {0x10, 0x00, 0x3B, 0xD2, 0x43, 0xD4};

static const uint8_t MAC_STA_MIDDLE[6] = {0x10, 0x00, 0x3B, 0xD2, 0x38, 0x04};

static const uint8_t MAC_AP_FTM[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};


typedef enum {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    MIDDLE,
    UNKNOWN
} MyType;

#define GET_AUTO_TAG() get_auto_tag(__FILE__, __LINE__)

typedef struct __attribute__((packed)) {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t receiver_addr[6];
    uint8_t transmitter_addr[6];
    uint8_t bssid_addr[6];
    uint16_t seq_ctrl;
} wifi_ieee80211_hdr_t;

#define WIFI_IEEE80211_HDR_LEN 24
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(wifi_ieee80211_hdr_t) == WIFI_IEEE80211_HDR_LEN, "wifi_ieee80211_hdr_t size mismatch");
#endif

#define WIFI_SEQ_NUM(_seq_ctrl) ((uint16_t)((_seq_ctrl) >> 4))


MyType is_my_mac_address(void);

const char *get_auto_tag(const char *file, int line);

void print_wifi_mac(void);

#endif //HELPER_H
