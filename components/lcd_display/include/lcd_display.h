#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <esp_err.h>
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

/**
 * @brief Resolution of the ST7796 LCD display
 */
#define LCD_H_RES 480
#define LCD_V_RES 320

/**
 * @brief Initialize the I8080 bus and ST7796 display.
 * 
 * Also sets up the backlight PWM channel.
 * 
 * @param[out] ret_panel Pointer to store the panel handle (useful for LVGL)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lcd_display_init(esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Set the backlight brightness via PWM
 * 
 * @param brightness_percent Brightness from 0 to 100
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lcd_set_backlight(uint8_t brightness_percent);

#endif // LCD_DISPLAY_H
