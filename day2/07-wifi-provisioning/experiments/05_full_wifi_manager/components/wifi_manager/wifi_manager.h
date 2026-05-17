#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

esp_err_t wifi_manager_init(void);

esp_err_t wifi_manager_connect(const char *ssid,
                               const char *password);

bool wifi_manager_is_connected(void);

esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms);

esp_err_t wifi_manager_get_ip(char *ip_str,
                              size_t len);

int wifi_manager_get_rssi(void);

void wifi_manager_disconnect(void);