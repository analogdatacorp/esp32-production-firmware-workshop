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
 * Session 08 - Experiment 02
 * Publish Sensor Data
 *
 * Concept:
 * ESP32 publishes MQTT messages every 2 seconds.
 *
 * MQTT Explorer on PC receives messages live.
 */

#define WIFI_SSID      "Wokwi-GUEST"
#define WIFI_PASSWORD  ""

#define MQTT_BROKER_URI  "mqtt://test.mosquitto.org:1883"
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
                 "WiFi started");

        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG_WIFI,
                 "Connected to Access Point");

        ESP_LOGW(TAG_WIFI,
                 "Waiting for IP...");
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG_WIFI,
                 "Got IP: " IPSTR,
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

        mqtt_connected = false;

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        ESP_LOGI(TAG_WIFI,
                 "Reconnecting WiFi...");

        esp_wifi_connect();
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

        case MQTT_EVENT_BEFORE_CONNECT:

            ESP_LOGI(TAG_MQTT,
                     "Connecting to MQTT broker...");

            break;

        case MQTT_EVENT_CONNECTED:

            mqtt_connected = true;

            ESP_LOGI(TAG_MQTT,
                     "Connected to MQTT broker");

            break;

        case MQTT_EVENT_DISCONNECTED:

            mqtt_connected = false;

            ESP_LOGW(TAG_MQTT,
                     "MQTT disconnected");

            break;

        case MQTT_EVENT_PUBLISHED:

            ESP_LOGI(TAG_MQTT,
                     "Message published successfully. msg_id=%d",
                     event->msg_id);

            break;

        case MQTT_EVENT_ERROR:

            ESP_LOGE(TAG_MQTT,
                     "MQTT ERROR");

            break;

        default:
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
 * Wait Until WiFi Connected
 * -----------------------------------------------------*/

static void wifi_wait_connected(void)
{
    ESP_LOGI(TAG_WIFI,
             "Waiting for WiFi...");

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

        .broker.address.uri =
            MQTT_BROKER_URI,

        .credentials.client_id =
            MQTT_CLIENT_ID,
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
 * Publish Task
 * -----------------------------------------------------*/

static void publish_task(void *arg)
{
    int counter = 0;

    char payload[32];
    char topic[128];

    /*
     * Unique topic for this ESP32.
     */
    snprintf(
        topic,
        sizeof(topic),
        "workshop/esp32/%s/counter",
        MQTT_CLIENT_ID
    );

    ESP_LOGI(TAG_MQTT,
             "Publish topic: %s",
             topic);

    while (1) {

        if (mqtt_connected) {

            /*
             * Build payload.
             */
            snprintf(
                payload,
                sizeof(payload),
                "%d",
                counter++
            );

            /*
             * Publish message.
             */
            int msg_id =
                esp_mqtt_client_publish(
                    mqtt_client,
                    topic,
                    payload,
                    0,
                    1,
                    0
                );

            /*
             * Check publish result.
             */
            if (msg_id == -1) {

                ESP_LOGE(TAG_MQTT,
                         "Publish FAILED");

            } else {

                ESP_LOGI(TAG_MQTT,
                         "Published: %s -> %s (msg_id=%d)",
                         topic,
                         payload,
                         msg_id);
            }

        } else {

            ESP_LOGW(TAG_MQTT,
                     "MQTT not connected. Skipping publish");
        }

        /*
         * Publish every 2 seconds.
         */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* -------------------------------------------------------
 * app_main
 * -----------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG_WIFI,
             "Session 08 - Experiment 02");

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
     * Connect WiFi.
     */
    wifi_init_sta();

    /*
     * Wait until IP ready.
     */
    wifi_wait_connected();

    /*
     * Start MQTT.
     */
    mqtt_init();

    /*
     * Start publishing task.
     */
    xTaskCreate(
        publish_task,
        "publish_task",
        4096,
        NULL,
        5,
        NULL
    );
}