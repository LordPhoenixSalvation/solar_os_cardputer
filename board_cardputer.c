#include "board_cardputer.h"
#include "keyboard_matrix.h"
#include "display_st7789.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BOARD_CARDPUTER";

esp_err_t board_cardputer_init_sdcard(void)
{
    ESP_LOGI(TAG, "Mounting SD Card over SPI2_HOST...");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_12;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SD Card mounted successfully at /sdcard.");
    } else {
        ESP_LOGE(TAG, "Failed to mount SD Card (0x%x).", ret);
    }
    return ret;
}

esp_err_t board_cardputer_init(void)
{
    ESP_LOGI(TAG, "Initializing M5Stack Cardputer 1.1 BSP...");

    // 1. Initialize Battery ADC (GPIO10 / ADC1 CH0)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_12);
    ESP_LOGI(TAG, "Battery ADC initialized.");

    // 2. Initialize ST7789 Display Driver
    if (display_st7789_init() == ESP_OK) {
        ESP_LOGI(TAG, "Display pipeline ready.");
    }

    // 3. Mount MicroSD Card
    board_cardputer_init_sdcard();

    // 4. Spawn Keyboard Scanning Task
    xTaskCreate(keyboard_scan_task, "kbd_scan", 3072, NULL, 5, NULL);

    ESP_LOGI(TAG, "Cardputer 1.1 BSP Initialization Complete.");
    return ESP_OK;
}

float board_cardputer_get_battery_voltage(void)
{
    int raw = adc1_get_raw(ADC1_CHANNEL_0);
    float mv = (raw / 4095.0f) * 3300.0f * 2.0f;
    return mv / 1000.0f;
}
