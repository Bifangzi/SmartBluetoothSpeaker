idf_component_register(SRCS "lvgl_demo_ui.c" "hello_world_main.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS ""
                       REQUIRES driver
                       REQUIRES fatfs
                       REQUIRES esp_timer)

