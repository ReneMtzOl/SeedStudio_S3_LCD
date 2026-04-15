#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <stdint.h>
#include <esp_err.h>
#include "driver/i2c_master.h"
#include "PinoutDefinitions.h"


// Estructura para dispositivo I2C
typedef struct {
    uint8_t address;
    char name[32];
    i2c_master_dev_handle_t dev_handle;
} i2c_device_t;

// Función para inicializar el manager I2C
esp_err_t i2c_manager_init(void);

// Función para escanear dispositivos I2C en el bus (solo muestra, no registra)
void i2c_scanner(void);

// Función para registrar dispositivos encontrados en el bus I2C
esp_err_t i2c_register_from_scan(void);

// Función para registrar un dispositivo manualmente
esp_err_t i2c_register_device(uint8_t addr, const char* name);

// Función para de-registrar un dispositivo específico
esp_err_t i2c_unregister_device(uint8_t addr);

// Función para de-registrar todos los dispositivos
esp_err_t i2c_unregister_all_devices(void);

// Función para de-inicializar el manager I2C
esp_err_t i2c_manager_deinit(void);

// Función para imprimir dispositivos registrados (para debug)
void i2c_print_registered_devices(void);

// Función para escribir datos a un dispositivo I2C
esp_err_t i2c_write(uint8_t addr, const uint8_t *data, size_t len);

// Función para leer datos de un dispositivo I2C
esp_err_t i2c_read(uint8_t addr, uint8_t *data, size_t len);

// Función para escribir y luego leer datos (operación común)
esp_err_t i2c_write_read(uint8_t addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len);

#define I2C_MASTER_SDA_IO TP_SDA
#define I2C_MASTER_SCL_IO TP_SCL
#define I2C_MASTER_FREQ_HZ 100000

#endif // I2C_MANAGER_H
