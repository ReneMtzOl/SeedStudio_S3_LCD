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
    
    vTaskDelay(200 / portTICK_PERIOD_MS);

    printf("RST PIN = 0\n");
    gpio_set_level(TP_RST, 0);
    i2c_scanner();

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    /*
    for(int i = device_count - 1; i >= 0; i--) {
        printf("Device[%d] = 0x%X...\n", i, registered_devices[i].addr);
    }
    */

    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
