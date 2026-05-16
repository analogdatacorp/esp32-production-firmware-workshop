# 05 — I2C and UART Production Patterns

## Session Overview

I2C and UART are the two most common communication protocols in embedded
systems. Almost every sensor, display and peripheral you will ever use speaks
one of these two protocols.

This session covers both — starting from what they are, how they work, and
then jumping straight into production-grade code. You will read a real sensor
over I2C, display data on an OLED screen over I2C, and communicate between
devices using UART — all using ESP-IDF drivers.

---

## What You Will Learn

- What I2C is — the two wire protocol explained simply
- I2C master configuration and initialisation in ESP-IDF
- Reading a DHT-style temperature sensor over I2C
- Driving an SSD1306 OLED display over I2C
- What UART is — serial communication explained
- UART configuration and event-driven receive
- UART transmit and receive patterns
- Choosing between I2C and UART for a given sensor

---

## Hardware Required

| Component | Quantity | Notes |
|---|---|---|
| ESP32 board | 1 | Any variant |
| SSD1306 OLED display | 1 | 128x64, I2C version |
| BMP280 or SHT31 sensor | 1 | I2C temperature/pressure sensor |
| USB-to-UART module | 1 | For UART demo (optional) |
| Jumper wires | 6–8 | |
| Breadboard | 1 | |

**No hardware? Use Wokwi — all demos work in browser.**
Wokwi link: see `wokwi/diagram.json` in this folder.

---

## Pin Connections

### I2C

| Signal | ESP32 Pin | Peripheral Pin |
|---|---|---|
| SDA | GPIO 21 | SDA |
| SCL | GPIO 22 | SCL |
| VCC | 3.3V | VCC |
| GND | GND | GND |

Both the OLED and sensor connect to the same SDA and SCL pins.
I2C is a bus — multiple devices share the same two wires.

### UART

| Signal | ESP32 Pin | Notes |
|---|---|---|
| TX | GPIO 17 | Transmit — connect to RX of other device |
| RX | GPIO 16 | Receive — connect to TX of other device |
| GND | GND | Always share ground |

---

## I2C Addresses Used

| Device | I2C Address |
|---|---|
| SSD1306 OLED | 0x3C (default) or 0x3D |
| BMP280 sensor | 0x76 (default) or 0x77 |
| SHT31 sensor | 0x44 (default) or 0x45 |

---

## Key Concepts

### I2C Init

```c
i2c_config_t conf = {
    .mode             = I2C_MODE_MASTER,
    .sda_io_num       = GPIO_NUM_21,
    .scl_io_num       = GPIO_NUM_22,
    .sda_pullup_en    = GPIO_PULLUP_ENABLE,
    .scl_pullup_en    = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000   // 400 kHz Fast Mode
};
i2c_param_config(I2C_NUM_0, &conf);
i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
```

### UART Init

```c
uart_config_t uart_config = {
    .baud_rate  = 115200,
    .data_bits  = UART_DATA_8_BITS,
    .parity     = UART_PARITY_DISABLE,
    .stop_bits  = UART_STOP_BITS_1,
    .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};
uart_param_config(UART_NUM_1, &uart_config);
uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_16, -1, -1);
uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
```

---

## Key Takeaways

By the end of this session you should be able to answer:

1. How many wires does I2C use and what does each one do?
2. How do multiple devices share the same I2C bus?
3. What is an I2C address and why does it matter?
4. What is the difference between I2C and UART?
5. Why do we use event-driven UART receive instead of polling?

---

## Resources

- [ESP-IDF I2C Driver Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html)
- [ESP-IDF UART Driver Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html)
- [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**Project 1 — Sensor Reading Pipeline** → Putting GPIO, I2C and UART together
into a complete production project