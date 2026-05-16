#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"

#define LED1_GPIO GPIO_NUM_2
#define LED2_GPIO GPIO_NUM_4

static const char *TAG = "FREE_RTOS_EXP_01";

static void configure_leds(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_GPIO) | (1ULL << LED2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);
}

void led1_task(void *arg)
{
    while (1) {
        gpio_set_level(LED1_GPIO, 1);
        ESP_LOGI(TAG, "LED 1 ON");
        vTaskDelay(pdMS_TO_TICKS(500));

        gpio_set_level(LED1_GPIO, 0);
        ESP_LOGI(TAG, "LED 1 OFF");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void led2_task(void *arg)
{
    while (1) {
        gpio_set_level(LED2_GPIO, 1);
        ESP_LOGI(TAG, "LED 2 ON");
        vTaskDelay(pdMS_TO_TICKS(1200));

        gpio_set_level(LED2_GPIO, 0);
        ESP_LOGI(TAG, "LED 2 OFF");
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Experiment 01: Two Tasks, Two LEDs");

    configure_leds();

    xTaskCreate(
        led1_task,
        "led1_task",
        2048,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        led2_task,
        "led2_task",
        2048,
        NULL,
        5,
        NULL
    );
}