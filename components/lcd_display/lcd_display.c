#include "lcd_display.h"
#include "PinoutDefinitions.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/lcd_types.h"

static const char *TAG = "LCD_DISPLAY";

/* Pixel clock: keep conservative at 10 MHz for stability */
#define LCD_PIXEL_CLOCK_HZ   (10 * 1000 * 1000)

/* ── LEDC backlight ───────────────────────────────────────────── */
#define LCD_LEDC_TIMER       LEDC_TIMER_0
#define LCD_LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LCD_LEDC_CHANNEL     LEDC_CHANNEL_0
#define LCD_LEDC_DUTY_RES    LEDC_TIMER_8_BIT   /* 0-255 */
#define LCD_LEDC_FREQUENCY   (5000)              /* 5 kHz */
#define LCD_LEDC_DUTY_MAX    ((1 << 8) - 1)      /* 255  */

/* ─────────────────────────────────────────────────────────────── */

esp_err_t lcd_set_backlight(uint8_t brightness_percent)
{
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    uint32_t duty = (brightness_percent * LCD_LEDC_DUTY_MAX) / 100;
    ESP_LOGD(TAG, "Backlight %d%% → duty %lu", brightness_percent, (unsigned long)duty);

    ESP_RETURN_ON_ERROR(ledc_set_duty(LCD_LEDC_MODE, LCD_LEDC_CHANNEL, duty),
                        TAG, "ledc_set_duty failed");
    return ledc_update_duty(LCD_LEDC_MODE, LCD_LEDC_CHANNEL);
}

/* ─────────────────────────────────────────────────────────────── */

esp_err_t lcd_display_init(esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = ESP_OK;

    /* ── 1. LEDC backlight ──────────────────────────────────────── */
    ESP_LOGI(TAG, "Initializing LEDC backlight...");

    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LCD_LEDC_MODE,
        .timer_num       = LCD_LEDC_TIMER,
        .duty_resolution = LCD_LEDC_DUTY_RES,
        .freq_hz         = LCD_LEDC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
        .deconfigure     = false,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), TAG, "LEDC timer config failed");

    ledc_channel_config_t ledc_ch = {
        .gpio_num   = LCD_BL,
        .speed_mode = LCD_LEDC_MODE,
        .channel    = LCD_LEDC_CHANNEL,
        .timer_sel  = LCD_LEDC_TIMER,
        .duty       = 0,          /* backlight off until display is ready */
        .hpoint     = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags      = { .output_invert = 0 },
        .deconfigure = false,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_ch), TAG, "LEDC channel config failed");

    /* ── 2. Intel 8080 (I80) bus ─────────────────────────────────── */
    ESP_LOGI(TAG, "Initializing Intel 8080 bus...");

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        /* IDF v6 uses dma_burst_size instead of psram/sram_trans_align */
        .clk_src        = LCD_CLK_SRC_DEFAULT,
        .dc_gpio_num    = LCD_RS,
        .wr_gpio_num    = LCD_WR,
        .data_gpio_nums = {
            LCD_DB0, LCD_DB1, LCD_DB2, LCD_DB3,
            LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7,
            /* remaining 8 entries are filled with -1 by C zero-init */
        },
        .bus_width          = 8,
        .max_transfer_bytes = LCD_H_RES * 40 * sizeof(uint16_t),
        .dma_burst_size     = 64,
    };
    ret = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I80 bus creation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── 3. Panel IO (I80) ──────────────────────────────────────── */
    ESP_LOGI(TAG, "Initializing I80 panel IO...");

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num      = -1,               /* CS not used – exclusive bus */
        .pclk_hz          = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx         = NULL,
        .lcd_cmd_bits     = 8,
        .lcd_param_bits   = 8,
        .dc_levels = {
            .dc_idle_level  = 0,
            .dc_cmd_level   = 0,
            .dc_dummy_level = 0,
            .dc_data_level  = 1,
        },
        .flags = {
            .swap_color_bytes = 1,  /* ST7796 expects big-endian RGB565 */
        },
    };
    ret = esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I80 panel IO creation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── 4. ST7796 panel driver ─────────────────────────────────── */
    ESP_LOGI(TAG, "Installing ST7796 panel driver...");

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RESET,
        /* IDF v6 uses rgb_ele_order instead of color_space */
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian    = LCD_RGB_DATA_ENDIAN_BIG,
        .bits_per_pixel = 16,
        .vendor_config  = NULL,
        .flags          = { .reset_active_high = 0 },
    };
    ret = esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ST7796 panel creation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ── 5. Reset and initialize ────────────────────────────────── */
    ESP_LOGI(TAG, "Resetting panel...");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "Panel reset failed");
    vTaskDelay(pdMS_TO_TICKS(120));

    ESP_LOGI(TAG, "Initializing panel...");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "Panel init failed");

    /* Turn on display */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true),
                        TAG, "Display on failed");

    /* Invert colors – typical for IPS panels */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, true),
                        TAG, "Invert color failed");

    /* Landscape: swap X/Y axes, mirror Y */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel_handle, true),
                        TAG, "Swap XY failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle, false, true),
                        TAG, "Mirror failed");

    if (ret_panel != NULL) {
        *ret_panel = panel_handle;
    }

    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}
