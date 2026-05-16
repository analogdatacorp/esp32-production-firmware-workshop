#include <stdio.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "i2c_scanner";

#define I2C_MASTER_PORT      I2C_NUM_0

// Stick to SDA GPIO21 and SCL GPIO22 for I2C on ESP32, but you can change these if needed
// Note: Some ESP32 boards have default I2C pins (SDA=21, SCL=22), but you can use other GPIOs if your board supports it.
// i2c documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html#_CPPv4N3i2c_master_bus_config_tE
// #define I2C_MASTER_SDA_IO    GPIO_NUM_21
// #define I2C_MASTER_SCL_IO    GPIO_NUM_22

#define I2C_MASTER_SDA_IO    GPIO_NUM_8
#define I2C_MASTER_SCL_IO    GPIO_NUM_9


#define I2C_TIMEOUT_MS       50

static i2c_master_bus_handle_t i2c_bus_handle = NULL;

static void i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    ESP_LOGI(TAG, "I2C master initialized on SDA GPIO%d, SCL GPIO%d",
             I2C_MASTER_SDA_IO,
             I2C_MASTER_SCL_IO);
}

static void i2c_scan(void)
{
    int devices_found = 0;

    ESP_LOGI(TAG, "Scanning I2C bus...");

    for (uint8_t address = 0x08; address <= 0x77; address++)
    {
        esp_err_t ret = i2c_master_probe(
            i2c_bus_handle,
            address,
            I2C_TIMEOUT_MS
        );

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "I2C device found at address 0x%02X", address);
            devices_found++;
        }
    }

    if (devices_found == 0)
    {
        ESP_LOGW(TAG, "No I2C devices found");
    }
    else
    {
        ESP_LOGI(TAG, "I2C scan complete. Devices found: %d", devices_found);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "I2C Scanner Started");

    i2c_master_init();
    i2c_scan();

    ESP_LOGI(TAG, "I2C Scanner Finished");
}