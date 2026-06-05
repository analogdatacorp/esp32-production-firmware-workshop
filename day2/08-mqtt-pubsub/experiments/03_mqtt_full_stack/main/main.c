#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "mqtt_client.h"
#include "nvs_flash.h"

#define WIFI_SSID      "esp-analogdata"
#define WIFI_PASSWORD  "analogdata.io"

#define MQTT_BROKER_URI "mqtt://broker.hivemq.com:1883"
#define MQTT_CLIENT_ID  "analogdata_esp32_led_client"

#define MQTT_TOPIC      "analogdata-esp"

#define LED_GPIO        GPIO_NUM_2

#define WIFI_CONNECTED_BIT BIT0

static const char *TAG_WIFI = "WIFI";
static const char *TAG_MQTT = "MQTT";
static const char *TAG_LED  = "LED";

static EventGroupHandle_t wifi_event_group;
static esp_mqtt_client_handle_t mqtt_client = NULL;

/* -------------------------------------------------------
 * LED Init
 * -----------------------------------------------------*/

static void led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);

    ESP_LOGI(TAG_LED, "LED initialized on GPIO 2");
}

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

        ESP_LOGI(TAG_WIFI, "WiFi started");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG_WIFI, "Connected to Access Point");
        ESP_LOGW(TAG_WIFI, "Waiting for IP address...");
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG_WIFI,
                 "Got IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG_WIFI, "WiFi disconnected");
        ESP_LOGI(TAG_WIFI, "Reconnecting WiFi...");

        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

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
            ESP_LOGI(TAG_MQTT, "Connecting to public Mosquitto broker...");
            break;

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_MQTT, "Connected to broker");

            esp_mqtt_client_subscribe(
                mqtt_client,
                MQTT_TOPIC,
                1
            );

            ESP_LOGI(TAG_MQTT,
                     "Subscribed to topic: %s",
                     MQTT_TOPIC);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG_MQTT, "Disconnected from broker");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_MQTT,
                     "Subscription successful. msg_id=%d",
                     event->msg_id);
            break;

        case MQTT_EVENT_DATA: {
            char topic[128] = {0};
            char data[128] = {0};

            snprintf(topic,
                     sizeof(topic),
                     "%.*s",
                     event->topic_len,
                     event->topic);

            snprintf(data,
                     sizeof(data),
                     "%.*s",
                     event->data_len,
                     event->data);

            ESP_LOGI(TAG_MQTT,
                     "Received topic: %s",
                     topic);

            ESP_LOGI(TAG_MQTT,
                     "Received data: %s",
                     data);

            if (strcmp(topic, MQTT_TOPIC) == 0) {

                if (strcmp(data, "A") == 0) {

                    gpio_set_level(LED_GPIO, 1);
                    ESP_LOGI(TAG_LED, "Command A received -> LED ON");

                } else if (strcmp(data, "B") == 0) {

                    gpio_set_level(LED_GPIO, 0);
                    ESP_LOGI(TAG_LED, "Command B received -> LED OFF");

                } else {

                    ESP_LOGW(TAG_LED,
                             "Unknown command. Send A for ON, B for OFF");
                }
            }

            break;
        }

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG_MQTT, "MQTT error");
            break;

        default:
            break;
    }
}

/* -------------------------------------------------------
 * WiFi Init
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

    strncpy((char *)wifi_config.sta.ssid,
            WIFI_SSID,
            sizeof(wifi_config.sta.ssid));

    strncpy((char *)wifi_config.sta.password,
            WIFI_PASSWORD,
            sizeof(wifi_config.sta.password));

    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

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
 * Wait for WiFi
 * -----------------------------------------------------*/

static void wifi_wait_connected(void)
{
    ESP_LOGI(TAG_WIFI, "Waiting for WiFi...");

    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );

    ESP_LOGI(TAG_WIFI, "WiFi ready");
}

/* -------------------------------------------------------
 * MQTT Init
 * -----------------------------------------------------*/

static void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    esp_mqtt_client_start(mqtt_client);
}

/* -------------------------------------------------------
 * app_main
 * -----------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG_WIFI, "MQTT LED Control Demo");

    led_init();

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    wifi_init_sta();

    wifi_wait_connected();

    mqtt_init();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}