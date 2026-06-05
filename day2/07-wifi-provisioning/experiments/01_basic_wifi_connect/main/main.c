#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "lwip/inet.h"

/*
 * Session 07 - Experiment 01
 * Basic WiFi Connect
 *
 * Concept:
 * ESP32 connects to WiFi and waits until IP address is acquired.
 *
 * Important point:
 *
 * WIFI_EVENT_STA_CONNECTED does NOT mean internet/network is ready.
 * It only means ESP32 is associated with the Access Point.
 *
 * IP_EVENT_STA_GOT_IP means ESP32 received IP address from DHCP.
 * Only after GOT_IP should we start MQTT, HTTP, OTA, cloud, etc.
 *
 * Wokwi WiFi:
 * SSID     = "Wokwi-GUEST"
 * Password = ""
 */

#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""

// #define WIFI_SSID      "esp-analogdata"
// #define WIFI_PASSWORD  "analogdata.io"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "WIFI_EXP_01";

static EventGroupHandle_t wifi_event_group;

/*
 * WiFi event handler.
 *
 * This function receives both:
 * - WIFI_EVENT events
 * - IP_EVENT events
 *
 * We use this to clearly show the connection lifecycle.
 */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        /*
         * WiFi driver started.
         * Now we can ask ESP32 to connect to AP.
         */
        ESP_LOGI(TAG, "Step 1: WiFi driver started");
        ESP_LOGI(TAG, "Calling esp_wifi_connect()");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        /*
         * ESP32 is associated with the WiFi router/AP.
         *
         * Very important:
         * At this point IP address may not be available yet.
         */
        ESP_LOGI(TAG, "Step 2: Associated with AP");
        ESP_LOGW(TAG, "But IP address is NOT ready yet");
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        /*
         * ESP32 received IP address from DHCP.
         *
         * Network is now usable.
         */
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG,
                 "Step 3: Got IP Address: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        ESP_LOGI(TAG, "Network is now READY");

        xEventGroupSetBits(wifi_event_group,
                           WIFI_CONNECTED_BIT);
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        /*
         * ESP32 disconnected from AP.
         *
         * In this first experiment, we only log it.
         * Auto reconnect will be handled in later experiments.
         */
        ESP_LOGW(TAG, "Disconnected from WiFi AP");

        xEventGroupSetBits(wifi_event_group,
                           WIFI_FAIL_BIT);
    }
}

/*
 * Initialize WiFi in Station mode.
 */
static void wifi_init_sta(void)
{
    /*
     * Event group is used to wait until connection succeeds or fails.
     */
    wifi_event_group = xEventGroupCreate();

    /*
     * Initialize TCP/IP network stack.
     */
    ESP_ERROR_CHECK(esp_netif_init());

    /*
     * Create default event loop.
     * Required before registering WiFi/IP event handlers.
     */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /*
     * Create default WiFi station interface.
     */
    esp_netif_create_default_wifi_sta();

    /*
     * Initialize WiFi driver with default config.
     */
    wifi_init_config_t wifi_init_config =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&wifi_init_config)
    );

    /*
     * Register event handlers.
     *
     * We register the same function for:
     * - All WiFi events
     * - GOT_IP event
     */
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL
        )
    );

    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL,
            NULL
        )
    );

    /*
     * Configure WiFi credentials.
     */
    wifi_config_t wifi_config = {0};

    strncpy((char *)wifi_config.sta.ssid,
            WIFI_SSID,
            sizeof(wifi_config.sta.ssid));

    strncpy((char *)wifi_config.sta.password,
            WIFI_PASSWORD,
            sizeof(wifi_config.sta.password));

    /*
     * WPA/WPA2 is enough for most routers.
     * Wokwi-GUEST is open network, so empty password is okay.
     */
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    /*
     * Set WiFi mode as Station.
     */
    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    /*
     * Apply station configuration.
     */
    ESP_ERROR_CHECK(
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config)
    );

    /*
     * Start WiFi.
     *
     * This triggers WIFI_EVENT_STA_START.
     */
    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(TAG, "WiFi initialization complete");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Session 07 - Experiment 01: Basic WiFi Connect");

    /*
     * NVS must be initialized before WiFi.
     *
     * WiFi driver stores calibration and config data in NVS.
     */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /*
     * Initialize WiFi and start connection.
     */
    wifi_init_sta();

    /*
     * Wait until connected or disconnected.
     */
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {

        ESP_LOGI(TAG,
                 "Application can safely start network operations now");

    } else if (bits & WIFI_FAIL_BIT) {

        ESP_LOGE(TAG,
                 "WiFi connection failed");

    } else {

        ESP_LOGE(TAG,
                 "Unexpected WiFi event");
    }
}