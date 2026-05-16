#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

/*
 * Experiment 02:
 * Priority Starvation in FreeRTOS
 *
 * Concept:
 * A higher priority task can starve lower priority tasks
 * if it never blocks, never delays, and never yields.
 *
 * In this experiment:
 * - Task A has priority 10
 * - Task B has priority 5
 * - Task C has priority 1
 *
 * First, Task A runs without delay.
 * Result:
 * Only Task A gets CPU time.
 *
 * Then enable the FIXED version by changing USE_FIXED_TASK_A to 1.
 * Result:
 * Task A yields using vTaskDelay(), so Task B and Task C also run.
 */

#define USE_FIXED_TASK_A 1

static const char *TAG_A = "TASK_A";
static const char *TAG_B = "TASK_B";
static const char *TAG_C = "TASK_C";

/*
 * High priority task WITHOUT delay.
 *
 * This task continuously runs forever.
 * Since it has the highest priority and never blocks,
 * lower priority tasks may not get CPU time.
 */
static void task_a_starving(void *arg)
{
    while (1) {
        ESP_LOGI(TAG_A, "High priority task running - no delay");

        /*
         * No vTaskDelay() here.
         *
         * This is intentionally wrong.
         * The task does not yield CPU time.
         */
    }
}

/*
 * High priority task WITH delay.
 *
 * This is the fixed version.
 * vTaskDelay() puts this task into blocked state temporarily.
 * During that time, lower priority tasks get CPU time.
 */
static void task_a_fixed(void *arg)
{
    while (1) {
        ESP_LOGI(TAG_A, "High priority task running - with delay");

        /*
         * This is not just a timer.
         * This gives the scheduler a chance to run other tasks.
         */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*
 * Medium priority task.
 *
 * This task has lower priority than Task A,
 * so it runs only when Task A blocks or yields.
 */
static void task_b(void *arg)
{
    while (1) {
        ESP_LOGI(TAG_B, "Medium priority task running");

        /*
         * Always delay in normal tasks.
         * This keeps the system responsive.
         */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/*
 * Low priority task.
 *
 * This task runs only when higher priority tasks
 * are blocked, delayed, or waiting.
 */
static void task_c(void *arg)
{
    while (1) {
        ESP_LOGI(TAG_C, "Low priority task running");

        /*
         * Delay allows other tasks to run
         * and prevents unnecessary CPU usage.
         */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI("APP", "Experiment 02: Priority Starvation");

#if USE_FIXED_TASK_A
    ESP_LOGI("APP", "Running FIXED version: Task A uses vTaskDelay()");
#else
    ESP_LOGI("APP", "Running STARVATION version: Task A has no delay");
#endif

    /*
     * Create Task A.
     *
     * Priority = 10
     * This is the highest priority task in this experiment.
     */
#if USE_FIXED_TASK_A
    xTaskCreate(
        task_a_fixed,
        "task_a_fixed",
        2048,
        NULL,
        10,
        NULL
    );
#else
    xTaskCreate(
        task_a_starving,
        "task_a_starving",
        2048,
        NULL,
        10,
        NULL
    );
#endif

    /*
     * Create Task B.
     *
     * Priority = 5
     * Lower than Task A.
     */
    xTaskCreate(
        task_b,
        "task_b",
        2048,
        NULL,
        5,
        NULL
    );

    /*
     * Create Task C.
     *
     * Priority = 1
     * Lowest priority task.
     */
    xTaskCreate(
        task_c,
        "task_c",
        2048,
        NULL,
        1,
        NULL
    );
}