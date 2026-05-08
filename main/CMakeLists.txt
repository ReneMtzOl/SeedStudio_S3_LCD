idf_component_register(SRCS "hello_world_main.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS ""
                       REQUIRES i2c_manager esp_driver_gpio PinoutDefinitions lcd_touchpad lcd_display)
