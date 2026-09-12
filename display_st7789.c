#include "display_st7789.h"
#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ST7789";

#define PIN_MOSI 21
#define PIN_SCLK 40
#define PIN_CS   37
#define PIN_DC   35
#define PIN_RST  33
#define PIN_BK   38

#define ST7789_OFFSET_X 40
#define ST7789_OFFSET_Y 53

static spi_device_handle_t spi_dev;

static void st7789_send_cmd(uint8_t cmd)
{
    gpio_set_level(PIN_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_polling_transmit(spi_dev, &t);
}

static void st7789_send_data(const uint8_t *data, int len)
{
    if (len == 0) return;
    gpio_set_level(PIN_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(spi_dev, &t);
}

esp_err_t display_st7789_init(void)
{
    gpio_set_direction(PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_BK, GPIO_MODE_OUTPUT);

    gpio_set_level(PIN_BK, 1);

    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 135 * 2
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 7,
    };

    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_dev);

    st7789_send_cmd(0x01); // SWRESET
    vTaskDelay(pdMS_TO_TICKS(150));
    st7789_send_cmd(0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(120));
    
    uint8_t pixfmt = 0x55; // RGB565 (16-bit)
    st7789_send_cmd(0x3A);
    st7789_send_data(&pixfmt, 1);

    uint8_t madctl = 0x70; // Landscape orientation
    st7789_send_cmd(0x36);
    st7789_send_data(&madctl, 1);

    st7789_send_cmd(0x29); // DISPON
    ESP_LOGI(TAG, "ST7789 initialized at 240x135.");
    return ESP_OK;
}

void display_st7789_flush_region(int x1, int y1, int x2, int y2, const uint16_t *buffer)
{
    uint16_t adj_x1 = x1 + ST7789_OFFSET_X;
    uint16_t adj_x2 = x2 + ST7789_OFFSET_X;
    uint16_t adj_y1 = y1 + ST7789_OFFSET_Y;
    uint16_t adj_y2 = y2 + ST7789_OFFSET_Y;

    // CASET (Column Address Set)
    st7789_send_cmd(0x2A);
    uint8_t col_data[] = { adj_x1 >> 8, adj_x1 & 0xFF, adj_x2 >> 8, adj_x2 & 0xFF };
    st7789_send_data(col_data, 4);

    // RASET (Row Address Set)
    st7789_send_cmd(0x2B);
    uint8_t row_data[] = { adj_y1 >> 8, adj_y1 & 0xFF, adj_y2 >> 8, adj_y2 & 0xFF };
    st7789_send_data(row_data, 4);

    // RAMWR (Memory Write)
    st7789_send_cmd(0x2C);
    int pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
    st7789_send_data((const uint8_t *)buffer, pixels * 2);
}

void display_st7789_flush(const uint16_t *buffer)
{
    display_st7789_flush_region(0, 0, 239, 134, buffer);
}
