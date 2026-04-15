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

// Pines I2C definidos en pinout.h
#define I2C_MASTER_SDA_IO TP_SDA
#define I2C_MASTER_SCL_IO TP_SCL
#define I2C_MASTER_FREQ_HZ 100000

#endif // I2C_MANAGER_H
