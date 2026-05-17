#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "wifi_manager.h"

/*
 * Session 07 - Experiment 06
 * Full WiFi Manager Component
 *
 * Application code no longer contains:
 * - WiFi event handlers
 * - NVS logic
 * - reconnect logic
 * - IP event logic
 *
 * All complexity is hidden inside wifi_manager.
 */

#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""

// #define WIFI_SSID      "esp-analogdata"
// #define WIFI_PASSWORD  "analogdata.io"

static const char *TAG = "APP";

/* -------------------------------------------------------
 * Application Task
 * -----------------------------------------------------*/

static void app_task(void *arg)
{
    char ip[16];

    while (1) {

        if (wifi_manager_is_connected()) {

            wifi_manager_get_ip(ip, sizeof(ip));

            ESP_LOGI(
                TAG,
                "App running | IP: %s | RSSI: %d dBm",
                ip,
                wifi_manager_get_rssi()
            );

        } else {

            ESP_LOGW(
                TAG,
                "WiFi not connected. App waiting..."
            );
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

/* -------------------------------------------------------
 * app_main
 * -----------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG,
             "Session 07 - Experiment 06: WiFi Manager Component");

    ESP_ERROR_CHECK(
        wifi_manager_init()
    );

    ESP_ERROR_CHECK(
        wifi_manager_connect(
            WIFI_SSID,
            WIFI_PASSWORD
        )
    );

    ESP_LOGI(TAG,
             "Waiting up to 30 seconds for WiFi");

    esp_err_t ret =
        wifi_manager_wait_connected(30000);

    if (ret != ESP_OK) {

        ESP_LOGE(TAG,
                 "WiFi failed to connect within timeout");

        return;
    }

    char ip[16];

    wifi_manager_get_ip(ip, sizeof(ip));

    ESP_LOGI(TAG,
             "WiFi connected successfully");

    ESP_LOGI(TAG,
             "IP Address: %s",
             ip);

    ESP_LOGI(TAG,
             "RSSI: %d dBm",
             wifi_manager_get_rssi());

    xTaskCreate(
        app_task,
        "app_task",
        4096,
        NULL,
        5,
        NULL
    );
}