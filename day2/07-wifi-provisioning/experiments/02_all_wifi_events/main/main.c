#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "lwip/inet.h"

/*
 * Session 07 - Experiment 02
 * All WiFi Events Logged
 *
 * Concept:
 * Log every important WiFi and IP event.
 *
 * This helps attendees understand:
 * - WiFi is event driven
 * - Connection has multiple states
 * - Disconnect reason codes matter
 * - Production firmware must handle all events
 */

// #define WIFI_SSID      "Wokwi-GUEST"
// #define WIFI_PASSWORD  ""

#define WIFI_SSID      "esp-analogdata"
#define WIFI_PASSWORD  "analogdata.io"

static const char *TAG = "WIFI_EVENTS";

/*
 * Convert disconnect reason into readable text.
 *
 * Helps during workshop demonstrations.
 */
static const char *disconnect_reason_to_string(uint8_t reason)
{
    switch (reason) {

        case WIFI_REASON_AUTH_EXPIRE:
            return "AUTH_EXPIRE";

        case WIFI_REASON_AUTH_FAIL:
            return "AUTH_FAIL / WRONG_PASSWORD";

        case WIFI_REASON_NO_AP_FOUND:
            return "NO_AP_FOUND";

        case WIFI_REASON_ASSOC_LEAVE:
            return "ASSOC_LEAVE";

        case WIFI_REASON_BEACON_TIMEOUT:
            return "BEACON_TIMEOUT";

        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            return "HANDSHAKE_TIMEOUT";

        default:
            return "UNKNOWN_REASON";
    }
}

/*
 * WiFi + IP event handler.
 */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    /* ---------------------------------------------------
     * WIFI EVENTS
     * -------------------------------------------------*/

    if (event_base == WIFI_EVENT) {

        switch (event_id) {

            /*
             * WiFi driver started.
             */
            case WIFI_EVENT_STA_START:

                ESP_LOGI(TAG,
                         "[EVENT] WIFI_EVENT_STA_START");

                ESP_LOGI(TAG,
                         "WiFi driver ready");

                ESP_LOGI(TAG,
                         "Calling esp_wifi_connect()");

                esp_wifi_connect();

                break;

            /*
             * Connected to AP.
             * Still waiting for DHCP IP.
             */
            case WIFI_EVENT_STA_CONNECTED: {

                wifi_event_sta_connected_t *event =
                    (wifi_event_sta_connected_t *)event_data;

                ESP_LOGI(TAG,
                         "[EVENT] WIFI_EVENT_STA_CONNECTED");

                ESP_LOGI(TAG,
                         "Associated with SSID: %s",
                         event->ssid);

                ESP_LOGW(TAG,
                         "No IP address yet");

                break;
            }

            /*
             * Lost WiFi connection.
             */
            case WIFI_EVENT_STA_DISCONNECTED: {

                wifi_event_sta_disconnected_t *event =
                    (wifi_event_sta_disconnected_t *)event_data;

                ESP_LOGW(TAG,
                         "[EVENT] WIFI_EVENT_STA_DISCONNECTED");

                ESP_LOGW(TAG,
                         "Disconnect reason code: %d",
                         event->reason);

                ESP_LOGW(TAG,
                         "Meaning: %s",
                         disconnect_reason_to_string(event->reason));

                ESP_LOGI(TAG,
                         "Attempting reconnect...");

                /*
                 * Reconnect automatically.
                 */
                esp_wifi_connect();

                break;
            }

            default:

                ESP_LOGI(TAG,
                         "[EVENT] Other WiFi Event ID: %" PRId32,
                         event_id);

                break;
        }
    }

    /* ---------------------------------------------------
     * IP EVENTS
     * -------------------------------------------------*/

    else if (event_base == IP_EVENT) {

        switch (event_id) {

            /*
             * DHCP assigned IP address.
             */
            case IP_EVENT_STA_GOT_IP: {

                ip_event_got_ip_t *event =
                    (ip_event_got_ip_t *)event_data;

                ESP_LOGI(TAG,
                         "[EVENT] IP_EVENT_STA_GOT_IP");

                ESP_LOGI(TAG,
                         "IP Address: " IPSTR,
                         IP2STR(&event->ip_info.ip));

                ESP_LOGI(TAG,
                         "Gateway   : " IPSTR,
                         IP2STR(&event->ip_info.gw));

                ESP_LOGI(TAG,
                         "Netmask   : " IPSTR,
                         IP2STR(&event->ip_info.netmask));

                ESP_LOGI(TAG,
                         "Network is READY");

                break;
            }

            /*
             * IP lost event.
             */
            case IP_EVENT_STA_LOST_IP:

                ESP_LOGW(TAG,
                         "[EVENT] IP_EVENT_STA_LOST_IP");

                ESP_LOGW(TAG,
                         "DHCP lease expired or network lost");

                break;

            default:

                ESP_LOGI(TAG,
                         "[EVENT] Other IP Event ID: %" PRId32,
                         event_id);

                break;
        }
    }
}

/*
 * Initialize WiFi in Station mode.
 */
static void wifi_init_sta(void)
{
    /*
     * Initialize TCP/IP stack.
     */
    ESP_ERROR_CHECK(esp_netif_init());

    /*
     * Create event loop.
     */
    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    /*
     * Create WiFi station interface.
     */
    esp_netif_create_default_wifi_sta();

    /*
     * Initialize WiFi driver.
     */
    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

    /*
     * Register ALL WiFi events.
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

    /*
     * Register ALL IP events.
     */
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL
        )
    );

    /*
     * Configure WiFi credentials.
     */
    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        WIFI_SSID,
        sizeof(wifi_config.sta.ssid)
    );

    strncpy(
        (char *)wifi_config.sta.password,
        WIFI_PASSWORD,
        sizeof(wifi_config.sta.password)
    );

    wifi_config.sta.threshold.authmode =
        WIFI_AUTH_OPEN;

    /*
     * Station mode.
     */
    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    /*
     * Apply configuration.
     */
    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    /*
     * Start WiFi driver.
     */
    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    ESP_LOGI(TAG,
             "WiFi initialization complete");
}

void app_main(void)
{
    ESP_LOGI(TAG,
             "Session 07 - Experiment 02");

    /*
     * Initialize NVS.
     */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /*
     * Start WiFi.
     */
    wifi_init_sta();

    /*
     * Main task just stays alive.
     *
     * All WiFi handling is event-driven.
     */
    while (1) {

        ESP_LOGI(TAG,
                 "Main application running...");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}