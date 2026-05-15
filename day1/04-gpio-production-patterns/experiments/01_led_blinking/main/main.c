#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "gpio_led_blink";

#define LED_GPIO GPIO_NUM_2
#define LED_BLINK_DELAY_MS 500

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
}

static void led_set(int state)
{
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, state));
}

void app_main(void)
{
    led_init();

    while (1)
    {
        led_set(1);
        ESP_LOGI(TAG, "LED ON");
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));

        led_set(0);
        ESP_LOGI(TAG, "LED OFF");
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
    }
}