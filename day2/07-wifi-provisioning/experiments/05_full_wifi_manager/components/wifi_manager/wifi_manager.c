#include "wifi_manager.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/inet.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define NVS_NAMESPACE "wifi_manager"
#define KEY_SSID      "ssid"
#define KEY_PASS      "pass"

#define MAX_RETRY_DELAY_MS 60000

static const char *TAG = "WIFI_MANAGER";

static EventGroupHandle_t s_wifi_event_group = NULL;

static bool s_initialized = false;
static bool s_connected = false;

static int s_retry_num = 0;
static int s_retry_delay_ms = 1000;

static char s_current_ip[16] = "0.0.0.0";

static esp_netif_t *s_sta_netif = NULL;

/* -------------------------------------------------------
 * Save Credentials to NVS
 * -----------------------------------------------------*/

static esp_err_t save_credentials(const char *ssid,
                                  const char *password)
{
    nvs_handle_t handle;

    esp_err_t ret = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &handle
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS");
        return ret;
    }

    ESP_ERROR_CHECK(nvs_set_str(handle, KEY_SSID, ssid));
    ESP_ERROR_CHECK(nvs_set_str(handle, KEY_PASS, password));
    ESP_ERROR_CHECK(nvs_commit(handle));

    nvs_close(handle);

    ESP_LOGI(TAG, "Credentials saved to NVS");

    return ESP_OK;
}

/* -------------------------------------------------------
 * Load Credentials from NVS
 * -----------------------------------------------------*/

static bool load_credentials(char *ssid,
                             size_t ssid_len,
                             char *password,
                             size_t password_len)
{
    nvs_handle_t handle;

    esp_err_t ret = nvs_open(
        NVS_NAMESPACE,
        NVS_READONLY,
        &handle
    );

    if (ret != ESP_OK) {
        return false;
    }

    ret = nvs_get_str(handle, KEY_SSID, ssid, &ssid_len);

    if (ret != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    ret = nvs_get_str(handle, KEY_PASS, password, &password_len);

    if (ret != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);

    return true;
}

/* -------------------------------------------------------
 * Retry Helper
 * -----------------------------------------------------*/

static void reconnect_with_backoff(void)
{
    s_retry_num++;

    ESP_LOGW(TAG,
             "Reconnect attempt #%d in %d ms",
             s_retry_num,
             s_retry_delay_ms);

    vTaskDelay(pdMS_TO_TICKS(s_retry_delay_ms));

    s_retry_delay_ms *= 2;

    if (s_retry_delay_ms > MAX_RETRY_DELAY_MS) {
        s_retry_delay_ms = MAX_RETRY_DELAY_MS;
    }

    esp_wifi_connect();
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

        ESP_LOGI(TAG, "WiFi started");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG, "Associated with Access Point");
        ESP_LOGW(TAG, "Waiting for IP address...");
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        s_connected = true;
        s_retry_num = 0;
        s_retry_delay_ms = 1000;

        snprintf(
            s_current_ip,
            sizeof(s_current_ip),
            IPSTR,
            IP2STR(&event->ip_info.ip)
        );

        ESP_LOGI(TAG, "Got IP: %s", s_current_ip);
        ESP_LOGI(TAG, "WiFi ready");

        xEventGroupSetBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;

        s_connected = false;

        strncpy(
            s_current_ip,
            "0.0.0.0",
            sizeof(s_current_ip)
        );

        ESP_LOGW(TAG,
                 "Disconnected. Reason code: %d",
                 event->reason);

        xEventGroupClearBits(
            s_wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        reconnect_with_backoff();
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_LOST_IP) {

        s_connected = false;

        strncpy(
            s_current_ip,
            "0.0.0.0",
            sizeof(s_current_ip)
        );

        ESP_LOGW(TAG, "IP address lost");
    }
}

/* -------------------------------------------------------
 * Public API: Init
 * -----------------------------------------------------*/

esp_err_t wifi_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WiFi Manager");

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    s_wifi_event_group = xEventGroupCreate();

    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );

    s_sta_netif = esp_netif_create_default_wifi_sta();

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

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );

    s_initialized = true;

    ESP_LOGI(TAG, "WiFi Manager initialized");

    return ESP_OK;
}

/* -------------------------------------------------------
 * Public API: Connect
 * -----------------------------------------------------*/

esp_err_t wifi_manager_connect(const char *ssid,
                               const char *password)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    save_credentials(ssid, password);

    wifi_config_t wifi_config = {0};

    strncpy(
        (char *)wifi_config.sta.ssid,
        ssid,
        sizeof(wifi_config.sta.ssid)
    );

    strncpy(
        (char *)wifi_config.sta.password,
        password,
        sizeof(wifi_config.sta.password)
    );

    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );

    ESP_ERROR_CHECK(
        esp_wifi_start()
    );

    return ESP_OK;
}

/* -------------------------------------------------------
 * Public API: Is Connected
 * -----------------------------------------------------*/

bool wifi_manager_is_connected(void)
{
    return s_connected;
}

/* -------------------------------------------------------
 * Public API: Wait Connected
 * -----------------------------------------------------*/

esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms)
{
    if (s_wifi_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t ticks;

    if (timeout_ms == 0) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout_ms);
    }

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        ticks
    );

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

/* -------------------------------------------------------
 * Public API: Get IP
 * -----------------------------------------------------*/

esp_err_t wifi_manager_get_ip(char *ip_str,
                              size_t len)
{
    if (ip_str == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    strncpy(ip_str, s_current_ip, len);

    ip_str[len - 1] = '\0';

    return ESP_OK;
}

/* -------------------------------------------------------
 * Public API: Get RSSI
 * -----------------------------------------------------*/

int wifi_manager_get_rssi(void)
{
    wifi_ap_record_t ap_info;

    if (!s_connected) {
        return 0;
    }

    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);

    if (ret != ESP_OK) {
        return 0;
    }

    return ap_info.rssi;
}

/* -------------------------------------------------------
 * Public API: Disconnect
 * -----------------------------------------------------*/

void wifi_manager_disconnect(void)
{
    ESP_LOGW(TAG, "Manual WiFi disconnect requested");

    s_connected = false;

    esp_wifi_disconnect();
}