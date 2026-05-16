/*
 * SSD1306 OLED Text Display — Experiment 03
 * ESP32 Production Firmware Workshop
 * Analog Data | analogdata.io
 *
 * Hardware:
 *   SSD1306 OLED 128x64 I2C
 *   SDA -> GPIO 21
 *   SCL -> GPIO 22
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * Demonstrates:
 *   - Custom 5x7 bitmap font rendering
 *   - Drawing text to frame buffer
 *   - Displaying counter on OLED
 *   - Border drawing
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
 * Tag for ESP_LOG filtering
 * ------------------------------------------------------- */
static const char *TAG = "oled_text";

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
 * Font Configuration
 * Each character is 5 pixels wide, 7 pixels tall
 * 1 pixel spacing between characters
 * Total character width on screen = 5 + 1 = 6 pixels
 * Max characters per row = 128 / 6 = 21 chars
 * ------------------------------------------------------- */
#define FONT_WIDTH             5
#define FONT_HEIGHT            7
#define FONT_SPACING           1

/* -------------------------------------------------------
 * Handles
 * ------------------------------------------------------- */
static i2c_master_bus_handle_t   i2c_bus_handle    = NULL;
static esp_lcd_panel_io_handle_t oled_io_handle    = NULL;
static esp_lcd_panel_handle_t    oled_panel_handle = NULL;

/* -------------------------------------------------------
 * Frame buffer
 * 128 x 64 pixels at 1 bit per pixel = 1024 bytes
 * Layout: 8 pages of 128 bytes each
 * Each page = 8 rows of pixels
 * Byte bit 0 = top pixel of page
 * Byte bit 7 = bottom pixel of page
 * ------------------------------------------------------- */
static uint8_t oled_frame_buffer[OLED_H_RES * OLED_V_RES / 8];

/* -------------------------------------------------------
 * 5x7 Bitmap Font Table
 *
 * Each character = 5 bytes (one per column)
 * Each byte = 7 bits (one per row, LSB = top row)
 *
 * Example for 'A':
 *   Col:  0     1     2     3     4
 *         0x7E  0x11  0x11  0x11  0x7E
 *   Binary:
 *         1111110  0001000  0001000  0001000  1111110
 *   Renders as:
 *         #####
 *         #   #
 *         #   #
 *         #####
 *         #   #
 *         #   #
 *         #   #
 *
 * Indexed directly by ASCII value for O(1) lookup.
 * Unused indices are zero-initialised (blank character).
 * ------------------------------------------------------- */
static const uint8_t font_5x7[128][5] = {
    /* Punctuation and symbols */
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x00, 0x00, 0x5F, 0x00, 0x00},
    ['"'] = {0x00, 0x07, 0x00, 0x07, 0x00},
    ['#'] = {0x14, 0x7F, 0x14, 0x7F, 0x14},
    ['$'] = {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    ['%'] = {0x62, 0x64, 0x08, 0x13, 0x23},
    ['&'] = {0x36, 0x49, 0x55, 0x22, 0x50},
    ['.'] = {0x00, 0x60, 0x60, 0x00, 0x00},
    [','] = {0x00, 0x50, 0x30, 0x00, 0x00},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['+'] = {0x08, 0x08, 0x3E, 0x08, 0x08},
    ['='] = {0x14, 0x14, 0x14, 0x14, 0x14},
    ['/'] = {0x20, 0x10, 0x08, 0x04, 0x02},
    ['('] = {0x00, 0x1C, 0x22, 0x41, 0x00},
    [')'] = {0x00, 0x41, 0x22, 0x1C, 0x00},
    ['_'] = {0x40, 0x40, 0x40, 0x40, 0x40},
    ['<'] = {0x08, 0x14, 0x22, 0x41, 0x00},
    ['>'] = {0x00, 0x41, 0x22, 0x14, 0x08},

    /* Digits 0-9 */
    ['0'] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1'] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1E},

    /* Uppercase A-Z */
    ['A'] = {0x7E, 0x11, 0x11, 0x11, 0x7E},
    ['B'] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D'] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F'] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G'] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I'] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['J'] = {0x20, 0x40, 0x41, 0x3F, 0x01},
    ['K'] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L'] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M'] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N'] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O'] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P'] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['Q'] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    ['R'] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U'] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V'] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W'] = {0x7F, 0x20, 0x18, 0x20, 0x7F},
    ['X'] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y'] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z'] = {0x61, 0x51, 0x49, 0x45, 0x43},

    /* Lowercase a-z — mapped to uppercase bitmaps
       using toupper() in oled_draw_char() */
};

/* =======================================================
 * I2C INITIALISATION
 * ======================================================= */

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

/* =======================================================
 * OLED INITIALISATION
 * ======================================================= */

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

    /* Step 2 — SSD1306 vendor config — set display height */
    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = OLED_V_RES,
    };

    /* Step 3 — Panel device config */
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = OLED_RESET_GPIO,
        .vendor_config  = &ssd1306_config,
    };

    /* Step 4 — Create panel driver instance */
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(
        oled_io_handle,
        &panel_config,
        &oled_panel_handle
    ));

    /* Step 5 — Reset, initialise, turn display on */
    ESP_ERROR_CHECK(esp_lcd_panel_reset(oled_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(oled_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(oled_panel_handle, true));

    /* Step 6 — Wait for display to fully wake up */
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "SSD1306 OLED initialised — %d x %d",
             OLED_H_RES, OLED_V_RES);
}

/* =======================================================
 * FRAME BUFFER OPERATIONS
 * ======================================================= */

/*
 * oled_flush()
 * Sends the entire frame buffer to the OLED over I2C.
 * Call this after any drawing operation to update the display.
 */
static void oled_flush(void)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
        oled_panel_handle,
        0, 0,
        OLED_H_RES, OLED_V_RES,
        oled_frame_buffer
    ));
}

/*
 * oled_clear_buffer()
 * Sets all pixels in frame buffer to OFF (0x00).
 * Does NOT flush — call oled_flush() after if needed.
 */
static void oled_clear_buffer(void)
{
    memset(oled_frame_buffer, 0x00, sizeof(oled_frame_buffer));
}

/*
 * oled_set_pixel()
 * Sets or clears a single pixel in the frame buffer.
 *
 * x     : 0 to 127 (left to right)
 * y     : 0 to 63  (top to bottom)
 * color : 1 = pixel ON, 0 = pixel OFF
 *
 * Frame buffer layout:
 *   Each page = 8 rows, one byte per column
 *   byte_index = (y / 8) * 128 + x
 *   bit_index  = y % 8  (bit 0 = top of page)
 */
static void oled_set_pixel(int x, int y, uint8_t color)
{
    /* Bounds check — silently ignore out of range */
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

/* =======================================================
 * FONT AND TEXT RENDERING
 * ======================================================= */

/*
 * oled_draw_char()
 * Draws a single character from the 5x7 font table
 * at position (x, y) in the frame buffer.
 *
 * x  : pixel column for left edge of character
 * y  : pixel row for top edge of character
 * ch : ASCII character to draw
 *
 * FIX: ch must be cast to unsigned char before calling
 * toupper() and before array indexing to avoid signed
 * comparison warnings in GCC with -Werror=type-limits.
 * On ESP32, plain char is unsigned, so ch < 0 is always
 * false — the compiler correctly flags this as an error.
 */
static void oled_draw_char(int x, int y, char ch)
{
    /* Cast to unsigned char before any comparisons.
     * This is the correct way to handle char with ctype.h
     * functions per the C standard. */
    unsigned char uch = (unsigned char)ch;

    /* Convert lowercase to uppercase for our font table */
    uch = (unsigned char)toupper((int)uch);

    /* Clamp to valid font table range
     * Our table is 128 entries — ASCII printable range */
    if (uch >= 128) {
        uch = (unsigned char)' ';
    }

    /* Render each column of the character */
    for (int col = 0; col < FONT_WIDTH; col++) {

        /* Get the column bitmap from font table */
        uint8_t col_data = font_5x7[uch][col];

        /* Render each row bit of this column */
        for (int row = 0; row < FONT_HEIGHT; row++) {

            /* Extract the pixel bit (LSB = top row) */
            uint8_t pixel_on = (col_data >> row) & 0x01;

            oled_set_pixel(x + col, y + row, pixel_on);
        }
    }
}

/*
 * oled_draw_text()
 * Draws a null-terminated string at position (x, y).
 * Characters advance left to right.
 * Does NOT wrap at screen edge — caller must manage.
 *
 * x    : starting pixel column
 * y    : starting pixel row
 * text : null-terminated ASCII string
 */
static void oled_draw_text(int x, int y, const char *text)
{
    if (text == NULL) return;

    while (*text != '\0') {
        oled_draw_char(x, y, *text);

        /* Advance x by character width + spacing */
        x += FONT_WIDTH + FONT_SPACING;

        /* Stop if next character would go off screen */
        if (x + FONT_WIDTH >= OLED_H_RES) break;

        text++;
    }
}

/* =======================================================
 * DRAWING UTILITIES
 * ======================================================= */

/*
 * oled_draw_border()
 * Draws a 1-pixel border around the entire display.
 */
static void oled_draw_border(void)
{
    /* Top and bottom rows */
    for (int x = 0; x < OLED_H_RES; x++) {
        oled_set_pixel(x, 0, 1);
        oled_set_pixel(x, OLED_V_RES - 1, 1);
    }

    /* Left and right columns */
    for (int y = 0; y < OLED_V_RES; y++) {
        oled_set_pixel(0, y, 1);
        oled_set_pixel(OLED_H_RES - 1, y, 1);
    }
}

/*
 * oled_draw_hline()
 * Draws a horizontal line at row y from x1 to x2.
 */
static void oled_draw_hline(int x1, int x2, int y)
{
    for (int x = x1; x <= x2; x++) {
        oled_set_pixel(x, y, 1);
    }
}

/* =======================================================
 * SCREEN COMPOSITIONS
 * ======================================================= */

/*
 * oled_show_status_screen()
 * Displays a startup status screen.
 * Shows firmware name, protocol, status, I2C address.
 *
 * Row layout (y values):
 *   y = 8  : "ANALOG DATA"
 *   y = 22 : "ESP-IDF I2C"
 *   y = 36 : "OLED OK"
 *   y = 50 : "ADDR 0X3C"
 */
static void oled_show_status_screen(void)
{
    oled_clear_buffer();
    oled_draw_border();

    /* Divider line under title */
    oled_draw_hline(2, OLED_H_RES - 3, 17);

    oled_draw_text(10,  8, "ANALOG DATA");
    oled_draw_text(10, 22, "ESP-IDF I2C");
    oled_draw_text(10, 36, "OLED OK");
    oled_draw_text(10, 50, "ADDR 0X3C");

    oled_flush();

    ESP_LOGI(TAG, "Status screen displayed");
}

/*
 * oled_show_counter_screen()
 * Displays a live counter value on the OLED.
 * Updates on every call.
 *
 * count : integer value to display
 *
 * Screen layout:
 *   Line 1 (y=8)  : "I2C OLED DEMO"
 *   Divider (y=18)
 *   Line 2 (y=24) : "COUNT: <value>"
 *   Line 3 (y=44) : "STATUS OK"
 */
static void oled_show_counter_screen(int count)
{
    /* Buffer for formatted counter string */
    char count_str[16];

    oled_clear_buffer();
    oled_draw_border();

    /* Title */
    oled_draw_text(4, 8, "I2C OLED DEMO");

    /* Divider under title */
    oled_draw_hline(2, OLED_H_RES - 3, 18);

    /* Counter label */
    oled_draw_text(4, 26, "COUNT:");

    /* Counter value — right of label
     * "COUNT:" = 6 chars x 6px = 36px
     * Start counter at x = 4 + 36 + 4 = 44 */
    snprintf(count_str, sizeof(count_str), "%d", count);
    oled_draw_text(44, 26, count_str);

    /* Divider above status */
    oled_draw_hline(2, OLED_H_RES - 3, 42);

    /* Status line */
    oled_draw_text(4, 50, "STATUS OK");

    oled_flush();

    ESP_LOGI(TAG, "Counter: %d", count);
}

/* =======================================================
 * APP MAIN
 * ======================================================= */

void app_main(void)
{
    ESP_LOGI(TAG, "=========================================");
    ESP_LOGI(TAG, "  SSD1306 OLED Text — Experiment 03");
    ESP_LOGI(TAG, "  Analog Data | analogdata.io");
    ESP_LOGI(TAG, "=========================================");

    /* Step 1 — Initialise I2C master bus */
    i2c_master_init();

    /* Step 2 — Initialise SSD1306 OLED panel */
    oled_init();

    /* Step 3 — Show startup status screen for 3 seconds */
    oled_show_status_screen();
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Step 4 — Count loop — updates OLED every second */
    int counter = 0;

    while (1) {
        oled_show_counter_screen(counter);
        counter++;

        /* Update every 1 second */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}