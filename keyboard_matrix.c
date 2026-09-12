#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "CARD_KBD";

// 74HC138 Demux Pins (Address lines)
#define PIN_A0  GPIO_NUM_8
#define PIN_A1  GPIO_NUM_9
#define PIN_A2  GPIO_NUM_11

// Column Input Pins (7 columns)
static const gpio_num_t col_pins[7] = {
    GPIO_NUM_13, GPIO_NUM_15, GPIO_NUM_3, 
    GPIO_NUM_4,  GPIO_NUM_5,  GPIO_NUM_6, GPIO_NUM_7
};

// 56-Key Map Lookup (8 rows x 7 cols)
static const char key_map[8][7] = {
    /* Row 0 */ {'`', '1', '2', '3', '4', '5', '6'},
    /* Row 1 */ {'7', '8', '9', '0', '-', '=', '\b'},
    /* Row 2 */ {'\t', 'q', 'w', 'e', 'r', 't', 'y'},
    /* Row 3 */ {'u', 'i', 'o', 'p', '[', ']', '\\'},
    /* Row 4 */ {'\0', 'a', 's', 'd', 'f', 'g', 'h'}, // \0 = Fn / Special
    /* Row 5 */ {'j', 'k', 'l', ';', '\'', '\n', '\0'},
    /* Row 6 */ {'\0', 'z', 'x', 'c', 'v', 'b', 'n'}, // \0 = Shift
    /* Row 7 */ {'m', ',', '.', '/', ' ', '\0', '\0'}
};

static uint8_t prev_matrix[8] = {0}; // Bitfield state tracking for debouncing

void keyboard_init(void)
{
    // 1. Configure 74HC138 Address Pins as Output
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_A0) | (1ULL << PIN_A1) | (1ULL << PIN_A2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

    // 2. Configure Column Pins as Input with Internal Pull-Ups
    uint64_t col_mask = 0;
    for (int i = 0; i < 7; i++) {
        col_mask |= (1ULL << col_pins[i]);
    }

    gpio_config_t in_conf = {
        .pin_bit_mask = col_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);
}

// Set active row line on 74HC138 (Row 0 through 7)
static inline void select_row(uint8_t row)
{
    gpio_set_level(PIN_A0, (row >> 0) & 0x01);
    gpio_set_level(PIN_A1, (row >> 1) & 0x01);
    gpio_set_level(PIN_A2, (row >> 2) & 0x01);
}

// Matrix Scanning FreeRTOS Task
void keyboard_scan_task(void *pvParameters)
{
    keyboard_init();
    ESP_LOGI(TAG, "Keyboard matrix scanner started.");

    while (1) {
        for (uint8_t r = 0; r < 8; r++) {
            select_row(r);
            
            // Short stabilization delay for demux output settling
            esp_rom_delay_us(10); 

            uint8_t current_row_state = 0;

            for (uint8_t c = 0; c < 7; c++) {
                // Active-Low check: Pressed key pulls input pin LOW (0)
                if (gpio_get_level(col_pins[c]) == 0) {
                    current_row_state |= (1 << c);
                }
            }

            // Edge Detection: Check for newly pressed keys
            uint8_t newly_pressed = current_row_state & ~prev_matrix[r];
            if (newly_pressed) {
                for (uint8_t c = 0; c < 7; c++) {
                    if (newly_pressed & (1 << c)) {
                        char ch = key_map[r][c];
                        if (ch != '\0') {
                            ESP_LOGI(TAG, "Key Pressed: '%c' [Row %d, Col %d]", ch, r, c);
                            // TODO: Dispatch character to SolarOS event queue!
                        }
                    }
                }
            }

            prev_matrix[r] = current_row_state;
        }

        // Poll keyboard every 20ms (~50Hz scan rate)
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
