#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "mqtt_client.h"

#include "nvs_flash.h"

#include "lwip/inet.h"

/*
 * Session 08 - Experiment 01
 * First MQTT Connection
 *
 * Concept:
 * WiFi connection is separate from MQTT connection.
 *
 * Connection sequence:
 *
 * ESP32 -> WiFi AP -> IP Address -> MQTT Broker
 */

#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""

#define MQTT_BROKER_URI  "mqtt://broker.hivemq.com:1883"
#define MQTT_CLIENT_ID   "analogdata_esp32_001"

#define WIFI_CONNECTED_BIT BIT0

static const char *TAG_WIFI = "WIFI";
static const char *TAG_MQTT = "MQTT";

static EventGroupHandle_t wifi_event_group;

static bool mqtt_connected = false;

static esp_mqtt_client_handle_t mqtt_client = NULL;

/* -------------------------------------------------------
 * WiFi Event Handler
 * -----------------------------------------------------*/

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG_WIFI,
                 "[1] WiFi driver started");

        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG_WIFI,
                 "[2] Connected to Access Point");

        ESP_LOGW(TAG_WIFI,
                 "Waiting for IP address...");
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG_WIFI,
                 "[3] Got IP Address: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        ESP_LOGI(TAG_WIFI,
                 "Network ready");

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG_WIFI,
                 "WiFi disconnected");

        ESP_LOGI(TAG_WIFI,
                 "Reconnecting WiFi...");

        esp_wifi_connect();

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}

/* -------------------------------------------------------
 * MQTT Event Handler
 * -----------------------------------------------------*/

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {

        /*
         * MQTT client preparing TCP connection.
         */
        case MQTT_EVENT_BEFORE_CONNECT:

            ESP_LOGI(TAG_MQTT,
                     "[1] Connecting to MQTT broker...");

            break;

        /*
         * MQTT connection established.
         */
        case MQTT_EVENT_CONNECTED:

            mqtt_connected = true;

            ESP_LOGI(TAG_MQTT,
                     "[2] Connected to MQTT broker");

            ESP_LOGI(TAG_MQTT,
                     "Client ID: %s",
                     MQTT_CLIENT_ID);

            break;

        /*
         * MQTT disconnected.
         *
         * ESP-IDF MQTT client automatically reconnects.
         */
        case MQTT_EVENT_DISCONNECTED:

            mqtt_connected = false;

            ESP_LOGW(TAG_MQTT,
                     "[3] MQTT disconnected");

            ESP_LOGW(TAG_MQTT,
                     "ESP-IDF MQTT client will auto reconnect");

            break;

        /*
         * MQTT subscribed successfully.
         */
        case MQTT_EVENT_SUBSCRIBED:

            ESP_LOGI(TAG_MQTT,
                     "Subscribed successfully. msg_id=%d",
                     event->msg_id);

            break;

        /*
         * MQTT published successfully.
         */
        case MQTT_EVENT_PUBLISHED:

            ESP_LOGI(TAG_MQTT,
                     "Message published. msg_id=%d",
                     event->msg_id);

            break;

        /*
         * MQTT data received.
         */
        case MQTT_EVENT_DATA:

            ESP_LOGI(TAG_MQTT,
                     "Data received");

            ESP_LOGI(TAG_MQTT,
                     "TOPIC=%.*s",
                     event->topic_len,
                     event->topic);

            ESP_LOGI(TAG_MQTT,
                     "DATA=%.*s",
                     event->data_len,
                     event->data);

            break;

        /*
         * MQTT error.
         */
        case MQTT_EVENT_ERROR:

            ESP_LOGE(TAG_MQTT,
                     "[!] MQTT ERROR");

            if (event->error_handle != NULL) {

                ESP_LOGE(TAG_MQTT,
                         "Error type: %d",
                         event->error_handle->error_type);
            }

            break;

        default:

            ESP_LOGI(TAG_MQTT,
                     "Other MQTT Event ID: %ld",
                     event_id);

            break;
    }
}

/* -------------------------------------------------------
 * Initialize WiFi
 * -----------------------------------------------------*/

static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );

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
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL,
            NULL
        )
    );

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

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );
}

/* -------------------------------------------------------
 * Wait Until WiFi Ready
 * -----------------------------------------------------*/

static void wifi_wait_connected(void)
{
    ESP_LOGI(TAG_WIFI,
             "Waiting for WiFi connection...");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );

    ESP_LOGI(TAG_WIFI,
             "WiFi fully ready");
}

/* -------------------------------------------------------
 * Initialize MQTT
 * -----------------------------------------------------*/

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {

        .broker.address.uri = MQTT_BROKER_URI,

        .credentials.client_id = MQTT_CLIENT_ID,
    };

    mqtt_client =
        esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    esp_mqtt_client_start(mqtt_client);

    ESP_LOGI(TAG_MQTT,
             "MQTT client started");
}

/* -------------------------------------------------------
 * app_main
 * -----------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG_WIFI,
             "Session 08 - Experiment 01");

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
     * Step 1:
     * Connect WiFi.
     */
    wifi_init_sta();

    /*
     * Step 2:
     * Wait until IP acquired.
     */
    wifi_wait_connected();

    /*
     * Step 3:
     * Start MQTT only AFTER WiFi is ready.
     */
    mqtt_init();

    /*
     * Keep application alive.
     */
    while (1) {

        if (mqtt_connected) {

            ESP_LOGI(TAG_MQTT,
                     "MQTT connection healthy");

        } else {

            ESP_LOGW(TAG_MQTT,
                     "MQTT not connected");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}