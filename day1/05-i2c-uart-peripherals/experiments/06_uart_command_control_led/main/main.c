/*
 * UART LED Control — Experiment 06
 * Send 'A' to turn LED ON
 * Send 'B' to turn LED OFF
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "uart_led";

#define LED_GPIO          GPIO_NUM_2

#define UART_PORT         UART_NUM_0
#define UART_BAUD_RATE    115200
#define UART_BUF_SIZE     1024

static void led_init(void)
{
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&led_config));
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 0));

    ESP_LOGI(TAG, "LED initialized on GPIO%d", LED_GPIO);
}

static void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
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

    ESP_LOGI(TAG, "UART%d initialized at %d baud", UART_PORT, UART_BAUD_RATE);
}

static void uart_send_string(const char *message)
{
    uart_write_bytes(UART_PORT, message, strlen(message));
}

void app_main(void)
{
    led_init();
    uart_init();

    uart_send_string("\r\n========================================\r\n");
    uart_send_string("UART LED Control — Experiment 06\r\n");
    uart_send_string("Send A -> LED ON\r\n");
    uart_send_string("Send B -> LED OFF\r\n");
    uart_send_string("========================================\r\n\r\n");

    uint8_t rx_byte;

    while (1) {
        int len = uart_read_bytes(
            UART_PORT,
            &rx_byte,
            1,
            pdMS_TO_TICKS(100)
        );

        if (len > 0) {
            if (rx_byte == 'A' || rx_byte == 'a') {
                ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 1));
                uart_send_string("LED ON\r\n");
                ESP_LOGI(TAG, "Received A -> LED ON");
            }
            else if (rx_byte == 'B' || rx_byte == 'b') {
                ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 0));
                uart_send_string("LED OFF\r\n");
                ESP_LOGI(TAG, "Received B -> LED OFF");
            }
            else {
                uart_send_string("Unknown command. Send A or B\r\n");
                ESP_LOGW(TAG, "Unknown command received: %c", rx_byte);
            }
        }
    }
}