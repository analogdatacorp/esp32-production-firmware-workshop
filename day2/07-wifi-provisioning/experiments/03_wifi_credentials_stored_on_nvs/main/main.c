#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/inet.h"

/*
 * Session 07 - Experiment 03
 * WiFi Credentials in NVS
 *
 * Concept:
 * Save credentials once.
 * Load credentials after reboot.
 * Connect automatically.
 *
 * This is how production IoT devices work.
 */

#define DEFAULT_WIFI_SSID      "Wokwi-GUEST"
#define DEFAULT_WIFI_PASSWORD  ""

// #define DEFAULT_WIFI_SSID      "esp-analogdata"
// #define DEFAULT_WIFI_PASSWORD  "analogdata.io"

#define NVS_NAMESPACE          "wifi"
#define KEY_SSID               "ssid"
#define KEY_PASS               "pass"

static const char *TAG = "WIFI_NVS";

/* -------------------------------------------------------
 * Save WiFi Credentials to NVS
 * -----------------------------------------------------*/

static esp_err_t save_credentials(const char *ssid,
                                  const char *password)
{
    nvs_handle_t nvs_handle;

    /*
     * Open NVS namespace in read/write mode.
     */
    ESP_ERROR_CHECK(
        nvs_open(
            NVS_NAMESPACE,
            NVS_READWRITE,
            &nvs_handle
        )
    );

    /*
     * Save SSID string.
     */
    ESP_ERROR_CHECK(
        nvs_set_str(
            nvs_handle,
            KEY_SSID,
            ssid
        )
    );

    /*
     * Save password string.
     */
    ESP_ERROR_CHECK(
        nvs_set_str(
            nvs_handle,
            KEY_PASS,
            password
        )
    );

    /*
     * Commit changes to flash.
     */
    ESP_ERROR_CHECK(
        nvs_commit(nvs_handle)
    );

    /*
     * Always close handle.
     */
    nvs_close(nvs_handle);

    ESP_LOGI(TAG,
             "WiFi credentials saved to NVS");

    return ESP_OK;
}

/* -------------------------------------------------------
 * Load WiFi Credentials from NVS
 * -----------------------------------------------------*/

static bool load_credentials(char *ssid,
                             size_t ssid_len,
                             char *password,
                             size_t password_len)
{
    nvs_handle_t nvs_handle;

    /*
     * Open namespace in read-only mode.
     */
    esp_err_t ret = nvs_open(
        NVS_NAMESPACE,
        NVS_READONLY,
        &nvs_handle
    );

    if (ret != ESP_OK) {

        ESP_LOGW(TAG,
                 "NVS namespace not found");

        return false;
    }

    /*
     * Read SSID from NVS.
     */
    ret = nvs_get_str(
        nvs_handle,
        KEY_SSID,
        ssid,
        &ssid_len
    );

    if (ret != ESP_OK) {

        ESP_LOGW(TAG,
                 "SSID not found in NVS");

        nvs_close(nvs_handle);

        return false;
    }

    /*
     * Read password from NVS.
     */
    ret = nvs_get_str(
        nvs_handle,
        KEY_PASS,
        password,
        &password_len
    );

    if (ret != ESP_OK) {

        ESP_LOGW(TAG,
                 "Password not found in NVS");

        nvs_close(nvs_handle);

        return false;
    }

    nvs_close(nvs_handle);

    ESP_LOGI(TAG,
             "Credentials loaded from NVS");

    return true;
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

        ESP_LOGI(TAG,
                 "WiFi started");

        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_CONNECTED) {

        ESP_LOGI(TAG,
                 "Connected to Access Point");

        ESP_LOGW(TAG,
                 "Waiting for DHCP IP...");
    }

    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP) {

        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG,
                 "Got IP Address: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        ESP_LOGI(TAG,
                 "Network ready");
    }

    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {

        ESP_LOGW(TAG,
                 "WiFi disconnected");

        ESP_LOGI(TAG,
                 "Retrying connection...");

        esp_wifi_connect();
    }
}

/* -------------------------------------------------------
 * Initialize WiFi
 * -----------------------------------------------------*/

static void wifi_init_sta(const char *ssid,
                          const char *password)
{
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

    /*
     * Register WiFi events.
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
     * Register IP events.
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
        ssid,
        sizeof(wifi_config.sta.ssid)
    );

    strncpy(
        (char *)wifi_config.sta.password,
        password,
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

    ESP_LOGI(TAG,
             "WiFi initialization complete");
}

/* -------------------------------------------------------
 * app_main
 * -----------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG,
             "Session 07 - Experiment 03");

    /*
     * Initialize NVS flash.
     */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /*
     * Buffers for loaded credentials.
     */
    char ssid[32] = {0};
    char password[64] = {0};

    /*
     * Attempt loading credentials.
     */
    bool credentials_found =
        load_credentials(
            ssid,
            sizeof(ssid),
            password,
            sizeof(password)
        );

    /*
     * First boot:
     * Save default credentials.
     */
    if (!credentials_found) {

        ESP_LOGW(TAG,
                 "No WiFi credentials found");

        ESP_LOGI(TAG,
                 "Saving default credentials");

        ESP_ERROR_CHECK(
            save_credentials(
                DEFAULT_WIFI_SSID,
                DEFAULT_WIFI_PASSWORD
            )
        );

        /*
         * Use default values immediately.
         */
        strncpy(ssid,
                DEFAULT_WIFI_SSID,
                sizeof(ssid));

        strncpy(password,
                DEFAULT_WIFI_PASSWORD,
                sizeof(password));
    }

    ESP_LOGI(TAG,
             "Connecting using SSID: %s",
             ssid);

    /*
     * Start WiFi.
     */
    wifi_init_sta(ssid, password);

    /*
     * Keep app alive.
     */
    while (1) {

        ESP_LOGI(TAG,
                 "Application running");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}