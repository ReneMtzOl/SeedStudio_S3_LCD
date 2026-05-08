#ifndef FT6336U_H
#define FT6336U_H
/*Touchpad Sensor in this dev board is a FT6336U capacitive I2C IC*/

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/**
 * @brief I2C Address of FT6336U
 */
#define FT6336U_ADDR 0x38

/**
 * @brief Recommended I2C Frequency for FT6336U
 */
#define FT6336U_I2C_FREQ 400000

/**
 * @name FT6336U Registers
 */
#define FT6336U_REG_DEV_MODE    0x00
#define FT6336U_REG_GEST_ID     0x01
#define FT6336U_REG_TD_STATUS   0x02
#define FT6336U_REG_P1_XH       0x03
#define FT6336U_REG_P1_XL       0x04
#define FT6336U_REG_P1_YH       0x05
#define FT6336U_REG_P1_YL       0x06
#define FT6336U_REG_P1_WEIGHT   0x07
#define FT6336U_REG_P1_MISC     0x08
#define FT6336U_REG_P2_XH       0x09
#define FT6336U_REG_P2_XL       0x0A
#define FT6336U_REG_P2_YH       0x0B
#define FT6336U_REG_P2_YL       0x0C
#define FT6336U_REG_P2_WEIGHT   0x0D
#define FT6336U_REG_P2_MISC     0x0E
#define FT6336U_REG_CHIP_ID     0xA3
#define FT6336U_REG_VEND_ID     0xA8

/**
 * @brief Initialize the FT6336U touchpad
 * 
 * Validates the Vendor ID and registers the device in the I2C manager.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t ft6336u_init(void);

/**
 * @brief Read the current touch coordinates from FT6336U
 * 
 * @param x Pointer to store the X coordinate
 * @param y Pointer to store the Y coordinate
 * @param is_pressed Pointer to store whether the screen is currently touched
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t ft6336u_read_pos(uint16_t *x, uint16_t *y, bool *is_pressed);

/**
 * @brief Wrapper function for LVGL touch driver
 * 
 * Note: LVGL v8/v9 uses different structures. This signature assumes a generic
 * pointer structure that matches the `lv_indev_drv_t` read_cb callback.
 * 
 * @param indev_drv Pointer to the LVGL input device driver
 * @param data Pointer to the LVGL input data structure
 */
void lvgl_touch_cb(void * indev_drv, void * data);

#endif // FT6336U_H