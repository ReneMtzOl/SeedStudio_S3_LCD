idf_component_register(
    SRCS "lcd_display.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_lcd
        esp_driver_ledc
        esp_driver_gpio
        PinoutDefinitions
        esp_lcd_st7796
        freertos
        hal
)
