idf_component_register(SRCS "main.c" "lvgl_demo_ui.c"
                       PRIV_REQUIRES spi_flash uart0
                       INCLUDE_DIRS ""
                       REQUIRES driver
                       REQUIRES fatfs
                       REQUIRES esp_timer
                       REQUIRES lvgl)

