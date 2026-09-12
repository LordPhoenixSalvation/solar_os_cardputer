#ifndef DISPLAY_ST7789_H
#define DISPLAY_ST7789_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize the SPI bus and ST7789 display controller for 240x135 resolution.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t display_st7789_init(void);

/**
 * @brief Flush a windowed pixel region to the ST7789 display RAM via SPI DMA.
 * 
 * @param x1 Starting X coordinate (0-239)
 * @param y1 Starting Y coordinate (0-134)
 * @param x2 Ending X coordinate (0-239)
 * @param y2 Ending Y coordinate (0-134)
 * @param buffer Pointer to RGB565 frame buffer data
 */
void display_st7789_flush_region(int x1, int y1, int x2, int y2, const uint16_t *buffer);

/**
 * @brief Flush a full 240x135 frame buffer to the display.
 * 
 * @param buffer Pointer to RGB565 frame buffer data (240x135 pixels)
 */
void display_st7789_flush(const uint16_t *buffer);

#endif // DISPLAY_ST7789_H
