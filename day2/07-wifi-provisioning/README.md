# 07 — WiFi Provisioning + Robust Connection Handling

## Session Overview

Every IoT product needs WiFi. But connecting to WiFi in a
real product is very different from connecting in a tutorial.

Tutorials connect with hardcoded credentials and assume WiFi
always works. Production firmware handles connection failures,
reconnects automatically, manages credentials securely in NVS,
and recovers gracefully from every possible failure scenario.

This session covers all of it — from the first connection to
robust production-grade WiFi management that your product can
rely on in the field for years.

---

## What You Will Learn

- How WiFi works on the ESP32 — the driver and event system
- Connecting to WiFi using the ESP-IDF WiFi stack
- The WiFi event loop — handling every connection state
- Storing WiFi credentials securely in NVS
- Automatic reconnection — what to do when WiFi drops
- WiFi provisioning — letting users configure credentials
  without hardcoding them
- SmartConfig and BLE provisioning overview
- IP address acquisition and network readiness detection
- Production WiFi patterns — retry logic, backoff, watchdog

---

## No Cloud Required

All experiments in this session work on your local network.
No internet connection needed — just a WiFi router.
MQTT broker is local (test.mosquitto.org or local broker).

---

## Experiments in This Session

| # | Name | What it shows |
|---|---|---|
| 01 | Basic WiFi connect | First connection, IP acquired |
| 02 | WiFi event handling | All events logged and handled |
| 03 | Credentials in NVS | Store and retrieve SSID + password |
| 04 | Auto reconnect | Disconnect simulation + auto recovery |
| 05 | Retry with backoff | Exponential backoff on failure |
| 06 | Full robust WiFi manager | Production-ready WiFi component |

---

## Hardware Required

- ESP32 DevKit — any variant
- WiFi router or mobile hotspot
- No additional sensors needed for this session

---

## Key ESP-IDF WiFi API

```c
// Initialise WiFi
esp_netif_init();
esp_event_loop_create_default();
esp_netif_create_default_wifi_sta();
wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
esp_wifi_init(&cfg);

// Register event handlers
esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                            &wifi_event_handler, NULL);
esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                            &wifi_event_handler, NULL);

// Configure and start
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
esp_wifi_start();
esp_wifi_connect();
```

---

## Key Takeaways

By the end of this session you should be able to answer:

1. What is the difference between WiFi connected and IP acquired?
2. Why should WiFi credentials never be hardcoded in firmware?
3. What happens when WiFi drops during operation?
4. What is exponential backoff and why does it matter?
5. How does NVS store credentials that survive device restart?

---

## Resources

- [ESP-IDF WiFi Driver Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/wifi.html)
- [ESP-IDF WiFi Station Example](https://github.com/espressif/esp-idf/tree/master/examples/wifi/getting_started/station)
- [ESP-IDF NVS Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**08 — MQTT pub/sub + TLS** →
Publishing sensor data to the cloud over a secure MQTT connection