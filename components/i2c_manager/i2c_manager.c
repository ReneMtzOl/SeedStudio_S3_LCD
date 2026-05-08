#include "i2c_manager.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define MAX_DEVICES 10

static const char *TAG = "I2C_MANAGER";

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_device_t registered_devices[MAX_DEVICES];
static int device_count = 0;
static SemaphoreHandle_t i2c_manager_mutex = NULL;

esp_err_t i2c_manager_init(int sda_pin, int scl_pin) {
  if (bus_handle != NULL) {
    ESP_LOGW(TAG, "I2C bus is already initialized");
    return ESP_OK;
  }

  if (i2c_manager_mutex == NULL) {
    i2c_manager_mutex = xSemaphoreCreateMutex();
    if (i2c_manager_mutex == NULL) {
      ESP_LOGE(TAG, "Failed to create I2C manager mutex");
      return ESP_ERR_NO_MEM;
    }
  }

  ESP_LOGI(TAG, "Initializing master I2C bus on SDA=%d, SCL=%d...", sda_pin,
           scl_pin);

  // Configure the master I2C bus
  i2c_master_bus_config_t bus_config = {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = sda_pin,
      .scl_io_num = scl_pin,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
  };

  esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error creating master I2C bus: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "Master I2C bus successfully initialized");
  return ESP_OK;
}

void i2c_scanner(void) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized. Call i2c_manager_init() first.");
    return;
  }

  ESP_LOGI(TAG, "Starting I2C device scan...");

  // Scan addresses
  printf("Scanning I2C devices...\n");
  printf("Found devices:\n");
  for (uint8_t addr = 1; addr < 127; addr++) {
    esp_err_t ret =
        i2c_master_probe(bus_handle, addr, 1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK) {
      printf("0x%02X ", addr);
    }
  }
  printf("\nScan completed.\n");
}

esp_err_t i2c_register_from_scan(uint32_t freq_hz) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized. Call i2c_manager_init() first.");
    return ESP_ERR_INVALID_STATE;
  }

  ESP_LOGI(TAG, "Registering found devices on the I2C bus...");

  int registered = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    esp_err_t ret =
        i2c_master_probe(bus_handle, addr, 1000 / portTICK_PERIOD_MS);
    if (ret == ESP_OK) {
      char name[32];
      sprintf(name, "Device_0x%02X", addr);
      ret = i2c_register_device(addr, name, freq_hz);
      if (ret == ESP_OK) {
        registered++;
      }
    }
  }
  ESP_LOGI(TAG, "Registered devices: %d", registered);
  return ESP_OK;
}

esp_err_t i2c_register_device(uint8_t addr, const char *name,
                              uint32_t freq_hz) {
  if (bus_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  if (i2c_manager_mutex != NULL) {
    xSemaphoreTake(i2c_manager_mutex, portMAX_DELAY);
  }

  if (device_count >= MAX_DEVICES) {
    ESP_LOGE(TAG, "Cannot register more devices. Maximum reached: %d",
             MAX_DEVICES);
    if (i2c_manager_mutex != NULL)
      xSemaphoreGive(i2c_manager_mutex);
    return ESP_ERR_NO_MEM;
  }

  // Check if it is already registered
  for (int i = 0; i < device_count; i++) {
    if (registered_devices[i].address == addr) {
      ESP_LOGW(TAG, "Device 0x%02X is already registered", addr);
      if (i2c_manager_mutex != NULL)
        xSemaphoreGive(i2c_manager_mutex);
      return ESP_OK;
    }
  }

  // Configure the device
  i2c_device_config_t dev_config = {
      .device_address = addr,
      .scl_speed_hz = freq_hz,
  };

  i2c_master_dev_handle_t dev_handle;
  esp_err_t ret =
      i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error adding device 0x%02X: %s", addr, esp_err_to_name(ret));
    if (i2c_manager_mutex != NULL)
      xSemaphoreGive(i2c_manager_mutex);
    return ret;
  }

  registered_devices[device_count].address = addr;
  registered_devices[device_count].dev_handle = dev_handle;
  strncpy(registered_devices[device_count].name, name,
          sizeof(registered_devices[device_count].name) - 1);
  registered_devices[device_count]
      .name[sizeof(registered_devices[device_count].name) - 1] = '\0';
  device_count++;

  ESP_LOGI(TAG, "Device registered: %s (0x%02X) at %lu Hz", name, addr,
           (unsigned long)freq_hz);

  if (i2c_manager_mutex != NULL) {
    xSemaphoreGive(i2c_manager_mutex);
  }

  return ESP_OK;
}

static i2c_master_dev_handle_t get_dev_handle(uint8_t addr) {
  i2c_master_dev_handle_t handle = NULL;

  if (i2c_manager_mutex != NULL) {
    xSemaphoreTake(i2c_manager_mutex, portMAX_DELAY);
  }

  for (int i = 0; i < device_count; i++) {
    if (registered_devices[i].address == addr) {
      handle = registered_devices[i].dev_handle;
      break;
    }
  }

  if (i2c_manager_mutex != NULL) {
    xSemaphoreGive(i2c_manager_mutex);
  }

  return handle;
}

esp_err_t i2c_unregister_device(uint8_t addr) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (i2c_manager_mutex != NULL) {
    xSemaphoreTake(i2c_manager_mutex, portMAX_DELAY);
  }

  int index = -1;
  for (int i = 0; i < device_count; i++) {
    if (registered_devices[i].address == addr) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    ESP_LOGW(TAG, "Device 0x%02X not found", addr);
    if (i2c_manager_mutex != NULL)
      xSemaphoreGive(i2c_manager_mutex);
    return ESP_ERR_NOT_FOUND;
  }

  // Remove from the bus
  esp_err_t ret =
      i2c_master_bus_rm_device(registered_devices[index].dev_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error removing device 0x%02X: %s", addr,
             esp_err_to_name(ret));
    if (i2c_manager_mutex != NULL)
      xSemaphoreGive(i2c_manager_mutex);
    return ret;
  }

  // Shift the array
  for (int i = index; i < device_count - 1; i++) {
    registered_devices[i] = registered_devices[i + 1];
  }
  device_count--;

  ESP_LOGI(TAG, "Device 0x%02X unregistered", addr);

  if (i2c_manager_mutex != NULL) {
    xSemaphoreGive(i2c_manager_mutex);
  }

  return ESP_OK;
}

esp_err_t i2c_unregister_all_devices(void) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (i2c_manager_mutex != NULL) {
    xSemaphoreTake(i2c_manager_mutex, portMAX_DELAY);
  }

  for (int i = 0; i < device_count; i++) {
    esp_err_t ret = i2c_master_bus_rm_device(registered_devices[i].dev_handle);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Error removing device 0x%02X: %s",
               registered_devices[i].address, esp_err_to_name(ret));
      // Continue with the others
    }
  }
  device_count = 0;

  ESP_LOGI(TAG, "All devices unregistered");

  if (i2c_manager_mutex != NULL) {
    xSemaphoreGive(i2c_manager_mutex);
  }

  return ESP_OK;
}

esp_err_t i2c_manager_deinit(void) {
  if (bus_handle == NULL) {
    ESP_LOGW(TAG, "I2C bus already de-initialized");
    return ESP_OK;
  }

  // Unregister all devices
  esp_err_t ret = i2c_unregister_all_devices();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error unregistering devices during de-init");
    // Continue anyway
  }

  // Free the bus
  ret = i2c_del_master_bus(bus_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error freeing I2C bus: %s", esp_err_to_name(ret));
    return ret;
  }

  bus_handle = NULL;

  if (i2c_manager_mutex != NULL) {
    vSemaphoreDelete(i2c_manager_mutex);
    i2c_manager_mutex = NULL;
  }

  ESP_LOGI(TAG, "I2C bus successfully de-initialized");
  return ESP_OK;
}

void i2c_print_registered_devices(void) {
  if (i2c_manager_mutex != NULL) {
    xSemaphoreTake(i2c_manager_mutex, portMAX_DELAY);
  }

  if (device_count == 0) {
    ESP_LOGI(TAG, "No devices registered");
    if (i2c_manager_mutex != NULL)
      xSemaphoreGive(i2c_manager_mutex);
    return;
  }

  ESP_LOGI(TAG, "Registered devices (%d):", device_count);
  for (int i = 0; i < device_count; i++) {
    ESP_LOGI(TAG, "  [%d] %s (0x%02X)", i, registered_devices[i].name,
             registered_devices[i].address);
  }

  if (i2c_manager_mutex != NULL) {
    xSemaphoreGive(i2c_manager_mutex);
  }
}

esp_err_t i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (data == NULL || len == 0) {
    ESP_LOGE(TAG, "Invalid data for writing");
    return ESP_ERR_INVALID_ARG;
  }

  i2c_master_dev_handle_t dev_handle = get_dev_handle(addr);
  if (dev_handle == NULL) {
    ESP_LOGE(TAG, "Device 0x%02X not found", addr);
    return ESP_ERR_NOT_FOUND;
  }

  esp_err_t ret =
      i2c_master_transmit(dev_handle, data, len, 1000 / portTICK_PERIOD_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error writing to 0x%02X: %s", addr, esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Wrote %d bytes to 0x%02X", len, addr);
  return ESP_OK;
}

esp_err_t i2c_read(uint8_t addr, uint8_t *data, size_t len) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (data == NULL || len == 0) {
    ESP_LOGE(TAG, "Invalid buffer for reading");
    return ESP_ERR_INVALID_ARG;
  }

  i2c_master_dev_handle_t dev_handle = get_dev_handle(addr);
  if (dev_handle == NULL) {
    ESP_LOGE(TAG, "Device 0x%02X not found", addr);
    return ESP_ERR_NOT_FOUND;
  }

  esp_err_t ret =
      i2c_master_receive(dev_handle, data, len, 1000 / portTICK_PERIOD_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error reading from 0x%02X: %s", addr, esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Read %d bytes from 0x%02X", len, addr);
  return ESP_OK;
}

esp_err_t i2c_write_read(uint8_t addr, const uint8_t *write_data,
                         size_t write_len, uint8_t *read_data,
                         size_t read_len) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (write_data == NULL || write_len == 0 || read_data == NULL ||
      read_len == 0) {
    ESP_LOGE(TAG, "Invalid parameters for write-read");
    return ESP_ERR_INVALID_ARG;
  }

  i2c_master_dev_handle_t dev_handle = get_dev_handle(addr);
  if (dev_handle == NULL) {
    ESP_LOGE(TAG, "Device 0x%02X not found", addr);
    return ESP_ERR_NOT_FOUND;
  }

  esp_err_t ret =
      i2c_master_transmit_receive(dev_handle, write_data, write_len, read_data,
                                  read_len, 1000 / portTICK_PERIOD_MS);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error in write-read to 0x%02X: %s", addr,
             esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGD(TAG, "Write-read to 0x%02X: wrote %d bytes, read %d bytes", addr,
           write_len, read_len);
  return ESP_OK;
}

esp_err_t i2c_set_device_name(uint8_t addr, const char *name) {
  if (bus_handle == NULL) {
    ESP_LOGE(TAG, "I2C bus not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (name == NULL || strlen(name) == 0) {
    ESP_LOGE(TAG, "Invalid name");
    return ESP_ERR_INVALID_ARG;
  }

  if (i2c_manager_mutex != NULL) {
    xSemaphoreTake(i2c_manager_mutex, portMAX_DELAY);
  }

  for (int i = 0; i < device_count; i++) {
    if (registered_devices[i].address == addr) {
      strncpy(registered_devices[i].name, name,
              sizeof(registered_devices[i].name) - 1);
      registered_devices[i].name[sizeof(registered_devices[i].name) - 1] = '\0';
      ESP_LOGI(TAG, "Name of device 0x%02X changed to: %s", addr, name);

      if (i2c_manager_mutex != NULL)
        xSemaphoreGive(i2c_manager_mutex);
      return ESP_OK;
    }
  }

  ESP_LOGW(TAG, "Device 0x%02X not found for renaming", addr);

  if (i2c_manager_mutex != NULL) {
    xSemaphoreGive(i2c_manager_mutex);
  }

  return ESP_ERR_NOT_FOUND;
}
