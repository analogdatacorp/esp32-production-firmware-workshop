/*
 * ============================================================
 * BMP280 Temperature & Pressure Sensor — Experiment 04
 * ESP32 Production Firmware Workshop
 * Analog Data | analogdata.io
 * ============================================================
 *
 * ABOUT THE BMP280 SENSOR
 * ============================================================
 * Manufacturer  : Bosch Sensortec
 * Type          : Digital pressure and temperature sensor
 * Protocol      : I2C (up to 3.4 MHz) or SPI (up to 10 MHz)
 * Supply voltage: 1.71V to 3.6V (3.3V on ESP32 boards)
 * Current draw  : 2.7 µA at 1 Hz sampling (ultra low power)
 * Package       : 2.0 x 2.5 x 0.95 mm LGA
 *
 * WHAT THE BMP280 MEASURES
 * ============================================================
 * 1. TEMPERATURE
 *    Range     : -40°C to +85°C
 *    Resolution: 0.01°C
 *    Accuracy  : ±1.0°C (typical at 25°C)
 *    Use case  : Room temperature, weather station, HVAC
 *
 * 2. BAROMETRIC PRESSURE
 *    Range     : 300 hPa to 1100 hPa
 *    Resolution: 0.16 Pa (0.0016 hPa)
 *    Accuracy  : ±1 hPa (typical)
 *    Use case  : Weather monitoring, altitude calculation
 *
 * 3. ALTITUDE (DERIVED — not directly measured)
 *    Calculated from pressure using the barometric formula
 *    Resolution: ~0.1 m at sea level
 *    Accuracy  : ±1 m (with known sea-level reference)
 *    Use case  : Drones, hiking trackers, floor detection
 *
 * WHAT THE BMP280 CANNOT MEASURE
 * ============================================================
 *    Humidity  : Use BME280 instead (pin-compatible, adds humidity)
 *    Gas       : Use BME680 instead (adds VOC gas sensing)
 *    UV Index  : Different sensor required
 *
 * BMP280 OPERATING MODES
 * ============================================================
 * Sleep mode   : No measurements, lowest power (0.1 µA)
 * Forced mode  : One measurement then back to sleep (power efficient)
 * Normal mode  : Continuous measurements at set interval (this demo)
 *
 * OVERSAMPLING SETTINGS
 * ============================================================
 * Higher oversampling = lower noise = more power + slower
 *
 * osrs_t / osrs_p values:
 *   000 = skipped (output 0x80000)
 *   001 = x1  (ultra low power)
 *   010 = x2
 *   011 = x4
 *   100 = x8
 *   101 = x16 (ultra high resolution)
 *
 * This demo uses:
 *   Temperature oversampling : x1 (osrs_t = 001)
 *   Pressure oversampling    : x4 (osrs_p = 011)
 *   Mode                     : Normal (mode = 11)
 *   ctrl_meas register       : 0x27 = 00 101 11 = x1 temp, x4 pres, normal
 *
 * CALIBRATION DATA
 * ============================================================
 * Every BMP280 chip is individually calibrated at the factory.
 * 24 bytes of calibration coefficients are stored in the chip's
 * non-volatile memory (registers 0x88 to 0x9F).
 *
 * These coefficients MUST be read and applied to every raw
 * measurement using Bosch's compensation formula.
 * Raw ADC values without compensation are meaningless.
 *
 * HARDWARE CONNECTIONS
 * ============================================================
 * BMP280 Pin   ESP32 Pin      Notes
 * ---------    ----------     --------------------------------
 * VCC          3.3V           DO NOT connect to 5V
 * GND          GND            Common ground
 * SDA          GPIO 8         I2C data line
 * SCL          GPIO 9         I2C clock line
 * SDO/ADDR     GND            Sets I2C address to 0x76
 *                             Connect to 3.3V for address 0x77
 * CSB          3.3V           Selects I2C mode (not SPI)
 *
 * I2C ADDRESSES
 * ============================================================
 * SDO pin to GND   -> address 0x76 (default)
 * SDO pin to VCC   -> address 0x77
 * Two BMP280s can share one I2C bus using different addresses.
 *
 * REGISTER MAP (key registers used in this code)
 * ============================================================
 * 0x88-0x9F : Calibration data (read once at startup)
 * 0xD0      : chip_id register (should read 0x58 for BMP280)
 * 0xE0      : reset register (write 0xB6 to soft reset)
 * 0xF3      : status register (bit 3 = measuring, bit 0 = updating)
 * 0xF4      : ctrl_meas (oversampling + mode control)
 * 0xF5      : config (standby time + filter + SPI 3-wire)
 * 0xF7-0xF9 : pressure ADC output (20-bit, MSB first)
 * 0xFA-0xFC : temperature ADC output (20-bit, MSB first)
 *
 * REAL WORLD USE CASES
 * ============================================================
 * 1. Weather station   : Temperature + pressure trend analysis
 * 2. Indoor navigation : Floor detection using pressure delta
 * 3. Drone altitude    : Barometric altitude hold
 * 4. HVAC control      : Room temperature monitoring
 * 5. Wearables         : Activity detection from pressure changes
 * 6. Smart home        : Environmental monitoring dashboard
 *
 * ALTITUDE FORMULA
 * ============================================================
 * altitude (m) = 44330 * (1 - (P / P0) ^ (1/5.255))
 * Where P  = measured pressure in hPa
 *       P0 = sea level pressure (standard = 1013.25 hPa)
 *
 * CHIP IDENTIFICATION
 * ============================================================
 * BMP280 chip_id = 0x58
 * BME280 chip_id = 0x60  (same pinout, adds humidity)
 * BMP180 chip_id = 0x55  (older, different register map)
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "esp_log.h"
#include "esp_err.h"

/* ============================================================
 * LOG TAG
 * ============================================================ */
static const char *TAG = "bmp280";

/* ============================================================
 * I2C CONFIGURATION
 *
 * NOTE: GPIO 8 and 9 are used here.
 * On standard ESP32 DevKit — GPIO 8 and 9 are connected to
 * the internal SPI flash on some boards and may cause issues.
 * If you see random crashes — switch to GPIO 21 (SDA) and
 * GPIO 22 (SCL) which are always safe on all ESP32 boards.
 * ============================================================ */
#define I2C_MASTER_PORT        I2C_NUM_0
#define I2C_MASTER_SDA_IO      GPIO_NUM_8
#define I2C_MASTER_SCL_IO      GPIO_NUM_9
#define I2C_MASTER_FREQ_HZ     400000         /* 400 kHz Fast Mode */
#define I2C_TIMEOUT_MS         1000           /* 1 second timeout */

/* ============================================================
 * BMP280 I2C ADDRESS
 *
 * 0x76 when SDO pin is connected to GND (default)
 * 0x77 when SDO pin is connected to VCC
 * ============================================================ */
#define BMP280_I2C_ADDR        0x76

/* ============================================================
 * BMP280 REGISTER ADDRESSES
 * From Bosch BMP280 Datasheet Section 4.2
 * ============================================================ */
#define BMP280_REG_CHIP_ID     0xD0    /* Always reads 0x58 */
#define BMP280_REG_RESET       0xE0    /* Write 0xB6 to soft reset */
#define BMP280_REG_STATUS      0xF3    /* Measuring and NVM update flags */
#define BMP280_REG_CTRL_MEAS   0xF4    /* Oversampling and mode */
#define BMP280_REG_CONFIG      0xF5    /* Standby time and IIR filter */
#define BMP280_REG_PRESS_MSB   0xF7    /* Pressure MSB — start of data */
#define BMP280_REG_TEMP_MSB    0xFA    /* Temperature MSB */
#define BMP280_REG_CALIB       0x88    /* Start of calibration data */

/* ============================================================
 * BMP280 EXPECTED CHIP ID
 *
 * If the chip_id register does not return this value:
 *   - Wiring may be wrong
 *   - Wrong I2C address
 *   - Sensor may be damaged
 *   - It may be a BME280 (returns 0x60) — different sensor
 * ============================================================ */
#define BMP280_CHIP_ID         0x58

/* ============================================================
 * BMP280 CTRL_MEAS REGISTER VALUE
 *
 * Bits [7:5] osrs_t — temperature oversampling
 * Bits [4:2] osrs_p — pressure oversampling
 * Bits [1:0] mode   — power mode
 *
 * 0x27 = 0b 001 001 11
 *          osrs_t=001 (x1) osrs_p=001 (x1) mode=11 (normal)
 *
 * 0xB7 = 0b 101 101 11
 *          osrs_t=101 (x16) osrs_p=101 (x16) mode=11 (normal)
 *          Higher accuracy but more power and slower
 * ============================================================ */
#define BMP280_CTRL_MEAS_VAL   0x27

/* ============================================================
 * BMP280 CONFIG REGISTER VALUE
 *
 * Bits [7:5] t_sb    — standby time in normal mode
 * Bits [4:2] filter  — IIR filter coefficient
 * Bit  [0]   spi3w_en — SPI 3-wire enable (0 = disabled)
 *
 * 0xA0 = 0b 101 000 0 0
 *          t_sb=101 (1000ms) filter=000 (off) spi3w=0
 * ============================================================ */
#define BMP280_CONFIG_VAL      0xA0

/* ============================================================
 * SEA LEVEL PRESSURE
 *
 * Standard sea level pressure = 1013.25 hPa
 * For accurate altitude — use actual local sea level pressure
 * from a weather service for your location.
 * ============================================================ */
#define SEA_LEVEL_PRESSURE_HPA 1013.25f

/* ============================================================
 * DRIVER HANDLES
 * ============================================================ */
static i2c_master_bus_handle_t i2c_bus_handle   = NULL;
static i2c_master_dev_handle_t bmp280_dev_handle = NULL;

/* ============================================================
 * BMP280 CALIBRATION DATA STRUCTURE
 *
 * These coefficients are unique to each BMP280 chip.
 * Read from registers 0x88-0x9D at startup.
 * Applied to raw ADC values using Bosch compensation formula.
 *
 * T1 is unsigned (uint16_t)
 * T2 and T3 are signed (int16_t)
 * ============================================================ */
typedef struct {
    uint16_t dig_T1;    /* Unsigned — always positive */
    int16_t  dig_T2;    /* Signed — can be negative */
    int16_t  dig_T3;    /* Signed — can be negative */
    uint16_t dig_P1;    /* Pressure calibration */
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_t;

static bmp280_calib_t bmp280_calib;

/*
 * t_fine is a global intermediate value produced by the
 * temperature compensation formula and reused by the
 * pressure compensation formula.
 * This is exactly how Bosch specifies it in the datasheet.
 */
static int32_t t_fine;

/* ============================================================
 * READING HELPERS
 *
 * BMP280 stores 16-bit values in little-endian format:
 *   byte[0] = LSB (low byte)
 *   byte[1] = MSB (high byte)
 *
 * read_u16_le : read unsigned 16-bit little-endian
 * read_s16_le : read signed 16-bit little-endian
 * ============================================================ */
static uint16_t read_u16_le(const uint8_t *buf)
{
    return (uint16_t)(buf[0]) | ((uint16_t)(buf[1]) << 8);
}

static int16_t read_s16_le(const uint8_t *buf)
{
    return (int16_t)read_u16_le(buf);
}

/* ============================================================
 * I2C INITIALISATION
 * ============================================================ */
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

/* ============================================================
 * BMP280 DEVICE REGISTRATION
 *
 * In the new ESP-IDF v6 I2C API, each device on the bus
 * must be explicitly registered with its I2C address.
 * This creates a device handle used for all communication.
 * ============================================================ */
static void bmp280_add_device(void)
{
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,  /* 7-bit address */
        .device_address  = BMP280_I2C_ADDR,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(
        i2c_bus_handle,
        &dev_config,
        &bmp280_dev_handle
    ));

    ESP_LOGI(TAG, "BMP280 registered at I2C address 0x%02X",
             BMP280_I2C_ADDR);
}

/* ============================================================
 * LOW LEVEL REGISTER ACCESS
 * ============================================================ */

/*
 * bmp280_write_reg()
 * Writes one byte to a BMP280 register.
 *
 * I2C write transaction:
 *   [START] [ADDR+W] [REG_ADDR] [VALUE] [STOP]
 */
static esp_err_t bmp280_write_reg(uint8_t reg_addr, uint8_t value)
{
    uint8_t write_buf[2] = { reg_addr, value };

    return i2c_master_transmit(
        bmp280_dev_handle,
        write_buf,
        sizeof(write_buf),
        I2C_TIMEOUT_MS
    );
}

/*
 * bmp280_read_regs()
 * Reads one or more bytes starting from reg_addr.
 *
 * I2C write-then-read transaction:
 *   [START] [ADDR+W] [REG_ADDR] [REPEATED START] [ADDR+R] [DATA...] [STOP]
 *
 * This is the standard way to read from I2C sensor registers.
 * The BMP280 auto-increments the register address for multi-byte reads.
 */
static esp_err_t bmp280_read_regs(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(
        bmp280_dev_handle,
        &reg_addr, 1,       /* Write: register address */
        data, len,          /* Read: data bytes */
        I2C_TIMEOUT_MS
    );
}

/* ============================================================
 * BMP280 CHIP ID VERIFICATION
 *
 * Always verify chip ID at startup.
 * If this returns wrong value — stop and fix hardware before
 * proceeding. Using wrong calibration data will give garbage
 * temperature readings.
 * ============================================================ */
static void bmp280_check_chip_id(void)
{
    uint8_t chip_id = 0;

    ESP_ERROR_CHECK(bmp280_read_regs(BMP280_REG_CHIP_ID, &chip_id, 1));

    ESP_LOGI(TAG, "BMP280 Chip ID: 0x%02X (expected 0x%02X)",
             chip_id, BMP280_CHIP_ID);

    if (chip_id == 0x60) {
        ESP_LOGW(TAG, "This is a BME280 (humidity sensor variant)");
        ESP_LOGW(TAG, "Code will still work for temperature and pressure");
    } else if (chip_id != BMP280_CHIP_ID) {
        ESP_LOGE(TAG, "Wrong chip ID! Check wiring and I2C address.");
        ESP_LOGE(TAG, "Expected 0x58, got 0x%02X", chip_id);
        /* In production — abort here. For demo — continue with warning. */
    } else {
        ESP_LOGI(TAG, "BMP280 confirmed OK");
    }
}

/* ============================================================
 * CALIBRATION DATA READ
 *
 * Bosch stores 24 bytes of calibration data starting at 0x88.
 * Layout:
 *   0x88-0x89 : dig_T1 (uint16, little-endian)
 *   0x8A-0x8B : dig_T2 (int16,  little-endian)
 *   0x8C-0x8D : dig_T3 (int16,  little-endian)
 *   0x8E-0x8F : dig_P1 (uint16, little-endian)
 *   0x90-0x91 : dig_P2 (int16,  little-endian)
 *   0x92-0x93 : dig_P3 (int16,  little-endian)
 *   0x94-0x95 : dig_P4 (int16,  little-endian)
 *   0x96-0x97 : dig_P5 (int16,  little-endian)
 *   0x98-0x99 : dig_P6 (int16,  little-endian)
 *   0x9A-0x9B : dig_P7 (int16,  little-endian)
 *   0x9C-0x9D : dig_P8 (int16,  little-endian)
 *   0x9E-0x9F : dig_P9 (int16,  little-endian)
 * ============================================================ */
static void bmp280_read_calibration_data(void)
{
    /* Read all 24 calibration bytes in one I2C transaction */
    uint8_t calib_data[24];

    ESP_ERROR_CHECK(bmp280_read_regs(
        BMP280_REG_CALIB,
        calib_data,
        sizeof(calib_data)
    ));

    /* Parse temperature calibration — bytes 0-5 */
    bmp280_calib.dig_T1 = read_u16_le(&calib_data[0]);
    bmp280_calib.dig_T2 = read_s16_le(&calib_data[2]);
    bmp280_calib.dig_T3 = read_s16_le(&calib_data[4]);

    /* Parse pressure calibration — bytes 6-23 */
    bmp280_calib.dig_P1 = read_u16_le(&calib_data[6]);
    bmp280_calib.dig_P2 = read_s16_le(&calib_data[8]);
    bmp280_calib.dig_P3 = read_s16_le(&calib_data[10]);
    bmp280_calib.dig_P4 = read_s16_le(&calib_data[12]);
    bmp280_calib.dig_P5 = read_s16_le(&calib_data[14]);
    bmp280_calib.dig_P6 = read_s16_le(&calib_data[16]);
    bmp280_calib.dig_P7 = read_s16_le(&calib_data[18]);
    bmp280_calib.dig_P8 = read_s16_le(&calib_data[20]);
    bmp280_calib.dig_P9 = read_s16_le(&calib_data[22]);

    ESP_LOGI(TAG, "Calibration loaded:");
    ESP_LOGI(TAG, "  dig_T1 = %u", bmp280_calib.dig_T1);
    ESP_LOGI(TAG, "  dig_T2 = %d", bmp280_calib.dig_T2);
    ESP_LOGI(TAG, "  dig_T3 = %d", bmp280_calib.dig_T3);
    ESP_LOGI(TAG, "  dig_P1 = %u", bmp280_calib.dig_P1);
    ESP_LOGI(TAG, "  dig_P2 = %d", bmp280_calib.dig_P2);
}

/* ============================================================
 * SENSOR CONFIGURATION
 *
 * Step 1: Soft reset — ensures clean start
 * Step 2: Set ctrl_meas — oversampling and operating mode
 * Step 3: Set config — standby time and IIR filter
 *
 * ctrl_meas = 0x27:
 *   osrs_t = 001 (temperature x1 oversampling)
 *   osrs_p = 001 (pressure x1 oversampling)
 *   mode   = 11  (normal mode — continuous measurement)
 *
 * config = 0xA0:
 *   t_sb   = 101 (1000ms standby between measurements)
 *   filter = 000 (IIR filter off)
 * ============================================================ */
static void bmp280_configure(void)
{
    /* Soft reset — equivalent to power cycling the sensor */
    ESP_ERROR_CHECK(bmp280_write_reg(BMP280_REG_RESET, 0xB6));

    /* Wait for reset to complete and NVM to load */
    vTaskDelay(pdMS_TO_TICKS(100));

    /* Configure oversampling and set normal mode */
    ESP_ERROR_CHECK(bmp280_write_reg(BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_VAL));

    /* Configure standby time and IIR filter */
    ESP_ERROR_CHECK(bmp280_write_reg(BMP280_REG_CONFIG, BMP280_CONFIG_VAL));

    ESP_LOGI(TAG, "BMP280 configured — normal mode, 1s standby");
}

/* ============================================================
 * RAW DATA READING
 *
 * The BMP280 stores temperature in 3 bytes at 0xFA-0xFC.
 * Data format: 20-bit value, MSB first
 *
 *   0xFA : temp_msb  [7:0] — bits 19:12
 *   0xFB : temp_lsb  [7:0] — bits 11:4
 *   0xFC : temp_xlsb [7:4] — bits 3:0 (lower 4 bits unused)
 *
 * Assembly: adc_T = msb << 12 | lsb << 4 | xlsb >> 4
 *
 * Similarly for pressure at 0xF7-0xF9.
 * Reading all 6 bytes in one transaction (0xF7 to 0xFC)
 * ensures pressure and temperature are from the same sample.
 * ============================================================ */
static void bmp280_read_raw(int32_t *raw_temp, int32_t *raw_press)
{
    /* Read 6 bytes: 3 pressure + 3 temperature in one transaction */
    uint8_t data[6];

    ESP_ERROR_CHECK(bmp280_read_regs(BMP280_REG_PRESS_MSB, data, sizeof(data)));

    /* Pressure raw: bytes 0, 1, 2 */
    *raw_press = ((int32_t)data[0] << 12) |
                 ((int32_t)data[1] << 4)  |
                 ((int32_t)data[2] >> 4);

    /* Temperature raw: bytes 3, 4, 5 */
    *raw_temp  = ((int32_t)data[3] << 12) |
                 ((int32_t)data[4] << 4)  |
                 ((int32_t)data[5] >> 4);
}

/* ============================================================
 * TEMPERATURE COMPENSATION
 *
 * This is the exact formula from Bosch BMP280 Datasheet
 * Section 4.2.3 — "Compensation formulas in double precision
 * floating point" (integer version used here for ESP32).
 *
 * Input  : adc_T — raw 20-bit ADC temperature value
 * Output : temperature in degrees Celsius (float)
 *
 * Side effect: sets t_fine global variable which is reused
 * by the pressure compensation formula.
 *
 * The math looks complex but it is just a 2nd order polynomial
 * correction using the 3 factory calibration coefficients.
 * ============================================================ */
static float bmp280_compensate_temperature(int32_t adc_T)
{
    int32_t var1;
    int32_t var2;

    /* First order correction using dig_T1 and dig_T2 */
    var1 = ((((adc_T >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) *
            ((int32_t)bmp280_calib.dig_T2)) >> 11;

    /* Second order correction using dig_T1 and dig_T3 */
    var2 = (((((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) *
            ((int32_t)bmp280_calib.dig_T3)) >> 14;

    /* t_fine is used by pressure compensation — must be set here */
    t_fine = var1 + var2;

    /* Convert to °C — result is in 1/100 degrees */
    int32_t temperature_centi = (t_fine * 5 + 128) >> 8;

    return (float)temperature_centi / 100.0f;
}

/* ============================================================
 * PRESSURE COMPENSATION
 *
 * Also from Bosch BMP280 Datasheet Section 4.2.3.
 * Must be called AFTER compensate_temperature because it
 * depends on t_fine being set correctly.
 *
 * Input  : adc_P — raw 20-bit ADC pressure value
 * Output : pressure in Pascals (Pa)
 *          Divide by 100 to get hPa (hectopascals / millibars)
 * ============================================================ */
static float bmp280_compensate_pressure(int32_t adc_P)
{
    int64_t var1;
    int64_t var2;
    int64_t p;

    /* int64 required to avoid overflow in intermediate values */
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) +
           ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) *
           ((int64_t)bmp280_calib.dig_P1) >> 33;

    /* Avoid division by zero */
    if (var1 == 0) {
        return 0.0f;
    }

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)bmp280_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)bmp280_calib.dig_P7) << 4);

    /* Result is in Q24.8 format — divide by 256 to get Pascals */
    return (float)p / 256.0f;
}

/* ============================================================
 * ALTITUDE CALCULATION
 *
 * Derived from pressure using the International Barometric Formula.
 * This formula assumes standard atmosphere conditions.
 *
 * For accurate results — update SEA_LEVEL_PRESSURE_HPA with
 * the actual sea level pressure from a nearby weather station.
 *
 * Input  : pressure_hpa — measured pressure in hPa
 * Output : altitude in metres above sea level
 * ============================================================ */
static float bmp280_calculate_altitude(float pressure_hpa)
{
    return 44330.0f * (1.0f - powf(
        pressure_hpa / SEA_LEVEL_PRESSURE_HPA,
        1.0f / 5.255f
    ));
}

/* ============================================================
 * BMP280 FULL INITIALISATION SEQUENCE
 * ============================================================ */
static void bmp280_init(void)
{
    bmp280_add_device();
    bmp280_check_chip_id();
    bmp280_read_calibration_data();
    bmp280_configure();

    ESP_LOGI(TAG, "BMP280 ready");
}

/* ============================================================
 * APP MAIN
 * ============================================================ */
void app_main(void)
{
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "  BMP280 Temperature and Pressure — Exp 04");
    ESP_LOGI(TAG, "  Analog Data | analogdata.io");
    ESP_LOGI(TAG, "================================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "BMP280 Sensor Capabilities:");
    ESP_LOGI(TAG, "  Temperature : -40 to +85 C (accuracy +-1.0 C)");
    ESP_LOGI(TAG, "  Pressure    : 300 to 1100 hPa (accuracy +-1 hPa)");
    ESP_LOGI(TAG, "  Altitude    : derived from pressure");
    ESP_LOGI(TAG, "  Humidity    : NOT available (use BME280)");
    ESP_LOGI(TAG, "");

    /* Step 1 — Initialise I2C bus */
    i2c_master_init();

    /* Step 2 — Initialise BMP280 sensor */
    bmp280_init();

    /* Step 3 — Read and display sensor data in a loop */
    int32_t raw_temp  = 0;
    int32_t raw_press = 0;
    uint32_t reading  = 0;

    while (1) {
        reading++;

        /* Read raw ADC values from sensor */
        bmp280_read_raw(&raw_temp, &raw_press);

        /* Apply Bosch compensation formulas
         * IMPORTANT: compensate temperature FIRST —
         * pressure compensation depends on t_fine */
        float temperature = bmp280_compensate_temperature(raw_temp);
        float pressure_pa = bmp280_compensate_pressure(raw_press);
        float pressure_hpa = pressure_pa / 100.0f;
        float altitude    = bmp280_calculate_altitude(pressure_hpa);

        /* Log all values */
        ESP_LOGI(TAG, "--- Reading #%lu ---", (unsigned long)reading);
        ESP_LOGI(TAG, "  Temperature : %.2f C", temperature);
        ESP_LOGI(TAG, "  Pressure    : %.2f hPa", pressure_hpa);
        ESP_LOGI(TAG, "  Altitude    : %.1f m (approx)", altitude);
        ESP_LOGI(TAG, "  Raw temp    : %ld (ADC counts)", (long)raw_temp);
        ESP_LOGI(TAG, "  Raw press   : %ld (ADC counts)", (long)raw_press);

        /* Read every 2 seconds */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}