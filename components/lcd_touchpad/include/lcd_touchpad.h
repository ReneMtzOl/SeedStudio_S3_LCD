#ifndef FT6336U_H
#define FT6336U_H
/*Touchpad Sensor in this dev board is a FT6336U capacitive I2C IC*/

#include <esp_err.h>

// Dirección I2C del FT6336U
#define FT6336U_ADDR 0x38

// Pines del touchpad (definidos en PinoutDefinitions.h)
// TP_SDA: 6
// TP_SCL: 5
// TP_INT: 7
// TP_RST: 4

// Función de inicialización del touchpad FT6336U
esp_err_t ft6336u_init(void);

#endif