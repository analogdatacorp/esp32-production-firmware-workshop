# 04 — GPIO Production Patterns

## Session Overview

GPIO — General Purpose Input Output — is the most fundamental hardware
interface on any microcontroller. Every embedded engineer knows `digitalWrite`.
But production firmware does GPIO very differently from Arduino sketches.

This session covers how ESP-IDF handles GPIO at a production level — proper
configuration structs, interrupt-driven input handling, debounce patterns,
and the ISR-to-task communication pattern that every real product uses.

By the end of this session you will have replaced every Arduino GPIO habit
with the correct ESP-IDF approach — and you will understand exactly why.

---

## What You Will Learn

- The `gpio_config_t` struct — configuring GPIO the right way
- Output patterns — setting, clearing and toggling pins
- Input patterns — polling vs interrupts
- Why you should never poll GPIO in a loop
- Software debounce — the right way and the wrong way
- ISR handlers — writing them correctly with IRAM_ATTR
- ISR to task communication — using queues from an interrupt
- GPIO hold and wakeup — for low power designs
- Real hardware wiring — LED, push button, pull-up and pull-down

---

## Hardware Required

| Component | Quantity | Notes |
|---|---|---|
| ESP32 board | 1 | Any variant |
| LED | 1 | Any colour |
| Push button | 1 | Momentary tactile switch |
| Resistor 220Ω | 1 | LED current limiter |
| Resistor 10kΩ | 1 | External pull-down (optional) |
| Breadboard | 1 | |
| Jumper wires | 4–6 | |

**No hardware? Use Wokwi simulator — all demos work in browser.**
Wokwi link: see `wokwi/diagram.json` in this folder.

---

## GPIO Pins Used in This Session

| Signal | GPIO Pin | Direction |
|---|---|---|
| LED | GPIO 2 | Output |
| Push Button | GPIO 0 | Input |

GPIO 0 and GPIO 2 are used because they are available on all ESP32 variants
and visible on all common dev boards.

---

## Key Concepts

### gpio_config_t

```c
gpio_config_t config = {
    .pin_bit_mask = (1ULL << GPIO_NUM_2),
    .mode         = GPIO_MODE_OUTPUT,
    .pull_up_en   = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type    = GPIO_INTR_DISABLE
};
gpio_config(&config);
```

### Output Control

```c
gpio_set_level(GPIO_NUM_2, 1);   // HIGH — LED on
gpio_set_level(GPIO_NUM_2, 0);   // LOW  — LED off
```

### Interrupt Handler

```c
void IRAM_ATTR button_isr_handler(void *arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
```

---

## Key Takeaways

By the end of this session you should be able to answer:

1. What is the difference between `gpio_config_t` and `pinMode()`?
2. Why should GPIO input never be polled in a while loop?
3. Why must ISR handlers have the `IRAM_ATTR` attribute?
4. What is the ISR-to-task queue pattern and why does it exist?
5. What causes button bounce and how do you fix it in firmware?

---

## Wokwi Simulation

Open `wokwi/diagram.json` at wokwi.com to run this session
without physical hardware. The simulation includes:
- LED on GPIO 2
- Push button on GPIO 0
- All connections pre-wired

---

## Resources

- [ESP-IDF GPIO Driver Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html)
- [ESP-IDF GPIO Example](https://github.com/espressif/esp-idf/tree/master/examples/peripherals/gpio/generic_gpio)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**05 — I2C and UART** → Reading sensors and communicating with peripherals