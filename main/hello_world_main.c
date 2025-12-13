/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// #include <driver/gpio.h>
#include <esp_log.h>


#include "driver/gpio.h"

#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"


/* SDMMC 外设相关硬件引脚 */
#define SDMMC_PIN_CMD GPIO_NUM_44
#define SDMMC_PIN_CLK GPIO_NUM_43
#define SDMMC_PIN_D0 GPIO_NUM_39
#define SDMMC_PIN_D1 GPIO_NUM_40
#define SDMMC_PIN_D2 GPIO_NUM_41
#define SDMMC_PIN_D3 GPIO_NUM_42
/* 挂载名称 */
#define MOUNT_POINT "/0:"




void app_main(void)
{

    esp_err_t ret;

    gpio_config_t io_config = {0};
    io_config.pin_bit_mask = 1ULL << GPIO_NUM_51; // Set GPIO51 as output
    io_config.mode = GPIO_MODE_OUTPUT; // Set as output mode
    io_config.pull_up_en = GPIO_PULLUP_ENABLE; // Enable pull-up resistor
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down resistor
    io_config.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    
    ret = gpio_config(&io_config);
    if(ret != ESP_OK) {
        ESP_LOGE("app_main", "GPIO configuration failed: %s", esp_err_to_name(ret));
        return;
    }

    sdmmc_host_init();
    esp_vfs_fat_mount_config_t mount_config = {0};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;
    mount_config.disk_status_check_enable = false;
    mount_config.use_one_fat = false;

    sdmmc_host_t sdmmc_host_config = SDMMC_HOST_DEFAULT();
    sdmmc_host_config.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t sdmmc_slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    sdmmc_slot_config.width = 4;
    sdmmc_slot_config.clk = SDMMC_PIN_CLK;
    sdmmc_slot_config.cmd = SDMMC_PIN_CMD;
    sdmmc_slot_config.d0 = SDMMC_PIN_D0;
    sdmmc_slot_config.d1 = SDMMC_PIN_D1;
    sdmmc_slot_config.d2 = SDMMC_PIN_D2;
    sdmmc_slot_config.d3 = SDMMC_PIN_D3;
    sdmmc_slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    sdmmc_card_t* card;

    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &sdmmc_host_config, &sdmmc_slot_config, &mount_config, &card);

    uint32_t size = 0;
    size = ((uint64_t)card->csd.capacity) * card->csd.sector_size/(1024 * 1024); 

    if (ret != ESP_OK) {
        ESP_LOGE("app_main", "Failed to mount SD card filesystem: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI("app_main", "SD card mounted successfully.");
    ESP_LOGI("app_main", "Card info:");
    ESP_LOGI("app_main", "Name: %s", card->cid.name);
    ESP_LOGI("app_main", "Type: %s", (card->is_mmc) ? "MMC" : (card->is_sdio) ? "SDIO" : "SD");
    ESP_LOGI("app_main", "Capacity: %ld MB", size);
    ESP_LOGI("app_main", "Max Frequency: %d kHz", card->max_freq_khz);

    

    const char* test_file_path = MOUNT_POINT"/程艾影-赵雷.lrc";
    FILE* f = fopen(test_file_path, "r");
    if (f == NULL) {
        ESP_LOGE("app_main", "Failed to open file %s", test_file_path);
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        // 输出读取的每一行内容
        printf("%s", line);
    }
    

    
    while (1)
    {
        // ESP_LOGI("main", "Hello world!");
        gpio_set_level(GPIO_NUM_51, 1); // Set GPIO51 high
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(GPIO_NUM_51, 0); // Set GPIO51 low
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}
