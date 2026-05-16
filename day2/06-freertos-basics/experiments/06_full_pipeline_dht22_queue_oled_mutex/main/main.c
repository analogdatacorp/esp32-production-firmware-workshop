#include <stdio.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "ssd1306.h"

/*
 * Experiment 06:
 * Full FreeRTOS Pipeline
 *
 * Architecture:
 *
 * sensor_task  ---> Queue ---> display_task ---> OLED
 *                       \
 *                        ---> log_task ---> Serial Monitor
 *
 * Concepts:
 * - Tasks
 * - Queue
 * - Mutex
 * - OLED display update
 * - Production-style task separation
 *
 * NOTE:
 * This version uses simulated sensor data.
 * No DHT22 library is required.
 */

#define I2C_SDA_GPIO GPIO_NUM_21
#define I2C_SCL_GPIO GPIO_NUM_22

#define SENSOR_QUEUE_LENGTH 10

static const char *TAG = "PIPELINE";

/* Sensor data structure */
typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

/* Global RTOS handles */
static QueueHandle_t display_queue = NULL;
static QueueHandle_t log_queue = NULL;
static SemaphoreHandle_t i2c_mutex = NULL;

/* SSD1306 device handle */
static SSD1306_t dev;

/* -------------------------------------------------------
 * OLED Initialization
 * -----------------------------------------------------*/

static void oled_init_display(void)
{
    /*
     * Initialize OLED in I2C mode.
     *
     * Parameters:
     * &dev         -> OLED device structure
     * SDA GPIO     -> GPIO 21
     * SCL GPIO     -> GPIO 22
     * RESET GPIO   -> -1 because reset pin is not used
     */
    i2c_master_init(&dev, I2C_SDA_GPIO, I2C_SCL_GPIO, -1);

    /*
     * Initialize 128x64 SSD1306 OLED.
     */
    ssd1306_init(&dev, 128, 64);

    /*
     * Clear display.
     */
    ssd1306_clear_screen(&dev, false);

    /*
     * Set maximum contrast.
     */
    ssd1306_contrast(&dev, 0xFF);
}

/* -------------------------------------------------------
 * OLED Display Update
 * -----------------------------------------------------*/

static void oled_show_sensor_data(float temperature, float humidity)
{
    char line1[32];
    char line2[32];

    snprintf(line1, sizeof(line1), "Temp: %.1f C", temperature);
    snprintf(line2, sizeof(line2), "Humi: %.1f %%", humidity);

    /*
     * Clear old content before writing new values.
     */
    ssd1306_clear_screen(&dev, false);

    /*
     * OLED has 8 text rows: 0 to 7.
     * Each row is 8 pixels high.
     */
    ssd1306_display_text(
        &dev,
        0,
        "FreeRTOS Pipeline",
        strlen("FreeRTOS Pipeline"),
        false
    );

    ssd1306_display_text(
        &dev,
        2,
        "Queue + Mutex Demo",
        strlen("Queue + Mutex Demo"),
        false
    );

    ssd1306_display_text(
        &dev,
        4,
        line1,
        strlen(line1),
        false
    );

    ssd1306_display_text(
        &dev,
        5,
        line2,
        strlen(line2),
        false
    );
}

/* -------------------------------------------------------
 * Sensor Task
 * -----------------------------------------------------*/

static void sensor_task(void *arg)
{
    sensor_data_t data;
    float angle = 0.0f;

    while (1) {
        /*
         * Simulated DHT22-like sensor values.
         *
         * Temperature changes between roughly 24C and 30C.
         * Humidity changes between roughly 50% and 70%.
         */
        data.temperature = 27.0f + (sinf(angle) * 3.0f);
        data.humidity = 60.0f + (cosf(angle) * 10.0f);

        angle += 0.4f;

        /*
         * Send complete structure to queue.
         *
         * Queue copies the full struct safely.
         */
        BaseType_t display_sent = xQueueSend(display_queue, &data, 0);
        BaseType_t log_sent = xQueueSend(log_queue, &data, 0);

        if (display_sent == pdTRUE && log_sent == pdTRUE) {
            ESP_LOGI(
                TAG,
                "Sensor sent -> Temp: %.1f C | Humi: %.1f %%",
                data.temperature,
                data.humidity
            );
        } else {
            if (display_sent != pdTRUE) {
                ESP_LOGW(TAG, "Display queue full - sensor data dropped");
            }

            if (log_sent != pdTRUE) {
                ESP_LOGW(TAG, "Log queue full - sensor data dropped");
            }
        }

        /*
         * Sensor reads every 2 seconds.
         */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* -------------------------------------------------------
 * Display Task
 * -----------------------------------------------------*/

static void display_task(void *arg)
{
    sensor_data_t data;

    while (1) {
        /*
         * Wait until sensor data is available.
         * This task sleeps efficiently while waiting.
         */
        if (xQueueReceive(display_queue, &data, portMAX_DELAY) == pdTRUE) {

            /*
             * OLED/I2C is a shared hardware resource.
             * Protect it with mutex.
             */
            xSemaphoreTake(i2c_mutex, portMAX_DELAY);

            oled_show_sensor_data(data.temperature, data.humidity);

            xSemaphoreGive(i2c_mutex);

            ESP_LOGI(TAG, "Display updated");
        }
    }
}

/* -------------------------------------------------------
 * Log Task
 * -----------------------------------------------------*/

static void log_task(void *arg)
{
    sensor_data_t data;

    while (1) {
        /*
         * IMPORTANT:
         * This receives from its own queue, so logging does not steal
         * samples from the display task.
         */
        if (xQueueReceive(log_queue, &data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(
                "LOGGER",
                "Logger received -> Temp: %.1f C | Humi: %.1f %%",
                data.temperature,
                data.humidity
            );
        }
    }
}

/* -------------------------------------------------------
 * app_main
 * -----------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG, "Experiment 06: Full Pipeline Started");

    /*
     * Create queue.
     */
    display_queue = xQueueCreate(
        SENSOR_QUEUE_LENGTH,
        sizeof(sensor_data_t)
    );

    log_queue = xQueueCreate(
        SENSOR_QUEUE_LENGTH,
        sizeof(sensor_data_t)
    );

    if (display_queue == NULL || log_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor queues");
        return;
    }

    /*
     * Create mutex for OLED/I2C protection.
     */
    i2c_mutex = xSemaphoreCreateMutex();

    if (i2c_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create I2C mutex");
        return;
    }

    /*
     * Initialize OLED safely.
     */
    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    oled_init_display();
    oled_show_sensor_data(0.0f, 0.0f);

    xSemaphoreGive(i2c_mutex);

    /*
     * Create tasks.
     */
    xTaskCreate(
        sensor_task,
        "sensor_task",
        4096,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        display_task,
        "display_task",
        4096,
        NULL,
        5,
        NULL
    );

    xTaskCreate(
        log_task,
        "log_task",
        4096,
        NULL,
        3,
        NULL
    );
}
