#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "lcd_touchpad.h"
#include "i2c_manager.h"

static const char *TAG = "FT6336U";

// Internal helper declaration
static esp_err_t ft6336u_read_byte(uint8_t reg, uint8_t *val);

esp_err_t ft6336u_init(void)
{
    ESP_LOGI(TAG, "Initializing FT6336U touchpad...");

    // Register device in I2C manager
    esp_err_t ret = i2c_register_device(FT6336U_ADDR, "FT6336U", FT6336U_I2C_FREQ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register FT6336U in I2C Manager: %s", esp_err_to_name(ret));
        return ret;
    }

    // Read Vendor ID to verify communication
    uint8_t vend_id = 0;
    ret = ft6336u_read_byte(FT6336U_REG_VEND_ID, &vend_id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error reading Vendor ID: %s", esp_err_to_name(ret));
        return ret;
    }

    if (vend_id != 0x11) {
        ESP_LOGW(TAG, "Unexpected Vendor ID: 0x%02X (Expected 0x11)", vend_id);
    } else {
        ESP_LOGI(TAG, "Vendor ID verified: 0x%02X", vend_id);
    }

    // Read Chip ID
    uint8_t chip_id = 0;
    ft6336u_read_byte(FT6336U_REG_CHIP_ID, &chip_id);
    ESP_LOGI(TAG, "Chip ID: 0x%02X", chip_id);

    return ESP_OK;
}

static esp_err_t ft6336u_read_byte(uint8_t reg, uint8_t *val)
{
    return i2c_write_read(FT6336U_ADDR, &reg, 1, val, 1);
}

esp_err_t ft6336u_read_pos(uint16_t *x, uint16_t *y, bool *is_pressed)
{
    if (x == NULL || y == NULL || is_pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t td_status = 0;
    esp_err_t ret = ft6336u_read_byte(FT6336U_REG_TD_STATUS, &td_status);
    if (ret != ESP_OK) {
        return ret;
    }

    uint8_t touches = td_status & 0x0F;
    if (touches == 0) {
        *is_pressed = false;
        return ESP_OK;
    }

    // Read registers 0x03 to 0x06 (4 bytes) for Touch 1
    uint8_t reg = FT6336U_REG_P1_XH;
    uint8_t data[4] = {0};
    
    ret = i2c_write_read(FT6336U_ADDR, &reg, 1, data, 4);
    if (ret != ESP_OK) {
        return ret;
    }

    *is_pressed = true;
    
    // Extracting coordinates with bitwise masks
    *x = ((data[0] & 0x0F) << 8) | data[1];
    *y = ((data[2] & 0x0F) << 8) | data[3];

    return ESP_OK;
}

/* =========================================================================
 * LVGL COMPATIBILITY WRAPPER
 * =========================================================================
 * The following mock structures simulate the LVGL API.
 * Once LVGL is installed, you can delete these local structures,
 * include "lvgl.h" and cast directly to lv_indev_data_t.
 */
typedef enum {
    LV_INDEV_STATE_RELEASED = 0,
    LV_INDEV_STATE_PRESSED
} lv_indev_state_t;

typedef struct {
    int32_t x;
    int32_t y;
} lv_point_t;

typedef struct {
    lv_point_t point;
    lv_indev_state_t state;
    uint32_t continue_reading;
} lv_indev_data_mock_t;

void lvgl_touch_cb(void * indev_drv, void * data)
{
    // Once LVGL is incorporated, change lv_indev_data_mock_t to lv_indev_data_t
    lv_indev_data_mock_t * touch_data = (lv_indev_data_mock_t *)data;
    
    uint16_t x = 0, y = 0;
    bool is_pressed = false;

    if (ft6336u_read_pos(&x, &y, &is_pressed) == ESP_OK) {
        if (is_pressed) {
            touch_data->state = LV_INDEV_STATE_PRESSED;
            touch_data->point.x = x;
            touch_data->point.y = y;
        } else {
            touch_data->state = LV_INDEV_STATE_RELEASED;
        }
    } else {
        touch_data->state = LV_INDEV_STATE_RELEASED;
    }
}

