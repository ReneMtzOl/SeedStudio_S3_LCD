#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <stdint.h>
#include <esp_err.h>
#include "driver/i2c_master.h"

/**
 * @brief Structure to represent an I2C device.
 */
typedef struct {
    uint8_t address;                    /*!< I2C device address */
    char name[32];                      /*!< I2C device name */
    i2c_master_dev_handle_t dev_handle; /*!< I2C master device handle */
} i2c_device_t;

/**
 * @brief Initialize the I2C master manager.
 *
 * @param sda_pin GPIO pin to use for SDA.
 * @param scl_pin GPIO pin to use for SCL.
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_manager_init(int sda_pin, int scl_pin);

/**
 * @brief Scan for I2C devices on the bus (prints the found addresses, does not register).
 */
void i2c_scanner(void);

/**
 * @brief Register I2C devices found on the bus automatically.
 *
 * @param freq_hz Frequency in Hz to use for the found devices (e.g., 100000 for 100kHz).
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_register_from_scan(uint32_t freq_hz);

/**
 * @brief Register a specific I2C device manually.
 *
 * @param addr I2C address of the device.
 * @param name Custom name for the device.
 * @param freq_hz Frequency in Hz to use for this specific device.
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_register_device(uint8_t addr, const char* name, uint32_t freq_hz);

/**
 * @brief Unregister a specific I2C device by its address.
 *
 * @param addr I2C address of the device.
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_NOT_FOUND: Device not found
 *     - Others: Fail
 */
esp_err_t i2c_unregister_device(uint8_t addr);

/**
 * @brief Unregister all currently registered I2C devices.
 *
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_unregister_all_devices(void);

/**
 * @brief De-initialize the I2C master manager and free resources.
 *
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_manager_deinit(void);

/**
 * @brief Print the list of registered I2C devices (useful for debugging).
 */
void i2c_print_registered_devices(void);

/**
 * @brief Write data to an I2C device.
 *
 * @param addr I2C address of the device.
 * @param data Pointer to the data buffer to be written.
 * @param len Length of the data to write.
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_write(uint8_t addr, const uint8_t *data, size_t len);

/**
 * @brief Read data from an I2C device.
 *
 * @param addr I2C address of the device.
 * @param data Pointer to the buffer where received data will be stored.
 * @param len Length of the data to read.
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_read(uint8_t addr, uint8_t *data, size_t len);

/**
 * @brief Write data to and then read data from an I2C device in a single transaction.
 *
 * @param addr I2C address of the device.
 * @param write_data Pointer to the data buffer to be written.
 * @param write_len Length of the data to write.
 * @param read_data Pointer to the buffer where received data will be stored.
 * @param read_len Length of the data to read.
 * @return
 *     - ESP_OK: Success
 *     - Others: Fail
 */
esp_err_t i2c_write_read(uint8_t addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len);

/**
 * @brief Update the name of an already registered I2C device.
 *
 * @param addr I2C address of the device.
 * @param name New name for the device.
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_NOT_FOUND: Device not found
 *     - Others: Fail
 */
esp_err_t i2c_set_device_name(uint8_t addr, const char* name);

#endif // I2C_MANAGER_H
