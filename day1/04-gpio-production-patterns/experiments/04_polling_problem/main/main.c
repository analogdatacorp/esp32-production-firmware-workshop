#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "polling_problem";

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO    GPIO_NUM_2

#define POLL_DELAY_MS 10
#define REPORT_INTERVAL_MS 1000

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

static void led_set(int state)
{
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, state));
}

static int button_read(void)
{
    return gpio_get_level(BUTTON_GPIO);
}

void app_main(void)
{
    led_init();
    button_init();

    int poll_count = 0;
    int last_button_state = -1;

    TickType_t last_report_time = xTaskGetTickCount();

    ESP_LOGI(TAG, "Polling problem demo started");

    while (1)
    {
        int button_state = button_read();

        poll_count++;

        led_set(button_state);

        if (button_state != last_button_state)
        {
            ESP_LOGI(TAG, "Button changed: %s",
                     button_state ? "PRESSED" : "RELEASED");

            last_button_state = button_state;
        }

        TickType_t now = xTaskGetTickCount();

        if ((now - last_report_time) >= pdMS_TO_TICKS(REPORT_INTERVAL_MS))
        {
            ESP_LOGI(TAG, "Button checked %d times in the last 1 second",
                     poll_count);

            poll_count = 0;
            last_report_time = now;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
    }
}