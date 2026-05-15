### ESP-IDF v6.x | analogdata.blog

---

## Project Setup

|Command|What it does|
|---|---|
|`idf.py create-project <name>`|Create a new ESP-IDF project scaffold|
|`idf.py set-target <chip>`|**Run this first.** Set the target chip before anything else|
|`idf.py menuconfig`|Open ncurses config menu — FreeRTOS tick rate, flash size, log level, Wi-Fi settings|

**Supported targets for `set-target`:**

```bash
idf.py set-target esp32       # Original ESP32 (dual-core)
idf.py set-target esp32s3     # ESP32-S3 (dual-core, USB OTG) ← XIAO ESP32-S3
idf.py set-target esp32s2     # ESP32-S2 (single-core)
idf.py set-target esp32c3     # ESP32-C3 (single-core, RISC-V)
idf.py set-target esp32c6     # ESP32-C6 (Wi-Fi 6 + Thread)
idf.py set-target esp32h2     # ESP32-H2 (Thread/Zigbee, no Wi-Fi)
```

---

## Build

|Command|What it does|
|---|---|
|`idf.py build`|Compile the project — full build first time, incremental after|
|`idf.py fullclean`|Delete all build artifacts — use when switching targets or after major config changes|
|`idf.py size`|Show flash and RAM usage summary|
|`idf.py size-components`|Detailed size per component — useful for trimming bloat|
|`idf.py size-files`|Size breakdown per object file|

---

## Flash

|Command|What it does|
|---|---|
|`idf.py -p <PORT> flash`|Flash firmware to board|
|`idf.py -p <PORT> flash monitor`|**Most used.** Flash and immediately open serial monitor|
|`idf.py -p <PORT> -b 921600 flash`|Flash at higher baud rate — faster upload|
|`idf.py erase-flash`|Wipe entire flash — use when NVS or partition table is corrupt|
|`idf.py erase-otadata`|Erase OTA data only — resets to factory app|
|`idf.py partition-table-flash`|Flash only the partition table|
|`idf.py app-flash`|Flash only the app binary — skips bootloader and partition table|
|`idf.py bootloader-flash`|Flash only the bootloader|

**Common port values:**

```bash
# Linux
idf.py -p /dev/ttyUSB0 flash monitor
idf.py -p /dev/ttyACM0 flash monitor

# macOS
idf.py -p /dev/cu.usbserial-0001 flash monitor
idf.py -p /dev/cu.usbmodem101 flash monitor

# Windows
idf.py -p COM3 flash monitor
```

---

## Monitor

|Command|What it does|
|---|---|
|`idf.py -p <PORT> monitor`|Open serial monitor only — no flash|
|`idf.py -p <PORT> --baud 115200 monitor`|Set baud rate explicitly — default is 115200|

### Monitor Keyboard Shortcuts

|Shortcut|Action|
|---|---|
|`Ctrl + ]`|**Exit monitor**|
|`Ctrl + T` then `Ctrl + R`|Reboot the device|
|`Ctrl + T` then `Ctrl + F`|Toggle log timestamps|
|`Ctrl + T` then `Ctrl + H`|Show all monitor shortcuts (help)|
|`Ctrl + T` then `Ctrl + P`|Reset target into bootloader / download mode|
|`Ctrl + T` then `Ctrl + X`|Exit monitor (alternative if `Ctrl + ]` doesn't work)|

---

## Build + Flash + Monitor (Combined)

```bash
# Full workflow — most common during development
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor

# One-liner after target is already set
idf.py -p /dev/ttyACM0 build flash monitor

# Fast flash at high baud rate
idf.py -p /dev/ttyACM0 -b 921600 flash monitor
```

---

## Debugging & Diagnostics

|Command|What it does|
|---|---|
|`idf.py gdb`|Launch GDB debugger (requires JTAG setup)|
|`idf.py gdbtui`|GDB with terminal UI|
|`idf.py openocd`|Start OpenOCD server for JTAG debugging|
|`idf.py coredump-info`|Decode and display a core dump|
|`idf.py coredump-debug`|Open GDB with core dump loaded|

---

## Components & Dependencies

|Command|What it does|
|---|---|
|`idf.py add-dependency <component>`|Add a component from the ESP-IDF Component Registry|
|`idf.py update-dependencies`|Update all managed components to latest versions|
|`idf.py create-component <name>`|Scaffold a new reusable component|

**Example — add OLED driver:**

```bash
idf.py add-dependency "espressif/esp_lcd_ssd1306"
```

---

## Partition Table

|Command|What it does|
|---|---|
|`idf.py partition-table`|Show the current partition table|
|`idf.py partition-table-flash`|Flash only the partition table|
|`idf.py read-otadata`|Read current OTA data partition|
|`idf.py erase-otadata`|Erase OTA data — reset to factory partition|

---

## Environment & Info

|Command|What it does|
|---|---|
|`idf.py --version`|Show current ESP-IDF version|
|`idf.py --list-targets`|List all supported chip targets|
|`idf.py --help`|Show all available commands|
|`idf.py <command> --help`|Show help for a specific command|

---

## Common Workflows

### First time on a new project

```bash
cd my_project
idf.py set-target esp32s3
idf.py menuconfig          # optional — configure if needed
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

### Switching target chip

```bash
idf.py fullclean           # clear old build artifacts
idf.py set-target esp32c3  # set new target
idf.py build
```

### Something is broken — start clean

```bash
idf.py fullclean
idf.py erase-flash         # wipe the board too
idf.py -p /dev/ttyACM0 build flash monitor
```

### Check firmware size before shipping

```bash
idf.py size
idf.py size-components     # see which component is eating your flash
```

---

## Troubleshooting

|Problem|Fix|
|---|---|
|`idf.py: command not found`|Run `. $HOME/esp/esp-idf/export.sh`|
|`IDF_PATH not set`|Re-run `export.sh` — add it to `.bashrc` to persist|
|Permission denied on port (Linux)|`sudo usermod -aG dialout $USER` then re-login|
|Board not detected (macOS)|Install CP210x or CH340 USB driver|
|Build fails after `set-target` change|Run `idf.py fullclean` first|
|Flash stuck at "Connecting..."|Hold BOOT button while flashing starts, release after|
|`A fatal error occurred: Failed to connect`|Wrong port, wrong baud, or missing USB driver|
|Garbage output in monitor|Baud rate mismatch — check `CONFIG_ESP_CONSOLE_UART_BAUDRATE` in menuconfig|

---

## Quick Reference Card

```
SETUP        idf.py set-target <chip>
             idf.py menuconfig

BUILD        idf.py build
             idf.py fullclean

FLASH        idf.py -p <PORT> flash
             idf.py -p <PORT> flash monitor   ← daily driver

MONITOR      idf.py -p <PORT> monitor
             Ctrl + ]                         ← exit monitor

DIAGNOSE     idf.py size
             idf.py size-components

CLEAN START  idf.py fullclean && idf.py erase-flash
```

---

> **Full ESP-IDF curriculum → [build.analogdata.io](https://build.analogdata.io/)**  
> **More posts like this → [analogdata.blog](https://analogdata.blog/)**  
> **YouTube tutorials → [youtube.com/@analogdataio](https://www.youtube.com/@analogdataio)**