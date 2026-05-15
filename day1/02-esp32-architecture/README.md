# 02 — ESP32 Architecture

## Session Overview

Before writing a single line of firmware, you need to understand what's inside
the chip you're programming. The ESP32 is not just a microcontroller — it's a
complete system-on-chip with dual cores, a rich peripheral set, multiple memory
regions and a wireless subsystem all on one die.

This session walks through the ESP32 internals that matter for firmware development —
the parts that directly affect how you write, structure and debug your code.

---

## What You'll Learn

- ESP32 dual-core architecture — what runs on Core 0 vs Core 1
- Memory map — IRAM, DRAM, flash, RTC memory and when to use each
- Peripheral bus architecture — how peripherals connect to the CPU
- Clock tree — system clock, peripheral clocks, power implications
- ESP32 chip variants — ESP32, S2, S3, C3, C6 — key differences
- Reset and boot sequence — what happens before `app_main()`

---

## No Hardware Required

This is an architecture and internals session.
Slides and diagrams only — no board, no Wokwi needed.

---

## Key Takeaways

By the end of this session you should be able to answer:

1. Which core runs your `app_main()` by default?
2. What is IRAM and when should you put code there?
3. What is the difference between DRAM and flash storage?
4. Why does the ESP32-S3 have more RAM than the ESP32?
5. What happens during the ESP32 boot sequence?

---

## ESP32 Variants Covered

| Chip | Cores | Architecture | Key Feature |
|---|---|---|---|
| ESP32 | 2 | Xtensa LX6 | Classic — WiFi + BT |
| ESP32-S3 | 2 | Xtensa LX7 | AI accelerator + USB |
| ESP32-C3 | 1 | RISC-V | Ultra low cost |
| ESP32-C6 | 1 | RISC-V | WiFi 6 + Zigbee |

---

## Resources

- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [ESP-IDF API Reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**03 — CMake + Project Structure + Menuconfig** → Setting up your first production project