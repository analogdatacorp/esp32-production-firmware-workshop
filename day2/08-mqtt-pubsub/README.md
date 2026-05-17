# 08 — MQTT: Real World IoT Connectivity

## Session Overview

WiFi gives your ESP32 access to the network.
MQTT gives it a voice.

MQTT (Message Queuing Telemetry Transport) is the standard
protocol for IoT communication. It is lightweight, reliable,
runs over TCP, and is used by AWS IoT, Google Cloud IoT,
Azure IoT Hub, Home Assistant, Node-RED and virtually every
IoT platform in existence.

This session covers MQTT from first principles — what it is,
how it works, how to publish sensor data, how to subscribe
to commands, how to handle disconnections gracefully, and
how to build a production MQTT client on ESP32 using ESP-IDF.

---

## What You Will Learn

- What MQTT is and why it became the standard for IoT
- The broker, publisher and subscriber model
- Topics — how data is organised and addressed
- QoS levels — 0, 1, 2 and when to use each
- Last Will and Testament — detecting dead devices
- Connecting to a public broker (test.mosquitto.org)
- Publishing sensor data as JSON
- Subscribing to command topics
- Handling MQTT disconnection and automatic reconnect
- Production MQTT patterns — retained messages, clean session

---

## Hardware Required

- ESP32 DevKit
- WiFi connection (router or hotspot)
- DHT22 sensor (for final experiment)
- SSD1306 OLED (optional — for final experiment)
- No cloud account needed — uses public test broker

---

## MQTT Broker Used in This Session

**Public test broker:**
```
Host     : test.mosquitto.org
Port     : 1883  (plain)
Port     : 8883  (TLS)
Port     : 8884  (TLS + client certificate)
```

**Local broker alternative (recommended for production):**
```bash
# Install Mosquitto on your PC
# macOS
brew install mosquitto

# Ubuntu
sudo apt install mosquitto

# Run locally
mosquitto -v
# Default port: 1883
# Your PC IP: check with ifconfig or ipconfig
```

---

## Topics Used in This Session

```
workshop/esp32/sensors/temperature
workshop/esp32/sensors/humidity
workshop/esp32/sensors/all
workshop/esp32/status
workshop/esp32/commands/led
workshop/esp32/commands/interval
```

---

## Experiments in This Session

| # | Name | What it shows |
|---|---|---|
| 01 | First MQTT connect | Connect to broker, log events |
| 02 | Publish sensor data | Publish temperature as plain text |
| 03 | Publish JSON payload | Structured data with metadata |
| 04 | Subscribe to commands | Receive LED on/off from broker |
| 05 | Last Will and Testament | Detect device offline automatically |
| 06 | Full IoT pipeline | DHT22 + MQTT + OLED + commands |

---

## Key ESP-IDF MQTT API

```c
// Configure
esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtt://test.mosquitto.org",
};

// Create client
esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

// Register event handler
esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                mqtt_event_handler, NULL);
// Start
esp_mqtt_client_start(client);

// Publish
esp_mqtt_client_publish(client, "topic", "payload", 0, 1, 0);

// Subscribe
esp_mqtt_client_subscribe(client, "topic", 1);
```

---

## Key Takeaways

By the end of this session you should be able to answer:

1. What is the difference between MQTT and HTTP for IoT?
2. What does QoS 1 guarantee that QoS 0 does not?
3. What is a retained message and when would you use it?
4. What is Last Will and Testament and how does it work?
5. Why is JSON the preferred payload format for MQTT?

---

## Resources

- [ESP-IDF MQTT Client Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/mqtt.html)
- [MQTT Protocol Specification](https://mqtt.org/mqtt-specification/)
- [Mosquitto Public Broker](https://test.mosquitto.org)
- [MQTT Explorer — GUI client for testing](https://mqtt-explorer.com)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next

**Project 2 — WiFi MQTT Sensor Node** →
Combining everything from Day 2 into one complete production project