#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "button_led_polling";

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO    GPIO_NUM_2

#define POLL_DELAY_MS 50

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

    ESP_LOGI(TAG, "Button LED polling experiment started");

    while (1)
    {
        int button_state = button_read();

        led_set(button_state);

        ESP_LOGI(TAG, "Button: %s | LED: %s",
                 button_state ? "PRESSED" : "RELEASED",
                 button_state ? "ON" : "OFF");

        vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
    }
}