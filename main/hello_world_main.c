#include <stdio.h>
#include "PinoutDefinitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "driver/gpio.h"

void app_main(void)
{
    gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);

    printf("RST PIN = 1\n");
    gpio_set_level(TP_RST, 1);

    // Inicializar el manager I2C
    i2c_manager_init();

    i2c_scanner();
    i2c_register_from_scan();

    vTaskDelay(300 / portTICK_PERIOD_MS);
    i2c_print_registered_devices();
    vTaskDelay(300 / portTICK_PERIOD_MS);

    // Test del touchpad FT6336U
    uint8_t ft6336_addr = 0x38;
    uint8_t reg_read[1];
    uint8_t chip_id, fw_version;

    printf("\n=== Leyendo chip FT6336U ===\n");

    // Leer ID del chip (0xA3)
    if (i2c_write_read(ft6336_addr, (uint8_t[]){0xA3}, 1, reg_read, 1) == ESP_OK) {
        chip_id = reg_read[0];
        printf("ID del chip: 0x%02X\n", chip_id);
    } else {
        printf("Error leyendo ID del chip\n");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Leer versión de firmware (0xA6)
    if (i2c_write_read(ft6336_addr, (uint8_t[]){0xA6}, 1, reg_read, 1) == ESP_OK) {
        fw_version = reg_read[0];
        printf("Versión de firmware: 0x%02X\n", fw_version);
    } else {
        printf("Error leyendo versión de firmware\n");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Loop de lectura del registro 0x00 cada 500ms
    printf("\n=== Leyendo registros 0x00-0x09 (estado + datos de toque) cada 500ms ===\n");
    uint8_t touch_data[10];
    for (int i = 0; i < 10; i++) {
        if (i2c_write_read(ft6336_addr, (uint8_t[]){0x00}, 1, touch_data, 10) == ESP_OK) {
            printf("[%d] Datos completos: ", i);
            for (int j = 0; j < 10; j++) {
                printf("%02X ", touch_data[j]);
            }
            printf("\n");
            printf("    Estado: 0x%02X (puntos táctiles: %d)\n", touch_data[0], touch_data[0] & 0x0F);
            printf("    P1 X: 0x%02X%02X, Y: 0x%02X%02X\n", touch_data[2] & 0x0F, touch_data[3], touch_data[4] & 0x0F, touch_data[5]);
        } else {
            printf("[%d] Error leyendo registro 0x00\n", i);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    printf("\n=== Fin del test ===\n\n");

for (int i = 3; i >= 0; i--)
{
    printf("Restarting in %d seconds...\n", i);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// De-registrar dispositivos y de-inicializar bus I2C antes de reiniciar
    i2c_unregister_all_devices();
    i2c_manager_deinit();
    
    vTaskDelay(300 / portTICK_PERIOD_MS);
    i2c_print_registered_devices();
    vTaskDelay(300 / portTICK_PERIOD_MS);
    
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();

}
