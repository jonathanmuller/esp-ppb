#include "esp_log.h"
#include "helper.h"
#include "esp_private/esp_clk.h"
#include "soc/rtc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include <string.h>
#include "esp_event.h"

#include "perf.h"
#include "ftm.h"

#include <math.h>
#include <limits.h>

#include "constant.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "i2c_helper.h"

#define FTM_FRM_CNT 8
#define ALLAN_DEV_PPB_PER_S 1.0
#define TS_ACC_NS 1.5


// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_wifi.html#_CPPv423wifi_ftm_report_entry_t
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html

#define ABS64(x) ((x) > 0 ? (x) : -(x))


bool check_csi_now_to_addr(const uint8_t *dmac, const uint8_t *payload, const uint8_t *expected_mac) {
    return (memcmp(expected_mac, dmac, 6) == 0) && (memcmp(payload, ESP_NOW_HEADER, sizeof(ESP_NOW_HEADER)) == 0);
}

static void IRAM_ATTR __attribute__((unused)) csi_angle(void *ctx, wifi_csi_info_t *data) {
    return;
    int64_t recv_time = WIFI_PKT_RX_TIMESTAMP_NSEC((hacked_wifi_pkt_rx_ctrl_t*)&data->rx_ctrl);

    if (check_csi_now_to_addr(data->dmac, data->payload, BROADCAST) == 0) {
        return;
    }


    printf("DelimiterStart %d secondary_channel %d  sig_mode %d bandwidth %d stbc %d Values ", data->rx_seq, data->rx_ctrl.secondary_channel, data->rx_ctrl.sig_mode, data->rx_ctrl.cwb, data->rx_ctrl.stbc);
    for (int i = 0; i < data->len; i++) {
        printf(" %d", data->buf[i]);
    }
    printf(" DelimiterStop\n");

    /*
    secondary_channel = None
    Signal mode = HT(11n)
    Channel bandwidth = 20MHz
    STBC = non-STBC
    */

    //ESP_LOG_BUFFER_HEX("TAGADA", (const uint8_t *)data->buf, data->len);
    ESP_LOGI(GET_AUTO_TAG(), "At time %llu angle %4.0f frame %d length %d", recv_time, atan2(data->buf[32+7], data->buf[32+7+1])*180/M_PI, data->rx_seq, data->payload_len);

    for (int i = 0; i < data->len; i += 2) {
        int8_t imag = data->buf[i];
        int8_t real = data->buf[i + 1];
        if (imag == 0 && real == 0) {
            printf("-");
        } else {
            printf("X");
        }
    }
    printf("\n");


    /*
    for (int i = 0; i < data->len; i += 2) {
        int8_t imag = data->buf[i];
        int8_t real = data->buf[i + 1];
        // Skip empty subcarriers
        if (imag == 0 && real == 0) {
            continue;
        }

        // atan2(y, x) -> atan2(Imaginary, Real)
        float angle = atan2f(imag, real);
        ESP_LOGI(GET_AUTO_TAG(), "At time %llu subcarrier %d angle %4.0f frame %d length %d", recv_time, i/2, angle*180/M_PI, data->rx_seq, data->payload_len);

        break; // Log only the first valid subcarrier to avoid flooding
    }
    */
}


#define FTM_TABLE_LEN 100

static struct ftm_exchange ftm_table[FTM_TABLE_LEN];
static int ftm_table_index_write = 0;


static inline bool ftm_exchange_valid(const struct ftm_exchange *e) {
    return (e->dialog_token != 0) && (e->tod != 0) && (e->recv_ts != 0);
}

static uint64_t u48_le(const uint8_t v[6]);

static void ftm_table_add(const struct ftm_dialog_t *dlg, int64_t recv_time_ps) {
    if (ftm_table_index_write > 0 && ftm_table[ftm_table_index_write - 1].dialog_token == dlg->followup_token) {
        ftm_table[ftm_table_index_write - 1].tod = u48_le(dlg->tod);
    }

    ftm_table[ftm_table_index_write].dialog_token = dlg->dialog_token;
    ftm_table[ftm_table_index_write].tod = 0;
    ftm_table[ftm_table_index_write].recv_ts = recv_time_ps;

    if (ftm_table_index_write < FTM_TABLE_LEN - 1) {
        ftm_table_index_write++;
    }
}


static int ftm_table_ppb_slope(double *slope_ppb_out, int64_t *span_ps_out, int *valid_out) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    double sum_x_x = 0.0;
    double sum_x_y = 0.0;
    int n = 0;

    *slope_ppb_out = 0.0;
    *span_ps_out = 0;
    *valid_out = 0;


    int base_idx = -1;
    for (int i = 0; i < ftm_table_index_write; i++) {
        if (ftm_exchange_valid(&ftm_table[i])) {
            base_idx = i;
            break;
        }
    }
    int valid_count = 0;
    for (int i = base_idx + 1; i < ftm_table_index_write; i++) {
        if (ftm_exchange_valid(&ftm_table[i])) {
            valid_count++;
        }
    }
    int valid_total = (base_idx >= 0) ? (valid_count + 1) : 0;
    *valid_out = valid_total;
    if (base_idx < 0 || valid_count < 1) {
        return -2;
    }

    int64_t base_recv = (int64_t) ftm_table[base_idx].recv_ts;
    int64_t base_tod = (int64_t) ftm_table[base_idx].tod;
    int64_t base_diff = base_recv - base_tod;
    int last_idx = base_idx;
    for (int i = base_idx + 1; i < ftm_table_index_write; i++) {
        if (ftm_exchange_valid(&ftm_table[i])) {
            last_idx = i;
        }
    }
    int64_t last_recv = (int64_t) ftm_table[last_idx].recv_ts;
    *span_ps_out = ABS64(last_recv - base_recv);

    for (int i = base_idx + 1; i < ftm_table_index_write; i++) {
        if (!ftm_exchange_valid(&ftm_table[i])) {
            continue;
        }
        int64_t recv = (int64_t) ftm_table[i].recv_ts;
        int64_t tod = (int64_t) ftm_table[i].tod;
        double x = (double) (recv - base_recv);
        double y = (double) ((recv - tod) - base_diff);
        sum_x += x;
        sum_y += y;
        sum_x_x += x * x;
        sum_x_y += x * y;
        n++;
    }

    double denom = (n * sum_x_x - sum_x * sum_x);
    if (denom == 0.0) {
        return -3;
    }

    double m = (n * sum_x_y - sum_x * sum_y) / denom;
    *slope_ppb_out = m * 1000000000.0;

    /*
    for (int i = 0; i < ftm_table_index_write; i++) {
        ESP_LOGI(GET_AUTO_TAG(), "DEBUG %d token %d recv %lld tod %lld", i, ftm_table[i].dialog_token, ftm_table[i].recv_ts, ftm_table[i].tod);
    }
    */

    memset(ftm_table, 0, sizeof(struct ftm_exchange) * FTM_TABLE_LEN);
    ftm_table_index_write = 0;

    return 0;
}


#define DIRECTION_FROM_ERROR(x) ((x) < 0.0 ? 1 : -1) // if error is positive, we need to lower the TCXO voltage

static const double SLOPE_THRESHOLD_PPB = 50.0;

static int voltage_correction_coarse = 0;
static int voltage_correction_fine = 0;

void apply_ppb_correction() {
    static double slope;
    int64_t span_ps = 0;
    int valid_total = 0;
    if (ftm_table_ppb_slope(&slope, &span_ps, &valid_total) != 0) {
        ESP_LOGI(GET_AUTO_TAG(), "Slope error");
        return;
    }


    if (fabs(slope) < SLOPE_THRESHOLD_PPB) {
        voltage_correction_fine += DIRECTION_FROM_ERROR(slope);
    } else {
        voltage_correction_coarse += DIRECTION_FROM_ERROR(slope);
    }
    i2c_set_dac(true, voltage_correction_coarse);
    i2c_set_dac(false, voltage_correction_fine);
    ESP_LOGI(GET_AUTO_TAG(), "Result : %+5.1f[ppb] valid=%d span=%.3fms DAC coarse correction is %d and fine is %d", slope, valid_total, (double)span_ps / 1000000000.0, voltage_correction_coarse, voltage_correction_fine);

    char oled_line1[16];
    char oled_line2[16];
    snprintf(oled_line1, sizeof(oled_line1), "PPB:%+.1f", slope);
    snprintf(oled_line2, sizeof(oled_line2), "Pkt: %d", valid_total);

    i2c_helper_oled_print_lines(oled_line1, oled_line2);
}

uint64_t last_ftm_frame_received_us = 0;
uint64_t last_tcxo_correction = 0;


void infinite_ftm() {
    wifi_ftm_initiator_cfg_t g_ftm_cfg = (wifi_ftm_initiator_cfg_t){
        .frm_count = FTM_FRM_CNT,
        .burst_period = 2, // 2->255
        .channel = MAIN_CHANNEL,
        .use_get_report_api = true, // We will call get_report in main loop
    };
    memcpy(g_ftm_cfg.resp_mac, MAC_AP_FTM, 6);


    while (true) {
        if (esp_timer_get_time() - last_ftm_frame_received_us > 300 * 1000) {
            ESP_ERROR_CHECK(esp_wifi_ftm_initiate_session(&g_ftm_cfg));
        }
        if (esp_timer_get_time() - last_tcxo_correction > 1000 * 1000) {
            apply_ppb_correction();
            last_tcxo_correction = esp_timer_get_time();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void IRAM_ATTR csi_ping_pong(void *ctx, wifi_csi_info_t *data) {
    if (check_csi_now_to_addr(data->dmac, data->payload, BROADCAST) == 0) {
        return;
    }
    static uint8_t empty = 0;
    ESP_ERROR_CHECK(esp_now_send(BROADCAST, &empty, sizeof(empty)));
    //ESP_LOGI(GET_AUTO_TAG(), "Received matching CSI with seq %u I will respond", data->rx_seq);
}

static struct dual_csi_and_time ping_pong;

void IRAM_ATTR csi_send_summary(void *ctx, wifi_csi_info_t *data) {
    int64_t now_usec = esp_timer_get_time();

    if (check_csi_now_to_addr(data->dmac, data->payload, BROADCAST) == true) {
        if (data->len != CSI_PAYLOAD_SIZE) {
            ESP_LOGI(GET_AUTO_TAG(), "Error, CSI expected to be %d but is %d", CSI_PAYLOAD_SIZE, data->len);
            return;
        }
        if (memcmp(data->mac, MAC_AP_FTM, 6) == 0) {
            memcpy(ping_pong.response.csi_buf, data->buf, CSI_PAYLOAD_SIZE);
            ping_pong.response.csi_recv_time_usec = now_usec;
            ESP_LOGI(GET_AUTO_TAG(), "Ping-pong took %lld[usec], I'll send result", ping_pong.response.csi_recv_time_usec-ping_pong.response.csi_recv_time_usec);
            //static uint8_t empty = 0;
            ESP_ERROR_CHECK(esp_now_send(BROADCAST, (uint8_t*)&ping_pong, sizeof(ping_pong)));
        } else {
            memcpy(ping_pong.request.csi_buf, data->buf, CSI_PAYLOAD_SIZE);
            ping_pong.request.csi_recv_time_usec = now_usec;
        }
    }
}

void print_now_recv(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
    ESP_LOGI(GET_AUTO_TAG(), "Received esp-now with length %d from %02X:%02X:%02X:%02X:%02X:%02X:", data_len,
             esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2], esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5]);
    if (data_len != sizeof(struct dual_csi_and_time)) {
        ESP_LOGI(GET_AUTO_TAG(), "ESP-NOW wrong size");
        return;
    }
    struct dual_csi_and_time *recv_ping_pong = (struct dual_csi_and_time *) data;
    // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html#wi-fi-channel-state-information

    /*
    For this specific config : (see wifi_tx_rate_config_t of heéper_init.c)
    Channel
        Secondary channel = none
    Packet info
        Signal mode : HT
        Bandwidth : 20mhz
        STBC : non-STBC
    Sub-carrier index
        LLTF : 0->31, -32->-1
        HT-LTF : 0->31, -32-> 1
        STBC-HT-LTF : /
    Total bytes : 256
    */


    for (int i = 64; i < 128; i += 2) {
        if ((recv_ping_pong->request.csi_buf[i] == 0) && (recv_ping_pong->request.csi_buf[i + 1]) == 0) {
            continue;
        }
        if ((recv_ping_pong->response.csi_buf[i] == 0) && (recv_ping_pong->response.csi_buf[i + 1]) == 0) {
            continue;
        }
        uint8_t *mac = esp_now_info->src_addr;
        double angle_diff = atan2(recv_ping_pong->request.csi_buf[i], recv_ping_pong->request.csi_buf[i + 1]) - atan2(recv_ping_pong->response.csi_buf[i], recv_ping_pong->response.csi_buf[i + 1]);
        ESP_LOGI(GET_AUTO_TAG(), "DEBUG %02X:%02X:%02X:%02X:%02X:%02X: offset %d angle diff %1.0f", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], i, angle_diff*180/M_PI) ;
        break;
    }


    printf("Angles : ");
    int8_t *ptr = recv_ping_pong->request.csi_buf;
    for (int j = 0; j < 1; j++) {
        for (int i = 0; i < 32; i++) {
            printf("%1.3f ", atan2(ptr[32 * 2 + i * 2], ptr[32 * 2 + i * 2 + 1]));
        }
        for (int i = 0; i < 32; i++) {
            printf("%1.3f ", atan2(ptr[i * 2], ptr[i * 2 + 1]));
        }
        printf("\n");
        ptr = recv_ping_pong->response.csi_buf;
    }
}


void simple_send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status) {
    ESP_LOGI(GET_AUTO_TAG(), "Sent at rate %d", tx_info->rate);
}


void IRAM_ATTR print_csi_angle(void *ctx, wifi_csi_info_t *data) {
    if (check_csi_now_to_addr(data->dmac, data->payload, BROADCAST) == 0) {
        return;
    }
    int offset = data->payload[CSI_PAYLOAD_ESP_NOW_OFFSET];
    printf("DelimiterStart %d secondary_channel %d  sig_mode %d bandwidth %d stbc %d", data->rx_seq, data->rx_ctrl.secondary_channel, data->rx_ctrl.sig_mode, data->rx_ctrl.cwb, data->rx_ctrl.stbc);
    ESP_LOGI(GET_AUTO_TAG(), "CSI data %d of length %d", offset, data->len);
}

void show_csi_summary() {
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(print_csi_angle, NULL));
}

void infinite_send() {
    //ESP_ERROR_CHECK(esp_wifi_set_mac(ESP_IF_WIFI_STA, MAC_STA_X));

    while (true) {
        uint8_t empty;
        ESP_LOGI(GET_AUTO_TAG(), "Will send");

        ESP_ERROR_CHECK(esp_now_send(BROADCAST, &empty, sizeof(empty)));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


static void IRAM_ATTR __attribute__((unused)) simple_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int data_len) {
    static uint8_t last_count = 0;
    uint8_t current_count = data[0];

    ESP_LOGI(GET_AUTO_TAG(), "ESP-NOW recv %d of length %d with rssi %d from %02X:%02X:%02X:%02X:%02X:%02X => diff = %d", current_count, data_len, esp_now_info->rx_ctrl->rssi,
             esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2], esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5],
             current_count-last_count-1);
    last_count = current_count;
}

static uint64_t u48_le(const uint8_t v[6]) {
    uint64_t out = 0;
    for (int i = 0; i < 6; i++) {
        out += ((uint64_t) v[i]) << (8 * i);
    }
    return out;
}


void promi_ftm_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) {
        return;
    }

    const wifi_promiscuous_pkt_t *ppkt = (const wifi_promiscuous_pkt_t *) buf;

    if (!WIFI_PKT_RATE_IS_OFDM((hacked_wifi_pkt_rx_ctrl_t*)&ppkt->rx_ctrl)) {
        return;
    }
    int64_t recv_time_ps = WIFI_PKT_RX_TIMESTAMP_PSEC((hacked_wifi_pkt_rx_ctrl_t*)&ppkt->rx_ctrl);


    const wifi_ieee80211_hdr_t *hdr = (const wifi_ieee80211_hdr_t *) ppkt->payload;
    const uint8_t *payload = ((const uint8_t *) ppkt->payload) + sizeof(*hdr);

    if ((hdr->frame_ctrl & 0x00FC) != 0x00D0) {
        return;
    }

    if (memcmp(hdr->transmitter_addr, MAC_AP_FTM, sizeof(MAC_AP_FTM)) != 0) {
        return;
    }


    const struct ftm_action_hdr_t *action = (const struct ftm_action_hdr_t *) payload;
    if ((action->category != 0x04) || (action->action != 0x21)) {
        //ESP_LOGI(GET_AUTO_TAG(), "Unknown cat=%02X action=%02X", action->category, action->action);
        return;
    }


    const struct ftm_dialog_t *dlg = (const struct ftm_dialog_t *) payload;

    if (dlg->dialog_token == 0) {
        //ESP_LOGW(GET_AUTO_TAG(), "Invalid dialog token (FTM termination frame)");
        return;
    }

    /*
    const uint64_t tod = u48_le(dlg->tod);
    const uint64_t toa = u48_le(dlg->toa);
    */

    ftm_table_add(dlg, recv_time_ps);
    last_ftm_frame_received_us = esp_timer_get_time();
}
