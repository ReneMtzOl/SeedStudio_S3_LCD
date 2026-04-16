#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "lcd_touchpad.h"
#include "i2c_manager.h"

static const char *TAG = "FT6336U";

esp_err_t ft6336u_init(void)
{
    ESP_LOGI(TAG, "Inicializando FT6336U touchpad...");

    // Verificar si el dispositivo ya está registrado en el bus I2C
    // Para esto, intentamos cambiar su nombre; si no existe, fallará
    esp_err_t ret = i2c_set_device_name(FT6336U_ADDR, "FT6336U");
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "FT6336U encontrado y registrado en el bus I2C");
        return ESP_OK;
    } else if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "FT6336U no encontrado en el bus I2C. Asegúrate de que esté conectado y registrado.");
        return ESP_ERR_NOT_FOUND;
    } else {
        ESP_LOGE(TAG, "Error al inicializar FT6336U: %s", esp_err_to_name(ret));
        return ret;
    }
}
