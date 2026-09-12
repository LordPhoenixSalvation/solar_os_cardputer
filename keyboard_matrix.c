#include "keyboard_matrix.h"
#include <stdio.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

// SolarOS Event System Includes
#include "solar_events.h"

static const char *TAG = "CARD_KBD";

#define PIN_A0  GPIO_NUM_8
#define PIN_A1  GPIO_NUM_9
#define PIN_A2  GPIO_NUM_11

static const gpio_num_t col_pins[7] = {
    GPIO_NUM_13, GPIO_NUM_15, GPIO_NUM_3, 
    GPIO_NUM_4,  GPIO_NUM_5,  GPIO_NUM_6, GPIO_NUM_7
};

// Base Keymap (Lower / Unshifted)
static const char key_map_base[8][7] = {
    {'`', '1', '2', '3', '4', '5', '6'},
    {'7', '8', '9', '0', '-', '=', '\b'},
    {'\t', 'q', 'w', 'e', 'r', 't', 'y'},
    {'u', 'i', 'o', 'p', '[', ']', '\\'},
    {'\0', 'a', 's', 'd', 'f', 'g', 'h'}, // \0 at [4][0] = Fn Key
    {'j', 'k', 'l', ';', '\'', '\n', '\0'},
    {'\0', 'z', 'x', 'c', 'v', 'b', 'n'}, // \0 at [6][0] = Shift Key
    {'m', ',', '.', '/', ' ', '\0', '\0'}
};

// Shifted Keymap
static const char key_map_shift[8][7] = {
    {'~', '!', '@', '#', '$', '%', '^'},
    {'&', '*', '(', ')', '_', '+', '\b'},
    {'\t', 'Q', 'W', 'E', 'R', 'T', 'Y'},
    {'U', 'I', 'O', 'P', '{', '}', '|'},
    {'\0', 'A', 'S', 'D', 'F', 'G', 'H'},
    {'J', 'K', 'L', ':', '"', '\n', '\0'},
    {'\0', 'Z', 'X', 'C', 'V', 'B', 'N'},
    {'M', '<', '>', '?', ' ', '\0', '\0'}
};

static uint8_t prev_matrix[8] = {0};

void keyboard_init(void)
{
    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << PIN_A0) | (1ULL << PIN_A1) | (1ULL << PIN_A2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);

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

static inline void select_row(uint8_t row)
{
    gpio_set_level(PIN_A0, (row >> 0) & 0x01);
    gpio_set_level(PIN_A1, (row >> 1) & 0x01);
    gpio_set_level(PIN_A2, (row >> 2) & 0x01);
}

void keyboard_scan_task(void *pvParameters)
{
    keyboard_init();
    ESP_LOGI(TAG, "Keyboard matrix scanner active.");

    while (1) {
        uint8_t current_matrix[8] = {0};

        // 1. First Pass: Read full key matrix state
        for (uint8_t r = 0; r < 8; r++) {
            select_row(r);
            esp_rom_delay_us(10); 

            for (uint8_t c = 0; c < 7; c++) {
                if (gpio_get_level(col_pins[c]) == 0) {
                    current_matrix[r] |= (1 << c);
                }
            }
        }

        // 2. Evaluate Modifiers: Shift = [6][0], Fn = [4][0]
        bool shift_active = (current_matrix[6] & (1 << 0)) != 0;
        bool fn_active    = (current_matrix[4] & (1 << 0)) != 0;

        uint8_t mod_flags = 0;
        if (shift_active) mod_flags |= SOLAR_MOD_SHIFT;
        if (fn_active)    mod_flags |= SOLAR_MOD_FN;

        // 3. Second Pass: Edge Detection and Event Dispatch
        for (uint8_t r = 0; r < 8; r++) {
            uint8_t newly_pressed = current_matrix[r] & ~prev_matrix[r];

            if (newly_pressed) {
                for (uint8_t c = 0; c < 7; c++) {
                    if (newly_pressed & (1 << c)) {
                        // Ignore pure modifier pin presses to avoid emitting empty characters
                        if ((r == 6 && c == 0) || (r == 4 && c == 0)) {
                            continue;
                        }

                        char ch = shift_active ? key_map_shift[r][c] : key_map_base[r][c];

                        if (ch != '\0') {
                            solar_event_t evt = {
                                .type = SOLAR_EVENT_KEYPRESS,
                                .data.key.ch = ch,
                                .data.key.modifiers = mod_flags
                            };
                            solar_event_post(&evt, pdMS_TO_TICKS(10));
                        }
                    }
                }
            }
            prev_matrix[r] = current_matrix[r];
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
