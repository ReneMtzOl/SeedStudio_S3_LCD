#include "PinoutDefinitions.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"     // IWYU pragma: keep
#include "i2c_manager.h"
#include "lcd_touchpad.h"
#include <stdio.h>


void app_main(void) {
  // Configure and apply reset sequence for touchpad
  gpio_set_direction(TP_RST, GPIO_MODE_OUTPUT);
  gpio_set_level(TP_RST, 0);
  vTaskDelay(10 / portTICK_PERIOD_MS);
  gpio_set_level(TP_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  // Initialize the I2C manager with the pins from PinoutDefinitions.h
  i2c_manager_init(TP_SDA, TP_SCL);

  // Initialize the FT6336U driver
  if (ft6336u_init() == ESP_OK) {
      printf("Touchpad initialized successfully.\n");
  } else {
      printf("Failed to initialize Touchpad.\n");
  }

  uint16_t x = 0, y = 0;
  bool is_pressed = false;

  printf("Waiting for touch screen interactions...\n");

  while(1) {
      if (ft6336u_read_pos(&x, &y, &is_pressed) == ESP_OK) {
          if (is_pressed) {
              printf("Touch Detected! X: %d, Y: %d\n", x, y);
          }
      }
      // Read at 20Hz (every 50ms)
      vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
