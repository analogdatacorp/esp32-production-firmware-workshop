/*
 * UART Basics — Experiment 05
 * ESP32 Production Firmware Workshop
 * Analog Data | analogdata.io
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "uart_basics";

#define UART_PORT        UART_NUM_0
#define UART_BAUD_RATE   115200
#define UART_BUF_SIZE    1024

static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));

    ESP_ERROR_CHECK(uart_driver_install(
        UART_PORT,
        UART_BUF_SIZE,
        0,
        0,
        NULL,
        0
    ));

    ESP_LOGI(TAG, "UART%d initialized at %d baud",
             UART_PORT,
             UART_BAUD_RATE);
}

static void uart_send_string(const char *message)
{
    uart_write_bytes(UART_PORT, message, strlen(message));
}

void app_main(void)
{
    uart_init();

    ESP_LOGI(TAG, "UART Basics Experiment Started");

    uart_send_string("\r\n========================================\r\n");
    uart_send_string("UART Basics — Experiment 05\r\n");
    uart_send_string("Analog Data | analogdata.io\r\n");
    uart_send_string("========================================\r\n");

    int counter = 0;
    char tx_buffer[128];

    while (1) {
        snprintf(
            tx_buffer,
            sizeof(tx_buffer),
            "UART message count: %d\r\n",
            counter++
        );

        uart_send_string(tx_buffer);

        ESP_LOGI(TAG, "Sent UART message");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}