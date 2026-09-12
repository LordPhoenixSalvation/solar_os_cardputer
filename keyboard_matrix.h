#ifndef KEYBOARD_MATRIX_H
#define KEYBOARD_MATRIX_H

#include "esp_err.h"

/**
 * @brief Initialize GPIOs and 74HC138 demux lines for the 56-key matrix.
 */
void keyboard_init(void);

/**
 * @brief FreeRTOS task that continuously scans the key matrix, tracks modifier state,
 *        and posts key events to the SolarOS event queue.
 */
void keyboard_scan_task(void *pvParameters);

#endif // KEYBOARD_MATRIX_H
