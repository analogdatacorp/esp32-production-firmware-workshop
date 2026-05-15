#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "button_pulldown";

#define BUTTON_GPIO GPIO_NUM_4
#define POLL_DELAY_MS 200

static void button_init(void)
{
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(gpio_config(&button_config));
}

void app_main(void)
{
    button_init();

    ESP_LOGI(TAG, "Button pull-down experiment started");

    while (1)
    {
        int button_state = gpio_get_level(BUTTON_GPIO);

        if (button_state == 1)
        {
            ESP_LOGI(TAG, "Button Pressed");
        }
        else
        {
            ESP_LOGI(TAG, "Button Released");
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
    }
}