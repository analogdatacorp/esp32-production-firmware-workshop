/*
 * SSD1306 OLED Initialization — Experiment 02
 * ESP32 Production Firmware Workshop
 * Analog Data | analogdata.io
 *
 * Hardware:
 *   SSD1306 OLED 128x64 I2C
 *   SDA -> GPIO 21
 *   SCL -> GPIO 22
 *   VCC -> 3.3V
 *   GND -> GND
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "esp_lcd_panel_ssd1306.h"

/* -------------------------------------------------------
 * Tag for log filtering
 * ------------------------------------------------------- */
static const char *TAG = "oled_init";

/* -------------------------------------------------------
 * I2C Configuration
 * ------------------------------------------------------- */
#define I2C_MASTER_PORT        I2C_NUM_0
#define I2C_MASTER_SDA_IO      GPIO_NUM_8
#define I2C_MASTER_SCL_IO      GPIO_NUM_9
#define I2C_MASTER_FREQ_HZ     400000

/* -------------------------------------------------------
 * OLED Configuration
 * ------------------------------------------------------- */
#define OLED_I2C_ADDR          0x3C
#define OLED_H_RES             128
#define OLED_V_RES             64
#define OLED_RESET_GPIO        -1

/* -------------------------------------------------------
 * Handles — global so all functions can access
 * ------------------------------------------------------- */
static i2c_master_bus_handle_t   i2c_bus_handle    = NULL;
static esp_lcd_panel_io_handle_t oled_io_handle    = NULL;
static esp_lcd_panel_handle_t    oled_panel_handle = NULL;

/* -------------------------------------------------------
 * Frame buffer
 * 128 x 64 pixels, 1 bit per pixel
 * Total: 128 * 64 / 8 = 1024 bytes
 * ------------------------------------------------------- */
static uint8_t oled_frame_buffer[OLED_H_RES * OLED_V_RES / 8];

// * i2c_master_init()
// * Initialises the I2C master bus on GPIO 21/22

static void i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_MASTER_PORT,
        .sda_io_num                   = I2C_MASTER_SDA_IO,
        .scl_io_num                   = I2C_MASTER_SCL_IO,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));

    ESP_LOGI(TAG, "I2C master initialised — SDA GPIO%d  SCL GPIO%d  @ %d Hz",
             I2C_MASTER_SDA_IO,
             I2C_MASTER_SCL_IO,
             I2C_MASTER_FREQ_HZ);
}

/* -------------------------------------------------------
 * oled_init()
 * Initialises the SSD1306 OLED over I2C
 * ------------------------------------------------------- */
static void oled_init(void)
{
    /* Step 1 — Create I2C panel IO handle */
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr            = OLED_I2C_ADDR,
        .scl_speed_hz        = I2C_MASTER_FREQ_HZ,
        .control_phase_bytes = 1,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .dc_bit_offset       = 6,
        .flags = {
            .dc_low_on_data        = false,
            .disable_control_phase = false,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(
        i2c_bus_handle,
        &io_config,
        &oled_io_handle
    ));

    /* Step 2 — SSD1306 vendor specific config */
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_V_RES,
    };

    /* Step 3 — Panel device config */
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = OLED_RESET_GPIO,
        .vendor_config  = &ssd1306_config,
    };

    /* Step 4 — Create SSD1306 panel handle */
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(
        oled_io_handle,
        &panel_config,
        &oled_panel_handle
    ));

    /* Step 5 — Reset, init, turn on */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(oled_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(oled_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(oled_panel_handle, true));

    /* Step 6 — Wait for display to wake up */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "SSD1306 OLED initialised — %d x %d pixels",
             OLED_H_RES, OLED_V_RES);
}

/* -------------------------------------------------------
 * oled_flush()
 * Pushes frame buffer to display
 * Call this after any change to oled_frame_buffer
 * ------------------------------------------------------- */
static void oled_flush(void)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
        oled_panel_handle,
        0, 0,
        OLED_H_RES, OLED_V_RES,
        oled_frame_buffer
    ));
}

/* -------------------------------------------------------
 * oled_clear()
 * Clears the frame buffer and updates display
 * ------------------------------------------------------- */
static void oled_clear(void)
{
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
    oled_flush();
    ESP_LOGI(TAG, "Display cleared");
}

/* -------------------------------------------------------
 * oled_set_pixel()
 * Sets or clears a single pixel in the frame buffer
 * x: 0 to 127
 * y: 0 to 63
 * color: 1 = on, 0 = off
 * ------------------------------------------------------- */
static void oled_set_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= OLED_H_RES) return;
    if (y < 0 || y >= OLED_V_RES) return;

    int byte_index = (y / 8) * OLED_H_RES + x;
    int bit_index  = y % 8;

    if (color) {
        oled_frame_buffer[byte_index] |=  (1 << bit_index);
    } else {
        oled_frame_buffer[byte_index] &= ~(1 << bit_index);
    }
}

/* -------------------------------------------------------
 * oled_fill_rect()
 * Fills a rectangle in the frame buffer
 * x, y: top-left corner
 * w: width, h: height
 * color: 1 = on, 0 = off
 * ------------------------------------------------------- */
static void oled_fill_rect(int x, int y, int w, int h, uint8_t color)
{
    for (int row = y; row < y + h; row++) {
        for (int col = x; col < x + w; col++) {
            oled_set_pixel(col, row, color);
        }
    }
}

/* -------------------------------------------------------
 * oled_draw_border()
 * Draws a 1px border around the entire display
 * ------------------------------------------------------- */
static void oled_draw_border(void)
{
    /* Top row */
    for (int x = 0; x < OLED_H_RES; x++) {
        oled_set_pixel(x, 0, 1);
    }
    /* Bottom row */
    for (int x = 0; x < OLED_H_RES; x++) {
        oled_set_pixel(x, OLED_V_RES - 1, 1);
    }
    /* Left column */
    for (int y = 0; y < OLED_V_RES; y++) {
        oled_set_pixel(0, y, 1);
    }
    /* Right column */
    for (int y = 0; y < OLED_V_RES; y++) {
        oled_set_pixel(OLED_H_RES - 1, y, 1);
    }
}

/* -------------------------------------------------------
 * oled_show_test_pattern()
 * Shows a series of patterns to verify display is working
 * ------------------------------------------------------- */
static void oled_show_test_pattern(void)
{
    /* Pattern 1 — All pixels ON */
    ESP_LOGI(TAG, "Pattern 1 — All pixels ON");
    memset(oled_frame_buffer, 0xFF, sizeof(oled_frame_buffer));
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Pattern 2 — All pixels OFF */
    ESP_LOGI(TAG, "Pattern 2 — All pixels OFF");
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Pattern 3 — Checkerboard */
    ESP_LOGI(TAG, "Pattern 3 — Checkerboard");
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
    for (int page = 0; page < OLED_V_RES / 8; page++) {
        for (int x = 0; x < OLED_H_RES; x++) {
            oled_frame_buffer[page * OLED_H_RES + x] =
                ((page + x) % 2 == 0) ? 0xAA : 0x55;
        }
    }
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Pattern 4 — Filled rectangle in the centre */
    ESP_LOGI(TAG, "Pattern 4 — Filled centre rectangle");
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
    oled_fill_rect(20, 16, 88, 32, 1);
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Pattern 5 — Border only */
    ESP_LOGI(TAG, "Pattern 5 — Border");
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
    oled_draw_border();
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Pattern 6 — Vertical stripes */
    ESP_LOGI(TAG, "Pattern 6 — Vertical stripes");
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
    for (int page = 0; page < OLED_V_RES / 8; page++) {
        for (int x = 0; x < OLED_H_RES; x++) {
            oled_frame_buffer[page * OLED_H_RES + x] =
                (x % 8 < 4) ? 0xFF : 0x00;
        }
    }
    oled_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* Pattern 7 — Clear screen */
    ESP_LOGI(TAG, "Pattern 7 — Clear");
    oled_clear();
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Test pattern sequence complete");
}

/* -------------------------------------------------------
 * app_main()
 * Entry point
 * ------------------------------------------------------- */
void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  SSD1306 OLED Init — Experiment 02");
    ESP_LOGI(TAG, "  Analog Data | analogdata.io");
    ESP_LOGI(TAG, "========================================");

    /* Step 1 — Init I2C bus */
    i2c_master_init();

    /* Step 2 — Init OLED display */
    oled_init();

    /* Step 3 — Run test patterns */
    oled_show_test_pattern();

    ESP_LOGI(TAG, "Experiment complete — display running");

    /* Step 4 — Blink display on/off to confirm control */
    int blink_count = 0;
    while (1) {
        blink_count++;

        /* Toggle display on/off every 2 seconds */
        if (blink_count % 2 == 0) {
            esp_lcd_panel_disp_on_off(oled_panel_handle, false);
            ESP_LOGI(TAG, "Display OFF");
        } else {
            esp_lcd_panel_disp_on_off(oled_panel_handle, true);
            ESP_LOGI(TAG, "Display ON");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}