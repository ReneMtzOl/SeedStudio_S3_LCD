#include "PinoutDefinitions.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"     // IWYU pragma: keep
#include "i2c_manager.h"
#include <stdio.h>


void app_main(void) {
  gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);

  printf("RST PIN = 1\n");
  gpio_set_level(TP_RST, 1);

  // Inicializar el manager I2C con los pines de PinoutDefinitions.h
  i2c_manager_init(TP_SDA, TP_SCL);

  i2c_scanner();
  // Registrar dispositivos con velocidad estándar de 100kHz
  i2c_register_from_scan(100000);

  vTaskDelay(300 / portTICK_PERIOD_MS);
  i2c_print_registered_devices();
  vTaskDelay(300 / portTICK_PERIOD_MS);

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
