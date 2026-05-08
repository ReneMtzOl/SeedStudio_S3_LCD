#ifndef UI_H
#define UI_H

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL v9 with display and touch input.
 *
 * Must be called after lcd_display_init() and ft6336u_init().
 * Internally calls lvgl_port_init(), registers the display and
 * touch device, then creates the demo screen.
 *
 * @param panel_handle  Panel handle returned by lcd_display_init()
 * @return ESP_OK on success
 */
esp_err_t ui_init(esp_lcd_panel_handle_t panel_handle);

#ifdef __cplusplus
}
#endif

#endif // UI_H
