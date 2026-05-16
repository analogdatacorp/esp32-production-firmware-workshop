#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

/*
 * Experiment 04:
 * Mutex - Protecting a Shared UART / Serial Output
 *
 * Concept:
 * When two tasks use the same shared resource at the same time,
 * the output can become unsafe, mixed, or unpredictable.
 *
 * In this experiment:
 * - Task A prints a long message
 * - Task B prints a long message
 * - Both tasks are using the same UART/serial output
 *
 * Without mutex:
 * - Both tasks can try to print at the same time
 *
 * With mutex:
 * - Only one task prints at a time
 *
 * Change this value:
 * 0 = Without mutex
 * 1 = With mutex
 */
#define USE_MUTEX 1

static const char *TAG_A = "TASK_A";
static const char *TAG_B = "TASK_B";

static SemaphoreHandle_t uart_mutex = NULL;

/*
 * This helper function prints a long message slowly.
 *
 * We use printf() character-by-character instead of only ESP_LOGI()
 * because ESP_LOGI() may internally reduce visible garbling.
 *
 * Character-by-character printing makes the shared-resource problem
 * easier to demonstrate in a workshop.
 */
static void print_slowly(const char *task_name, const char *message)
{
    printf("%s: ", task_name);

    for (int i = 0; message[i] != '\0'; i++) {
        putchar(message[i]);

        /*
         * Small delay between characters.
         * This increases the chance that another task runs in between
         * and corrupts/interleaves the output when mutex is disabled.
         */
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    printf("\n");
    fflush(stdout);
}

static void task_a(void *arg)
{
    const char *message =
        "Sending long sensor data string: temperature=28.5C humidity=61.2 percent";

    while (1) {
#if USE_MUTEX
        /*
         * Take mutex before accessing UART.
         * If Task B already has the mutex, Task A waits here.
         */
        xSemaphoreTake(uart_mutex, portMAX_DELAY);
#endif

        print_slowly("TASK_A", message);

#if USE_MUTEX
        /*
         * Release mutex after UART access is complete.
         * Now another task can safely print.
         */
        xSemaphoreGive(uart_mutex);
#endif

        vTaskDelay(pdMS_TO_TICKS(700));
    }
}

static void task_b(void *arg)
{
    const char *message =
        "Publishing MQTT payload: device_id=esp32_001 status=online battery=92 percent";

    while (1) {
#if USE_MUTEX
        /*
         * Take mutex before accessing UART.
         * This protects shared serial output.
         */
        xSemaphoreTake(uart_mutex, portMAX_DELAY);
#endif

        print_slowly("TASK_B", message);

#if USE_MUTEX
        /*
         * Give mutex back after complete message is printed.
         */
        xSemaphoreGive(uart_mutex);
#endif

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI("APP", "Experiment 04: Mutex Shared UART");

#if USE_MUTEX
    ESP_LOGI("APP", "Mode: WITH mutex - clean output expected");

    uart_mutex = xSemaphoreCreateMutex();

    if (uart_mutex == NULL) {
        ESP_LOGE("APP", "Failed to create UART mutex");
        return;
    }
#else
    ESP_LOGW("APP", "Mode: WITHOUT mutex - mixed output may be visible");
#endif

    xTaskCreate(
        task_a,
        "task_a",
        4096,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        task_b,
        "task_b",
        4096,
        NULL,
        5,
        NULL
    );
}