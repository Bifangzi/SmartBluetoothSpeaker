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

#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"

#include "lvgl.h"
#include "esp_timer.h"
#include <sys/lock.h>
#include "unistd.h"
#include "esp_ldo_regulator.h"


/* SDMMC 外设相关硬件引脚 */
#define SDMMC_PIN_CMD GPIO_NUM_44
#define SDMMC_PIN_CLK GPIO_NUM_43
#define SDMMC_PIN_D0 GPIO_NUM_39
#define SDMMC_PIN_D1 GPIO_NUM_40
#define SDMMC_PIN_D2 GPIO_NUM_41
#define SDMMC_PIN_D3 GPIO_NUM_42
/* 挂载名称 */
#define MOUNT_POINT "/0:"


// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;


extern void example_lvgl_demo_ui(lv_display_t *disp);


static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}


static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(2);
}


static bool example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}


static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI("TEST", "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of triggering a task watch dog time out
        time_till_next_ms = MAX(time_till_next_ms, 1500);
        // in case of lvgl display not ready yet
        time_till_next_ms = MIN(time_till_next_ms, 500);
        usleep(1000 * time_till_next_ms);
    }
}


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

    fclose(f);


    ESP_LOGI("MIPI", "MIPI DSI PHY Powered on");
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_mipi_phy_config = {
        .chan_id = 3,
        .voltage_mv = 1800,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_mipi_phy_config, &ldo_mipi_phy));

    ESP_LOGI("MIPI", "Initialize MIPI DSI bus");
    esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
    esp_lcd_dsi_bus_config_t bus_config = {
        .bus_id = 0,
        .num_data_lanes = 2,
        .lane_bit_rate_mbps = 400,
        // .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,
    };
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus));

    ESP_LOGI("MIPI", "Install panel IO");
    esp_lcd_panel_io_handle_t mipi_dbi_io = NULL;
    esp_lcd_dbi_io_config_t dbi_config = ST7701_PANEL_IO_DBI_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &mipi_dbi_io));

    ESP_LOGI("MIPI", "Install LCD driver of st7701");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_dpi_panel_config_t dpi_config = {0};
    dpi_config.virtual_channel = 0;
    dpi_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
    dpi_config.dpi_clock_freq_mhz = 27;
    // dpi_config.pixel_format = COLOR_PIXEL_RGB888;
    dpi_config.in_color_format = LCD_COLOR_FMT_RGB565;
    // dpi_config.out_color_format = LCD_COLOR_FMT_RGB888;
    dpi_config.num_fbs = 2;
    dpi_config.video_timing.h_size = 480;
    dpi_config.video_timing.v_size = 800;
    dpi_config.video_timing.hsync_pulse_width = 4;
    dpi_config.video_timing.hsync_back_porch = 32;
    dpi_config.video_timing.hsync_front_porch = 32;
    dpi_config.video_timing.vsync_pulse_width = 4;
    dpi_config.video_timing.vsync_back_porch = 3;
    dpi_config.video_timing.vsync_front_porch = 9;
    dpi_config.flags.use_dma2d = true;
    
    
    st7701_vendor_config_t vendor_config = {
        // .init_cmds = lcd_init_cmds,      // Uncomment these line if use custom initialization commands
        // .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_t),
        .flags.use_mipi_interface = 1,
        .mipi_config = {
            .dsi_bus = mipi_dsi_bus,
            .dpi_config = &dpi_config,
        },
    };

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = 45,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(mipi_dbi_io, &panel_config, &panel_handle));
    ESP_LOGI("MIPI", "LCD panel installed successfully");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_LOGI("MIPI", "LCD panel reset done");

    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_LOGI("MIPI", "LCD panel init done");

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI("MIPI", "LCD panel display on");




    ESP_LOGI("TEST", "Initialize LVGL library");
    lv_init();
    // create a lvgl display
    lv_display_t *display = lv_display_create(480, 800);
    // associate the mipi panel handle to the display
    lv_display_set_user_data(display, panel_handle);
    // set color depth
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    // create draw buffer
    void *buf1 = NULL;
    void *buf2 = NULL;
    ESP_LOGI("TEST", "Allocate separate LVGL draw buffers");
    // Note:
    // Keep the display buffer in **internal** RAM can speed up the UI because LVGL uses it a lot and it should have a fast access time
    // This example allocate the buffer from PSRAM mainly because we want to save the internal RAM
    size_t draw_buffer_sz = 800 * 48 * sizeof(lv_color_t);
    buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf1);
    buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    ESP_LOGI("TEST", "Register DPI panel event callback for LVGL flush ready notification");
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
#if CONFIG_EXAMPLE_MONITOR_REFRESH_BY_GPIO
        .on_refresh_done = example_monitor_refresh_rate,
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(panel_handle, &cbs, display));

    ESP_LOGI("TEST", "Use esp_timer as LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));

    ESP_LOGI("TEST", "Create LVGL task");
    xTaskCreate(example_lvgl_port_task, "LVGL", 4096, NULL, 2, NULL);

    ESP_LOGI("TEST", "Display LVGL Meter Widget");
    _lock_acquire(&lvgl_api_lock);
    example_lvgl_demo_ui(display);
    _lock_release(&lvgl_api_lock);


    
    while (1)
    {
        // ESP_LOGI("main", "Hello world!");
        gpio_set_level(GPIO_NUM_51, 1); // Set GPIO51 high
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(GPIO_NUM_51, 0); // Set GPIO51 low
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}
