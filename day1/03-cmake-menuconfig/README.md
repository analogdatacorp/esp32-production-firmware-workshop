# 03 — CMake + Project Structure + Menuconfig

## Session Overview

Every professional software project needs a build system — a tool that knows
how to take your source code and turn it into a working binary. ESP-IDF uses
CMake. This session explains what CMake does, how an ESP-IDF project is
structured, and how to configure your project using Menuconfig.

By the end of this session you will have set up your first real ESP-IDF project
from scratch — not from a template, but understanding every single file and
every single line.

---

## What You Will Learn

- What a build system is and why CMake exists
- The ESP-IDF project folder structure — every file explained
- What CMakeLists.txt does and how to write it
- The `idf.py` command workflow — build, flash, monitor, clean
- What Menuconfig is and how to configure your project
- What sdkconfig is and why you should never edit it manually
- How to split code into components for production projects

---

## No Hardware Required

All concepts in this session are software-side.
You will run `idf.py build` at the end — Wokwi is not needed yet.

---

## Commands Used in This Session

```bash
# Create a new project
idf.py create-project my_project

# Set the target chip
idf.py set-target esp32

# Open menuconfig
idf.py menuconfig

# Build the project
idf.py build

# Flash to device
idf.py -p PORT flash

# Open serial monitor
idf.py -p PORT monitor

# Build + flash + monitor in one command
idf.py -p PORT flash monitor

# Clean the build
idf.py fullclean
```

---

## Key Files in Every ESP-IDF Project

| File | Purpose |
|---|---|
| `CMakeLists.txt` (root) | Top-level build definition |
| `CMakeLists.txt` (main/) | Main component build definition |
| `main/main.c` | Your application entry point |
| `sdkconfig` | Generated config — never edit manually |
| `sdkconfig.defaults` | Your default config overrides |
| `dependencies.lock` | Component dependency lockfile |
| `build/` | Generated build output — never commit |

---

## Key Takeaways

By the end of this session you should be able to answer:

1. What does CMake do that the compiler alone cannot?
2. Why does every ESP-IDF project need two CMakeLists.txt files?
3. What does `idf.py set-target esp32` actually change?
4. Why should you never manually edit sdkconfig?
5. What is the difference between a component and a library?

---

## Resources

- [ESP-IDF Build System Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html)
- [CMake Official Docs](https://cmake.org/documentation/)
- [Analog Data Workshop Docs](https://docs.analogdata.io)

---

## Next Session

**04 — GPIO Production Patterns** → Writing your first real ESP-IDF hardware code