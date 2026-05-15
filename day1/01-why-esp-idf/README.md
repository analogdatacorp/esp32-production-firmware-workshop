# 01 — Why ESP-IDF Over Arduino

## Session Overview

Most engineers start their ESP32 journey with Arduino. It's the perfect tool for
getting started — but when your firmware needs to run reliably in a real product,
Arduino shows its limits fast.

This session explains exactly what changes when you move to ESP-IDF, what you gain,
and why the industry builds production firmware this way.

---

## What You'll Learn

- What Arduino actually is under the hood and why it abstracts too much
- What ESP-IDF is and how Espressif designed it for production use
- The key differences — build system, memory model, task management
- Why companies like Bosch, Siemens and Espressif themselves use ESP-IDF
- When Arduino is still the right choice and when it isn't

---

## No Hardware Required

This is a concepts and comparison session.
No board, no wiring, no Wokwi needed.
Just follow along with the slides.

---

## Key Takeaways

By the end of this session you should be able to answer:

1. Why does `delay()` block everything in Arduino?
2. What does ESP-IDF give you that Arduino doesn't?
3. What is a component in ESP-IDF?
4. Why is the ESP-IDF build system better for large projects?
5. When would you still choose Arduino over ESP-IDF?

---

## Resources

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/)
- [ESP-IDF GitHub Repository](https://github.com/espressif/esp-idf)
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**02 — ESP32 Architecture** → Understanding the hardware you're programming