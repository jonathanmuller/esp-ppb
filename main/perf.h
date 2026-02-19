#ifndef PERF_H
#define PERF_H


#include <helper.h>
#include <string.h>

#include "esp_now.h"


/** @brief Received packet radio metadata header, this is the common header at the beginning of all promiscuous mode RX callback buffers */
typedef struct {
    signed rssi: 8; /**< Received Signal Strength Indicator(RSSI) of packet. unit: dBm */
    unsigned rate: 5; /**< PHY rate encoding of the packet. Only valid for non HT(11bg) packet */
    unsigned : 1; /**< reserved */
    unsigned sig_mode: 2; /**< Protocol of the received packet, 0: non HT(11bg) packet; 1: HT(11n) packet; 3: VHT(11ac) packet */
    unsigned : 16; /**< reserved */
    unsigned mcs: 7; /**< Modulation Coding Scheme. If is HT(11n) packet, shows the modulation, range from 0 to 76(MSC0 ~ MCS76) */
    unsigned cwb: 1; /**< Channel Bandwidth of the packet. 0: 20MHz; 1: 40MHz */
    unsigned : 16; /**< reserved */
    unsigned smoothing: 1; /**< Set to 1 indicates that channel estimate smoothing is recommended.
                                       Set to 0 indicates that only per-carrierindependent (unsmoothed) channel estimate is recommended. */
    unsigned not_sounding: 1; /**< Set to 0 indicates that PPDU is a sounding PPDU. Set to 1indicates that the PPDU is not a sounding PPDU.
                                       sounding PPDU is used for channel estimation by the request receiver */
    unsigned : 1; /**< reserved */
    unsigned aggregation: 1; /**< Aggregation. 0: MPDU packet; 1: AMPDU packet */
    unsigned stbc: 2; /**< Space Time Block Code(STBC). 0: non STBC packet; 1: STBC packet */
    unsigned fec_coding: 1; /**< Forward Error Correction(FEC). Flag is set for 11n packets which are LDPC */
    unsigned sgi: 1; /**< Short Guide Interval(SGI). 0: Long GI; 1: Short GI */

    unsigned : 8; /**< reserved */

    unsigned ampdu_cnt: 8; /**< the number of subframes aggregated in AMPDU */
    unsigned channel: 4; /**< primary channel on which this packet is received */
    unsigned secondary_channel: 4; /**< secondary channel on which this packet is received. 0: none; 1: above; 2: below */
    //unsigned : 8; /**< reserved */
    unsigned rxstart_time_cyc: 7; /**< Rx timestamp related special counter */
    unsigned : 1; /**< reserved */

    unsigned timestamp: 32; /**< timestamp. The local time when this packet is received. It is precise only if modem sleep or light sleep is not enabled. unit: microsecond */
    unsigned : 8; /**< reserved */
    unsigned rx_freq_local: 8; /**< reserved */
    unsigned : 16; /**< reserved */

    signed noise_floor: 8; /**< noise floor of Radio Frequency Module(RF). unit: dBm*/
    unsigned : 24; /**< reserved */
    unsigned : 32; /**< reserved */
    //unsigned : 31; /**< reserved */
    unsigned : 20; /**< reserved */
    unsigned rxstart_time_cyc_dec: 11; /**< Rx timestamp related special counter */

    unsigned ant: 1; /**< antenna number from which this packet is received. 0: WiFi antenna 0; 1: WiFi antenna 1 */

    unsigned : 32; /**< reserved */
    unsigned : 32; /**< reserved */
    unsigned : 32; /**< reserved */
    unsigned sig_len: 12; /**< length of packet including Frame Check Sequence(FCS) */
    unsigned : 12; /**< reserved */
    unsigned rx_state: 8; /**< state of the packet. 0: no error; others: error numbers which are not public */
} hacked_wifi_pkt_rx_ctrl_t;


#define CSI_PAYLOAD_SIZE 256 // Based on the esp-now rate options

struct csi_and_time {
    int64_t csi_recv_time_usec;
    int8_t csi_buf[CSI_PAYLOAD_SIZE];
};

struct dual_csi_and_time {
    struct csi_and_time request;
    struct csi_and_time response;
};


#define WIFI_PKT_RATE_IS_OFDM(_hctrl)     (((_hctrl)->sig_mode != 0) || ((_hctrl)->sig_mode == 0 && (_hctrl)->rate >= WIFI_PHY_RATE_48M))

#define WIFI_PKT_RX_CYCDEC_ADJUST(_ctrl)    ((((_ctrl)->rxstart_time_cyc_dec) >= 1024) ? (2048 - ((_ctrl)->rxstart_time_cyc_dec)) : ((_ctrl)->rxstart_time_cyc_dec))
#define WIFI_PKT_RX_TIMESTAMP_NSEC(_ctrl)   (((((uint64_t)(_ctrl)->timestamp)) * 1000L) + ((((uint64_t)(_ctrl)->rxstart_time_cyc) * 12500L) / 1000L) + ((((uint64_t)WIFI_PKT_RX_CYCDEC_ADJUST(_ctrl)) * 1562L) / 1000L) - (20800L))
#define WIFI_PKT_RX_TIMESTAMP_PSEC(_ctrl)   (((((uint64_t)(_ctrl)->timestamp)) * 1000000ULL) + ((((uint64_t)(_ctrl)->rxstart_time_cyc) * 12500ULL)) + ((((uint64_t)WIFI_PKT_RX_CYCDEC_ADJUST(_ctrl)) * 1562ULL)) - (20800ULL * 1000ULL))


void master();

void receiver();

void sender();

void sender_fast();

void infinite_ftm();

void infinite_send();

void respond_to_csi();

void send_angle_received();

void show_csi_summary();

void simple_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status);

void csi_ping_pong(void *ctx, wifi_csi_info_t *data);

void csi_send_summary(void *ctx, wifi_csi_info_t *data);

void promi_ftm_cb(void *buf, wifi_promiscuous_pkt_type_t type);

void print_now_recv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len);

#endif //PERF_H
