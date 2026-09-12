#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ST7789";

#define PIN_MOSI 21
#define PIN_SCLK 40
#define PIN_CS   37
#define PIN_DC   35
#define PIN_RST  33
#define PIN_BK   38

static spi_device_handle_t spi_dev;

static void st7789_send_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_DC, 0); // Command mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_dev, &t);
}

static void st7789_send_data(const uint8_t *data, int len)
{
    if (len == 0) return;
    gpio_set_level(PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_dev, &t);
}

esp_err_t display_st7789_init(void)
{
    // Configure DC, RST, and Backlight GPIOs
    gpio_set_direction(PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BK, GPIO_MODE_OUTPUT);

    // Turn Backlight ON
    gpio_set_level(PIN_BK, 1);

    // Hardware Reset
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    // SPI Bus Config
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 135 * 2
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000, // 40MHz
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 7,
    };

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev);

    // Init Commands (Software Reset, Exit Sleep, Set Color Format to RGB565)
    st7789_send_cmd(0x01); // SWRESET
    vTaskDelay(pdMS_TO_TICKS(150));
    st7789_send_cmd(0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(120));
    
    uint8_t pixfmt = 0x55; // 16-bit RGB565
    st7789_send_cmd(0x3A);
    st7789_send_data(&pixfmt, 1);

    st7789_send_cmd(0x29); // DISPON
    ESP_LOGI(TAG, "ST7789 initialized at 240x135.");
    return ESP_OK;
}
