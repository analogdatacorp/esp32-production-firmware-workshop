#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

/*
 * Experiment 03:
 * Queue - Producer Consumer
 *
 * Concept:
 * One task produces data.
 * Another task consumes data.
 * Queue safely transfers complete data between both tasks.
 *
 * Producer:
 * - Creates a counter value every 500 ms
 * - Sends it to queue
 *
 * Consumer:
 * - Receives data from queue
 * - Prints it
 * - Runs slower than producer
 *
 * Teaching:
 * Producer and consumer do not directly depend on each other.
 * Queue acts as a safe buffer between both tasks.
 */

#define QUEUE_LENGTH 5

static const char *TAG_PRODUCER = "PRODUCER";
static const char *TAG_CONSUMER = "CONSUMER";

typedef struct {
    uint32_t count;
    int32_t value;
} queue_item_t;

static QueueHandle_t data_queue = NULL;

static void producer_task(void *arg)
{
    queue_item_t item = {
        .count = 0,
        .value = 0
    };

    while (1) {
        item.count++;
        item.value = item.count * 10;

        /*
         * Send item to queue.
         *
         * Third argument = 0 means:
         * Do not wait if queue is full.
         *
         * If queue is full, xQueueSend returns pdFALSE.
         */
        if (xQueueSend(data_queue, &item, 0) == pdTRUE) {
            ESP_LOGI(TAG_PRODUCER,
                     "Sent: count=%lu value=%ld",
                     (unsigned long)item.count,
                     (long)item.value);
        } else {
            ESP_LOGW(TAG_PRODUCER,
                     "Queue full. Dropped: count=%lu value=%ld",
                     (unsigned long)item.count,
                     (long)item.value);
        }

        /*
         * Producer generates data every 500 ms.
         */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void consumer_task(void *arg)
{
    queue_item_t received_item;

    while (1) {
        /*
         * Receive data from queue.
         *
         * portMAX_DELAY means:
         * Wait here forever until data is available.
         *
         * This is efficient.
         * The task does not waste CPU while waiting.
         */
        if (xQueueReceive(data_queue, &received_item, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG_CONSUMER,
                     "Received: count=%lu value=%ld",
                     (unsigned long)received_item.count,
                     (long)received_item.value);
        }

        /*
         * Consumer is intentionally slower than producer.
         *
         * Producer delay = 500 ms
         * Consumer delay = 800 ms
         *
         * This allows students to see the queue buffering effect.
         */
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

void app_main(void)
{
    ESP_LOGI("APP", "Experiment 03: Queue Producer Consumer");

    /*
     * Create queue.
     *
     * QUEUE_LENGTH = maximum number of items queue can hold.
     * sizeof(queue_item_t) = size of each item.
     */
    data_queue = xQueueCreate(QUEUE_LENGTH, sizeof(queue_item_t));

    if (data_queue == NULL) {
        ESP_LOGE("APP", "Failed to create queue");
        return;
    }

    ESP_LOGI("APP", "Queue created successfully");

    xTaskCreate(
        producer_task,
        "producer_task",
        2048,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        consumer_task,
        "consumer_task",
        2048,
        NULL,
        5,
        NULL
    );
}