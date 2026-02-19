#ifndef I2C_HELPER_H
#define I2C_HELPER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"

// I2C configuration for ESP32-C3
#define I2C_HELPER_PORT      0
#define I2C_HELPER_SDA_PIN   GPIO_NUM_6
#define I2C_HELPER_SCL_PIN   GPIO_NUM_7
#define I2C_HELPER_FREQ_HZ   400000

// Device Addresses
#define I2C_ADDR_SCREEN      0x3C // HS91L02W2C01
#define I2C_ADDR_DAC_MSB     0x60 // MCP4725 with A0=GND
#define I2C_ADDR_DAC_LSB     0x61 // MCP4725 with A0=VCC

// Checks for the presence of the Screen, DAC-MSB, and DAC-LSB.
bool i2c_check_hardware(void);

// Generic scan function
void i2c_helper_scan_all(void);

// Unified DAC Setter
// is_coarse: true for MSB (0x60), false for LSB (0x61)
// value: Signed offset from mid-scale (0 = 1.65V/Code 2048)
//        Range: -2048 (0V) to +2047 (3.3V)
esp_err_t i2c_set_dac(bool is_coarse, int32_t value);

// Initialize OLED once at boot.
void i2c_helper_oled_init(void);

// Print a string on the OLED screen (scaled)
void i2c_helper_oled_print_string(const char *text);

// Print two lines on the OLED screen (scaled)
void i2c_helper_oled_print_lines(const char *line1, const char *line2);

#endif // I2C_HELPER_H
