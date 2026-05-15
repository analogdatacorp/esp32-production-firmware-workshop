#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "button_interrupt";

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO    GPIO_NUM_2

static volatile int led_state = 0;

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
    ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, led_state));
}

static void button_init(void)
{
    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };

    ESP_ERROR_CHECK(gpio_config(&button_config));
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state);
}

void app_main(void)
{
    led_init();
    button_init();

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    ESP_ERROR_CHECK(gpio_isr_handler_add(
        BUTTON_GPIO,
        button_isr_handler,
        NULL
    ));

    ESP_LOGI(TAG, "Button interrupt experiment started");
    ESP_LOGI(TAG, "Press button on GPIO %d to toggle LED on GPIO %d",
             BUTTON_GPIO,
             LED_GPIO);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}