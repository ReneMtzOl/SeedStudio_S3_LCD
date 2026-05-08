#include "ui.h"
#include "lcd_display.h"
#include "lcd_touchpad.h"
#include "PinoutDefinitions.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"
#include "lvgl.h"

static const char *TAG = "UI";

static lv_display_t *s_disp       = NULL;
static lv_indev_t   *s_touch_indev = NULL;

/* ── LVGL touch read callback (manual indev) ──────────────────── */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t x = 0, y = 0;
    bool pressed = false;

    if (ft6336u_read_pos(&x, &y, &pressed) == ESP_OK && pressed) {
        /* FT6336U raw coords are in portrait (320 wide × 480 tall).
         * Display is landscape: H=480, V=320.
         * After swap_xy=true, mirror_y=true in panel:
         *   LVGL x ← raw y
         *   LVGL y ← (LCD_V_RES - 1) - raw x   */
        data->point.x = (lv_coord_t)y;
        data->point.y = (lv_coord_t)(LCD_V_RES - 1 - x);
        data->state   = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ── Demo screen ─────────────────────────────────────────────── */
static void btn_click_cb(lv_event_t *e)
{
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    static uint32_t count = 0;
    count++;
    lv_label_set_text_fmt(label, "Pulsaciones: %" PRIu32, count);
}

static void create_demo_screen(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* Background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    /* Title – uses default font (always available) */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ESP32-S3  *  LVGL v9");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE94560), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    /* Counter label */
    lv_obj_t *counter_lbl = lv_label_create(scr);
    lv_label_set_text(counter_lbl, "Pulsaciones: 0");
    lv_obj_set_style_text_color(counter_lbl, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
    lv_obj_align(counter_lbl, LV_ALIGN_CENTER, 0, -24);

    /* Button */
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 180, 52);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 36);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xE94560), LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, counter_lbl);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, LV_SYMBOL_PLAY "  Tocar aqui");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(btn_lbl);

    /* Footer */
    lv_obj_t *info = lv_label_create(scr);
    lv_label_set_text(info, "ST7796  480x320  I8080  FT6336U");
    lv_obj_set_style_text_color(info, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -10);

    ESP_LOGI(TAG, "Demo screen created");
}

/* ── Public init ─────────────────────────────────────────────── */
esp_err_t ui_init(esp_lcd_panel_handle_t panel_handle)
{
    ESP_LOGI(TAG, "Initializing LVGL port...");

    /* 1. Init LVGL port (creates LVGL task + tick timer) */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init failed");

    /* 2. Register display */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle      = NULL,
        .panel_handle   = panel_handle,
        .control_handle = NULL,
        .buffer_size    = LCD_H_RES * 40,
        .double_buffer  = true,
        .trans_size     = 0,
        .hres           = LCD_H_RES,
        .vres           = LCD_V_RES,
        .monochrome     = false,
        .color_format   = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy  = true,
            .mirror_x = false,
            .mirror_y = true,
        },
        .rounder_cb     = NULL,
        .flags = {
            .buff_dma    = true,
            .buff_spiram = false,
            .sw_rotate   = false,
            .swap_bytes  = false,  /* swap already done in i80 io_config */
            .full_refresh = false,
            .direct_mode  = false,
        },
    };
    s_disp = lvgl_port_add_disp(&disp_cfg);
    if (s_disp == NULL) {
        ESP_LOGE(TAG, "lvgl_port_add_disp failed");
        return ESP_FAIL;
    }

    /* 3. Register touch as a manual LVGL indev
     *    (lvgl_port_add_touch requires esp_lcd_touch which is not used here) */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
    lv_indev_set_display(indev, s_disp);
    s_touch_indev = indev;
    ESP_LOGI(TAG, "Touch indev registered");

    /* 4. Build demo screen (must hold the LVGL port lock) */
    lvgl_port_lock(0);
    create_demo_screen();
    lvgl_port_unlock();

    ESP_LOGI(TAG, "UI initialized");
    return ESP_OK;
}
