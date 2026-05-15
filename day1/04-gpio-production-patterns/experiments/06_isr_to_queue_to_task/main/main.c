#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "isr_queue_blink";

#define BUTTON_GPIO GPIO_NUM_4
#define LED_GPIO    GPIO_NUM_2

#define GPIO_QUEUE_LENGTH 10
#define LED_BLINK_COUNT   3
#define LED_BLINK_DELAY_MS 200

static QueueHandle_t gpio_evt_queue = NULL;

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

static void led_blink_multiple_times(int count)
{
    for (int i = 0; i < count; i++)
    {
        ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 1));
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));

        ESP_ERROR_CHECK(gpio_set_level(LED_GPIO, 0));
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
    }
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_event_task(void *arg)
{
    uint32_t gpio_num;

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY))
        {
            if (gpio_num == BUTTON_GPIO)
            {
                ESP_LOGI(TAG, "Button interrupt received from GPIO %lu", gpio_num);
                ESP_LOGI(TAG, "Blinking LED %d times inside task", LED_BLINK_COUNT);

                led_blink_multiple_times(LED_BLINK_COUNT);

                ESP_LOGI(TAG, "LED blink completed");
            }
        }
    }
}

void app_main(void)
{
    led_init();
    button_init();

    gpio_evt_queue = xQueueCreate(GPIO_QUEUE_LENGTH, sizeof(uint32_t));

    if (gpio_evt_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create GPIO event queue");
        return;
    }

    xTaskCreate(
        gpio_event_task,
        "gpio_event_task",
        2048,
        NULL,
        10,
        NULL
    );

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    ESP_ERROR_CHECK(gpio_isr_handler_add(
        BUTTON_GPIO,
        button_isr_handler,
        (void *)BUTTON_GPIO
    ));

    ESP_LOGI(TAG, "ISR to Queue to Task LED blink experiment started");
    ESP_LOGI(TAG, "Press button on GPIO %d to blink LED on GPIO %d",
             BUTTON_GPIO,
             LED_GPIO);
}