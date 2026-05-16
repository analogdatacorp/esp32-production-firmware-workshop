#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

/*
 * Experiment 05:
 * Race Condition Demo
 *
 * Concept:
 * Two tasks are modifying the same global variable.
 *
 * Without mutex:
 * - Both tasks access the variable simultaneously
 * - Data corruption occurs
 * - Final value becomes unpredictable
 *
 * With mutex:
 * - Only one task accesses variable at a time
 * - Final result is always correct
 *
 * Change this value:
 * 0 = WITHOUT mutex (race condition)
 * 1 = WITH mutex (safe access)
 */

#define USE_MUTEX 1

/*
 * Both tasks together should increment:
 *
 * 100000 + 100000 = 200000
 */
#define INCREMENT_COUNT 100000

static const char *TAG = "RACE_CONDITION";

/*
 * Shared global variable.
 *
 * volatile tells compiler:
 * "This value may change unexpectedly."
 *
 * Very important in embedded systems and RTOS.
 */
static volatile uint32_t shared_counter = 0;

/*
 * Mutex for protecting shared_counter.
 */
static SemaphoreHandle_t counter_mutex = NULL;

/*
 * Increment task.
 *
 * Both tasks run this same function.
 */
static void increment_task(void *arg)
{
    const char *task_name = (const char *)arg;

    ESP_LOGI(task_name, "Started");

    for (uint32_t i = 0; i < INCREMENT_COUNT; i++) {

#if USE_MUTEX

        /*
         * Take mutex before modifying shared variable.
         *
         * Only one task can enter this section at a time.
         */
        xSemaphoreTake(counter_mutex, portMAX_DELAY);

        shared_counter++;

        /*
         * Release mutex after update.
         */
        xSemaphoreGive(counter_mutex);

#else

        /*
         * RACE CONDITION:
         *
         * shared_counter++ is NOT atomic.
         *
         * CPU internally performs:
         * 1. Read value
         * 2. Increment value
         * 3. Write value back
         *
         * Another task can interrupt in between.
         *
         * Result:
         * Lost increments.
         */

        shared_counter++;

#endif

        /*
         * Small periodic yield.
         *
         * Makes race condition easier to observe.
         */
        if ((i % 1000) == 0) {
            taskYIELD();
        }
    }

    ESP_LOGI(task_name,
             "Finished. Current Counter = %lu",
             (unsigned long)shared_counter);

    /*
     * Delete task after completion.
     */
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Experiment 05: Race Condition Demo");

#if USE_MUTEX
    ESP_LOGI(TAG, "Mode: WITH mutex");
#else
    ESP_LOGW(TAG, "Mode: WITHOUT mutex");
#endif

    /*
     * Reset shared counter.
     */
    shared_counter = 0;

#if USE_MUTEX

    /*
     * Create mutex.
     */
    counter_mutex = xSemaphoreCreateMutex();

    if (counter_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }

#endif

    /*
     * Create Task 1
     */
    xTaskCreate(
        increment_task,
        "increment_task_1",
        4096,
        "TASK_1",
        5,
        NULL
    );

    /*
     * Create Task 2
     */
    xTaskCreate(
        increment_task,
        "increment_task_2",
        4096,
        "TASK_2",
        5,
        NULL
    );

    /*
     * Wait before checking final result.
     *
     * Both tasks should finish within this time.
     */
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG,
             "EXPECTED COUNTER = %d",
             INCREMENT_COUNT * 2);

    ESP_LOGI(TAG,
             "ACTUAL COUNTER   = %lu",
             (unsigned long)shared_counter);

#if USE_MUTEX
    ESP_LOGI(TAG, "Mutex protected the shared variable successfully");
#else
    ESP_LOGW(TAG, "Race condition occurred - result may be wrong");
#endif
}