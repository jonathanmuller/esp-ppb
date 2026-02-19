#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"
#include "i2c_helper.h"
#include "helper.h"
#include "font.h"

// Shared I2C master bus handle
static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;

static esp_err_t i2c_helper_master_setup(void) {
    if (s_i2c_bus_handle) {
        return ESP_OK;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_HELPER_PORT,
        .sda_io_num = I2C_HELPER_SDA_PIN,
        .scl_io_num = I2C_HELPER_SCL_PIN,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
    };
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.flags.enable_internal_pullup = 1;

    return i2c_new_master_bus(&bus_cfg, &s_i2c_bus_handle);
}

static bool check_device(uint8_t addr, const char *name) {
    ESP_LOGI(GET_AUTO_TAG(), "Checking %s...", name);
    if (i2c_master_probe(s_i2c_bus_handle, addr, 50) == ESP_OK) {
        ESP_LOGI(GET_AUTO_TAG(), "%s ok", name);
        return true;
    } else {
        ESP_LOGE(GET_AUTO_TAG(), "%s MISSING (0x%02X)", name, addr);
        return false;
    }
}

static int read_adc_gpio3_batt_mv(void) {
    adc_oneshot_unit_handle_t adc_handle = NULL;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    if (adc_oneshot_new_unit(&init_cfg, &adc_handle) != ESP_OK) {
        return -1;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    if (adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_3, &chan_cfg) != ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        return -1;
    }

    int raw = 0;
    if (adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &raw) != ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        return -1;
    }
    adc_oneshot_del_unit(adc_handle);

    // GPIO sees Vbatt * (100k / (100k + 100k)) = Vbatt / 2
    // So Vbatt = Vgpio * 2
    const int adc_max = (1 << 12) - 1;
    const int vref_mv = 3300;
    return (raw * (vref_mv * 2)) / adc_max;
}

static bool is_5v_present_gpio20(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << GPIO_NUM_20,
        .pull_down_en = 1,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    return gpio_get_level(GPIO_NUM_20) != 0;
}

bool i2c_check_hardware(void) {
    if (i2c_helper_master_setup() != ESP_OK) {
        ESP_LOGE(GET_AUTO_TAG(), "Failed to init I2C master");
        return false;
    }

    // 1. Check Screen (0x3C)
    if (!check_device(I2C_ADDR_SCREEN, "screen")) return false;

    // 2. Check DAC-MSB (A0=GND -> 0x60)
    if (!check_device(I2C_ADDR_DAC_MSB, "DAC-MSB")) return false;

    // 3. Check DAC-LSB (A0=VCC -> 0x61)
    if (!check_device(I2C_ADDR_DAC_LSB, "DAC-LSB")) return false;

    int batt_mv = read_adc_gpio3_batt_mv();

    ESP_LOGI(GET_AUTO_TAG(), "Battery: %d mV", batt_mv);


    bool v5_present = is_5v_present_gpio20();
    ESP_LOGI(GET_AUTO_TAG(), "5V present: %s", v5_present ? "YES" : "NO");

    return true;
}

void i2c_helper_scan_all(void) {
    if (i2c_helper_master_setup() != ESP_OK) {
        ESP_LOGE(GET_AUTO_TAG(), "Failed to init master for scan");
        return;
    }

    ESP_LOGI(GET_AUTO_TAG(), "Scanning I2C bus (SDA=%d, SCL=%d)...", I2C_HELPER_SDA_PIN, I2C_HELPER_SCL_PIN);

    for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
        esp_err_t ret = i2c_master_probe(s_i2c_bus_handle, addr, 50);
        if (ret == ESP_OK) {
            ESP_LOGI(GET_AUTO_TAG(), "Found device at 0x%02X", addr);
        }
    }
    ESP_LOGI(GET_AUTO_TAG(), "Scan complete.");
}

// ---- DAC Helpers ----

// value: 0 is Vcc/2 (Code 2048)
// +1 increment = +1 DAC LSB
esp_err_t i2c_set_dac(bool is_coarse, int32_t value) {
    if (i2c_helper_master_setup() != ESP_OK) return ESP_FAIL;

    // Center point is 2048 (Vcc/2)
    int32_t raw_code = 2048 + value;

    // Clamp to 12-bit range (0-4095)
    if (raw_code < 0) raw_code = 0;
    if (raw_code > 4095) raw_code = 4095;

    uint16_t code = (uint16_t) raw_code;
    uint8_t addr = is_coarse ? I2C_ADDR_DAC_MSB : I2C_ADDR_DAC_LSB;

    // MCP4725 Fast Mode Write Command
    // Byte 1: 0 0 PD1 PD0 D11 D10 D9 D8
    // Byte 2: D7 D6 D5 D4 D3 D2 D1 D0
    uint8_t data[2];
    data[0] = (code >> 8) & 0x0F; // Upper 4 bits
    data[1] = code & 0xFF; // Lower 8 bits

    // Add device temporarily for the transaction
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_HELPER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) return ret;

    ret = i2c_master_transmit(dev_handle, data, 2, 50);

    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

// ---- OLED Helpers (SSD1306) ----

#define OLED_COLS 128
#define OLED_PAGES 8
#define OLED_LINE_PAGES 4
#define OLED_H_SCALE 2
#define OLED_H_SPACE 1

static esp_err_t i2c_oled_send(const uint8_t *data, size_t len) {
    if (i2c_helper_master_setup() != ESP_OK) return ESP_FAIL;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR_SCREEN,
        .scl_speed_hz = I2C_HELPER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) return ret;

    ret = i2c_master_transmit(dev_handle, data, len, 50);

    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

static esp_err_t i2c_oled_write_cmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd}; // Co=0, D/C#=0 (Command)
    return i2c_oled_send(data, 2);
}

static esp_err_t i2c_oled_write_data(const uint8_t *payload, size_t len) {
    // We must prefix with 0x40 (Co=0, D/C#=1 -> Data)
    const size_t max_chunk = OLED_COLS;
    uint8_t buffer[1 + OLED_COLS];
    size_t offset = 0;
    esp_err_t ret = ESP_OK;

    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > max_chunk) chunk = max_chunk;
        buffer[0] = 0x40;
        memcpy(buffer + 1, payload + offset, chunk);
        ret = i2c_oled_send(buffer, chunk + 1);
        if (ret != ESP_OK) return ret;
        offset += chunk;
    }

    return ret;
}

static const uint8_t oled_init_cmds[] = {
    0xAE, // Display OFF
    0x20, 0x00, // Memory Addressing Mode: Horizontal
    0xB0, // Page Start Address (Page 0)
    0xC8, // COM Output Scan Direction (Normal)
    0x00, // Low Column Address
    0x10, // High Column Address
    0x40, // Start Line Address
    0x81, 0xFF, // Contrast: Max
    0xA1, // Segment Re-map
    0xA6, // Normal Display
    0xA8, 0x3F, // Multiplex Ratio
    0xA4, // Output follows RAM content
    0xD3, 0x00, // Display Offset
    0xD5, 0xF0, // Clock Divide Ratio
    0xD9, 0x22, // Pre-charge Period
    0xDA, 0x12, // COM Pins HW Config
    0xDB, 0x20, // VCOMh Deselect Level
    0x8D, 0x14, // Charge Pump Enable
    0xAF // Display ON
};

static bool oled_initialized = false;
static uint8_t oled_shadow[OLED_PAGES][OLED_COLS];

static void oled_set_window(uint8_t col_start, uint8_t col_end, uint8_t page_start, uint8_t page_end) {
    i2c_oled_write_cmd(0x21);
    i2c_oled_write_cmd(col_start);
    i2c_oled_write_cmd(col_end);
    i2c_oled_write_cmd(0x22);
    i2c_oled_write_cmd(page_start);
    i2c_oled_write_cmd(page_end);
}

static void oled_clear_pages(int page_start, int page_count) {
    oled_set_window(0, OLED_COLS - 1, page_start, page_start + page_count - 1);
    uint8_t clear_buf[OLED_COLS];
    memset(clear_buf, 0, sizeof(clear_buf));
    for (int p = 0; p < page_count; p++) {
        i2c_oled_write_data(clear_buf, OLED_COLS);
        memset(oled_shadow[page_start + p], 0, OLED_COLS);
    }
}

static void oled_init_once(void) {
    if (oled_initialized) return;

    for (int i = 0; i < sizeof(oled_init_cmds); i++) {
        i2c_oled_write_cmd(oled_init_cmds[i]);
    }

    oled_clear_pages(0, OLED_PAGES);
    oled_initialized = true;
}

static void oled_write_page_diff(int page, const uint8_t *buf) {
    int col = 0;
    while (col < OLED_COLS) {
        while (col < OLED_COLS && buf[col] == oled_shadow[page][col]) {
            col++;
        }
        if (col >= OLED_COLS) break;

        int start = col;
        while (col < OLED_COLS && buf[col] != oled_shadow[page][col]) {
            col++;
        }
        int end = col - 1;

        oled_set_window(start, end, page, page);
        i2c_oled_write_data(&buf[start], (size_t) (end - start + 1));
        memcpy(&oled_shadow[page][start], &buf[start], (size_t) (end - start + 1));
    }
}

static void oled_print_line_scaled(const char *text, int page_base) {
    if (!text) return;

    int num_chars = strlen(text);
    for (int page_row = 0; page_row < OLED_LINE_PAGES; page_row++) {
        uint8_t line_buf[OLED_COLS];
        memset(line_buf, 0, sizeof(line_buf));
        int x = 0;

        for (int c = 0; c < num_chars; c++) {
            uint8_t ascii = (uint8_t) text[c];
            if (ascii < 32 || ascii > 126) ascii = 32;

            const uint8_t *font_data = font5x7[ascii - 32];
            int len = 5;

            for (int col = 0; col < len; col++) {
                if (x >= OLED_COLS) break;
                uint8_t original = font_data[col];
                uint8_t expanded = 0;

                if (original & (1 << (page_row * 2))) expanded |= 0x0F;
                if (original & (1 << (page_row * 2 + 1))) expanded |= 0xF0;

                for (int sx = 0; sx < OLED_H_SCALE && x < OLED_COLS; sx++) {
                    line_buf[x++] = expanded;
                }
            }

            for (int sx = 0; sx < OLED_H_SPACE && x < OLED_COLS; sx++) {
                line_buf[x++] = 0x00;
            }
            if (x >= OLED_COLS) break;
        }

        oled_write_page_diff(page_base + page_row, line_buf);
    }
}

void i2c_helper_oled_init(void) {
    oled_init_once();
}

void i2c_helper_oled_print_string(const char *text) {
    if (!text) return;

    oled_init_once();
    oled_print_line_scaled(text, 0);
}

void i2c_helper_oled_print_lines(const char *line1, const char *line2) {
    if (!line1 && !line2) return;

    oled_init_once();
    if (line1) oled_print_line_scaled(line1, 0);
    if (line2) oled_print_line_scaled(line2, OLED_LINE_PAGES);
}
