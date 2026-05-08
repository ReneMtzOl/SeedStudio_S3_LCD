#include "PinoutDefinitions.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "lcd_touchpad.h"
#include "lcd_display.h"
#include "ui.h"
#include <stdio.h>


void app_main(void)
{
    /* ── Touch controller hard reset ─────────────────────────── */
    gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(TP_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TP_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    /* ── I2C manager ─────────────────────────────────────────── */
    ESP_ERROR_CHECK(i2c_manager_init(TP_SDA, TP_SCL));

    /* ── Touchpad ────────────────────────────────────────────── */
    if (ft6336u_init() == ESP_OK) {
        printf("Touchpad initialized.\n");
    } else {
        printf("Touchpad init failed – continuing without touch.\n");
    }

    /* ── LCD display ─────────────────────────────────────────── */
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(lcd_display_init(&panel_handle));
    printf("LCD display initialized.\n");

    /* ── Backlight on ────────────────────────────────────────── */
    ESP_ERROR_CHECK(lcd_set_backlight(80));

    /* ── LVGL + UI ───────────────────────────────────────────── */
    ESP_ERROR_CHECK(ui_init(panel_handle));
    printf("UI ready.\n");

    /* app_main returns – LVGL runs in its own FreeRTOS task */
}
