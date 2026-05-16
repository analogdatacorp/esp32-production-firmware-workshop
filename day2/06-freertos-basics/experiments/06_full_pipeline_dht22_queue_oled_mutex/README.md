# Experiment 06: Full FreeRTOS Pipeline with SSD1306 OLED

This project demonstrates a production-style ESP32 FreeRTOS pipeline:

```text
sensor_task -> display_queue -> display_task -> SSD1306 OLED
            -> log_queue     -> log_task     -> Serial monitor
```

The sensor values are simulated DHT22-like temperature and humidity readings. No DHT22 driver is required for this experiment.

## What This Project Shows

- Creating multiple FreeRTOS tasks
- Passing structured data through queues
- Fan-out using separate queues for separate consumers
- Protecting I2C/OLED access with a mutex
- Adding and registering a local ESP-IDF component
- Displaying text on a 128x64 SSD1306 OLED over I2C

## Hardware / Wokwi Connections

The code uses I2C on these ESP32 pins:

| OLED Pin | ESP32 Pin |
| --- | --- |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

The display is initialized as a 128x64 SSD1306 OLED.

## Project Structure

```text
.
├── CMakeLists.txt
├── main
│   ├── CMakeLists.txt
│   └── main.c
├── components
│   └── ssd1306
│       ├── CMakeLists.txt
│       └── components
│           └── ssd1306
│               ├── ssd1306.h
│               ├── ssd1306.c
│               ├── ssd1306_i2c_new.c
│               ├── ssd1306_i2c_legacy.c
│               └── ssd1306_spi.c
├── diagram.json
└── wokwi.toml
```

## How The SSD1306 Library Was Added

This project uses a local vendored SSD1306 component under:

```text
components/ssd1306/components/ssd1306
```

ESP-IDF only auto-detects components one level under `components/`, so a wrapper component file was added at:

```text
components/ssd1306/CMakeLists.txt
```

That wrapper registers the real SSD1306 source files:

```cmake
idf_component_register(
    SRCS "${component_srcs}"
    INCLUDE_DIRS "${ssd1306_src_dir}"
    REQUIRES "${component_requires}"
)
```

It also exposes the driver dependencies needed by `ssd1306.h`:

```cmake
set(component_requires driver esp_driver_spi esp_driver_i2c esp_driver_gpio)
```

The main component then depends on `ssd1306`:

```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ssd1306 esp_driver_gpio)
```

After that, application code can include:

```c
#include "ssd1306.h"
```

## How The Code Works

### 1. Data Structure

Sensor readings are passed as one struct:

```c
typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;
```

Queues copy this whole struct, so tasks do not share pointers or mutable sensor buffers.

### 2. Queues

The app creates two queues:

```c
static QueueHandle_t display_queue = NULL;
static QueueHandle_t log_queue = NULL;
```

This is intentional. If two tasks receive from the same FreeRTOS queue, each item is consumed by only one task. Separate queues ensure both the display task and logger receive every sample.

### 3. Mutex

The OLED uses I2C, which is a shared hardware resource. A mutex protects display access:

```c
static SemaphoreHandle_t i2c_mutex = NULL;
```

Before updating the OLED:

```c
xSemaphoreTake(i2c_mutex, portMAX_DELAY);
oled_show_sensor_data(data.temperature, data.humidity);
xSemaphoreGive(i2c_mutex);
```

### 4. OLED Initialization

The OLED is initialized on GPIO 21 and GPIO 22:

```c
i2c_master_init(&dev, I2C_SDA_GPIO, I2C_SCL_GPIO, -1);
ssd1306_init(&dev, 128, 64);
ssd1306_clear_screen(&dev, false);
ssd1306_contrast(&dev, 0xFF);
```

The reset pin is `-1` because this circuit does not use a separate OLED reset pin.

### 5. Sensor Task

`sensor_task` generates simulated temperature and humidity every 2 seconds:

```c
data.temperature = 27.0f + (sinf(angle) * 3.0f);
data.humidity = 60.0f + (cosf(angle) * 10.0f);
```

Then it sends the same reading to both queues:

```c
xQueueSend(display_queue, &data, 0);
xQueueSend(log_queue, &data, 0);
```

### 6. Display Task

`display_task` waits for data from `display_queue`, then updates the OLED:

```c
xQueueReceive(display_queue, &data, portMAX_DELAY);
oled_show_sensor_data(data.temperature, data.humidity);
```

### 7. Log Task

`log_task` waits for data from `log_queue` and prints it to the serial monitor:

```c
ESP_LOGI(
    "LOGGER",
    "Logger received -> Temp: %.1f C | Humi: %.1f %%",
    data.temperature,
    data.humidity
);
```

## Build

From this project directory:

```sh
idf.py build
```

If `idf.py` is not on your shell path, source the ESP-IDF export script first:

```sh
. $IDF_PATH/export.sh
idf.py build
```

## Flash And Monitor

For a physical ESP32:

```sh
idf.py -p /dev/ttyUSB0 flash monitor
```

On macOS, the port may look like:

```sh
idf.py -p /dev/cu.usbserial-0001 flash monitor
```

Exit the monitor with:

```text
Ctrl + ]
```

## Expected Output

The serial monitor should show logs similar to:

```text
I PIPELINE: Experiment 06: Full Pipeline Started
I PIPELINE: Sensor sent -> Temp: 27.0 C | Humi: 70.0 %
I PIPELINE: Display updated
I LOGGER: Logger received -> Temp: 27.0 C | Humi: 70.0 %
```

The OLED should show:

```text
FreeRTOS Pipeline

Queue + Mutex Demo

Temp: 27.0 C
Humi: 70.0 %
```

## Important Notes

- This demo uses simulated sensor values, not a real DHT22.
- The OLED is driven over I2C.
- The mutex is included to model safe shared peripheral access.
- Separate queues are used because a FreeRTOS queue item is consumed by only one receiver.
- The local SSD1306 wrapper is needed because the vendored library is nested deeper than ESP-IDF's default component scan.

## Common Issues

### `ssd1306.h: No such file or directory`

Check that:

- `components/ssd1306/CMakeLists.txt` exists.
- `main/CMakeLists.txt` has `REQUIRES ssd1306`.
- The library files exist under `components/ssd1306/components/ssd1306`.

### `undefined reference to ssd1306_*`

The header is visible, but the SSD1306 source files are not being compiled. Check the wrapper component CMake file.

### OLED does not display anything

Check:

- SDA is connected to GPIO 21.
- SCL is connected to GPIO 22.
- OLED address is `0x3C`.
- OLED type is SSD1306 128x64.
- Power and ground are connected correctly.

### Display and logger do not both receive every reading

Use two queues, one for each consumer. A single FreeRTOS queue does not broadcast to multiple receivers.
