idf_component_register(
    SRCS "ui.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_lcd
        esp_lvgl_port
        lcd_display
        lcd_touchpad
)
