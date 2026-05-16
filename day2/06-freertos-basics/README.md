# 06 — FreeRTOS: Tasks, Queues and Semaphores That Actually Matter

## Session Overview

Yesterday you wrote firmware that does one thing at a time.
Today that changes.

FreeRTOS is the operating system running inside your ESP32.
It was already there in Session 01 — powering `vTaskDelay`,
scheduling `app_main`, managing memory. You just did not see it.

Today we pull back the curtain. You will understand what tasks
are, how the scheduler works, how tasks communicate safely using
queues, and how they share resources safely using semaphores.

Every production ESP32 product uses all three. By the end of
this session you will use them too.

---

## What You Will Learn

- What FreeRTOS is and why it exists on a microcontroller
- Tasks — creating, prioritising and deleting them
- The FreeRTOS scheduler — how it decides what runs when
- Why `vTaskDelay` is not `delay()` — the critical difference
- Queues — passing data between tasks safely
- Semaphores — protecting shared resources
- Mutex vs binary semaphore — when to use which
- Task watchdog — detecting stuck tasks in production
- Stack overflow detection — finding memory bugs early
- Practical patterns used in every real ESP32 product

---

## No Hardware Required for Most Experiments

Experiments 01 to 05 run on Wokwi or bare ESP32 — no sensor needed.
Experiment 06 uses the DHT22 and OLED from Project 1.

---

## Experiments in This Session

| # | Name | What it shows |
|---|---|---|
| 01 | Two tasks blinking two LEDs | Tasks running simultaneously |
| 02 | Task priorities | Higher priority task starves lower |
| 03 | Queue — producer consumer | Passing sensor data between tasks |
| 04 | Semaphore — shared UART | Two tasks printing without garbling |
| 05 | Mutex — shared counter | Race condition then fix with mutex |
| 06 | Full pipeline — sensor + queue + display | Everything together |

---

## Key FreeRTOS API Used Today

```c
// Tasks
xTaskCreate(func, name, stack, param, priority, handle);
xTaskCreatePinnedToCore(func, name, stack, param, prio, handle, core);
vTaskDelete(NULL);
vTaskDelay(pdMS_TO_TICKS(1000));
uxTaskGetStackHighWaterMark(NULL);

// Queues
xQueueCreate(length, item_size);
xQueueSend(queue, &item, timeout);
xQueueReceive(queue, &item, timeout);
xQueueSendFromISR(queue, &item, NULL);

// Semaphores and Mutexes
xSemaphoreCreateBinary();
xSemaphoreCreateMutex();
xSemaphoreTake(sem, timeout);
xSemaphoreGive(sem);
```

---

## Key Takeaways

By the end of this session you should be able to answer:

1. What happens when two tasks have the same priority?
2. Why does a task calling `vTaskDelay` not block other tasks?
3. What is the difference between a queue and a global variable?
4. What is a race condition and how does a mutex prevent it?
5. When would you use a binary semaphore instead of a mutex?

---

## Resources

- [FreeRTOS Kernel Docs](https://www.freertos.org/Documentation/RTOS_book.html)
- [ESP-IDF FreeRTOS Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos.html)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**07 — WiFi Provisioning and Reconnection** →
Connecting your ESP32 to the network reliably