#include "board_cardputer.h"
#include "keyboard_matrix.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

static const char *TAG = "BOARD_CARDPUTER";

esp_err_t board_cardputer_init(void)
{
    ESP_LOGI(TAG, "Initializing M5Stack Cardputer 1.1 BSP...");

    // 1. Initialize Battery ADC (GPIO10 / ADC1 Channel 0)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);

    // 2. Spawn Key Scanning Task (Priority 5, 3KB stack)
    xTaskCreate(keyboard_scan_task, "kbd_scan", 3072, NULL, 5, NULL);

    // 3. TODO: Call display_st7789_init() once display driver is linked

    ESP_LOGI(TAG, "Cardputer 1.1 BSP Ready.");
    return ESP_OK;
}

float board_cardputer_get_battery_voltage(void)
{
    int raw = adc1_get_raw(ADC1_CHANNEL_0);
    // 1:1 voltage divider scaling + 3.3V VREF
    float mv = (raw / 4095.0f) * 3300.0f * 2.0f;
    return mv / 1000.0f;
}
