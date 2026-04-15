#include <stdio.h>
#include <string.h>
#include "i2c_manager.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_FREQ_HZ 100000
#define MAX_DEVICES 10

static const char *TAG = "I2C_MANAGER";

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_device_t registered_devices[MAX_DEVICES];
static int device_count = 0;

esp_err_t i2c_manager_init(void)
{
    if (bus_handle != NULL) {
        ESP_LOGW(TAG, "Bus I2C ya inicializado");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Inicializando bus I2C maestro...");

    // Configurar el bus I2C maestro
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creando bus I2C maestro: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Bus I2C maestro inicializado correctamente");
    return ESP_OK;
}

void i2c_scanner(void)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado. Llama a i2c_manager_init() primero.");
        return;
    }

    ESP_LOGI(TAG, "Iniciando escaneo de dispositivos I2C...");

    // Escanear direcciones
    printf("Escaneando dispositivos I2C...\n");
    printf("Dispositivos encontrados:\n");
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t ret = i2c_master_probe(bus_handle, addr, 1000 / portTICK_PERIOD_MS);
        if (ret == ESP_OK) {
            printf("0x%02X ", addr);
        }
    }
    printf("\nEscaneo completado.\n");
}

esp_err_t i2c_register_from_scan(void)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado. Llama a i2c_manager_init() primero.");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Registrando dispositivos encontrados en el bus I2C...");

    int registered = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        esp_err_t ret = i2c_master_probe(bus_handle, addr, 1000 / portTICK_PERIOD_MS);
        if (ret == ESP_OK) {
            char name[32];
            sprintf(name, "Device_0x%02X", addr);
            ret = i2c_register_device(addr, name);
            if (ret == ESP_OK) {
                registered++;
            }
        }
    }
    ESP_LOGI(TAG, "Dispositivos registrados: %d", registered);
    return ESP_OK;
}

esp_err_t i2c_register_device(uint8_t addr, const char* name)
{
    if (bus_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (device_count >= MAX_DEVICES) {
        ESP_LOGE(TAG, "No se pueden registrar más dispositivos. Máximo: %d", MAX_DEVICES);
        return ESP_ERR_NO_MEM;
    }

    // Verificar si ya está registrado
    for (int i = 0; i < device_count; i++) {
        if (registered_devices[i].address == addr) {
            ESP_LOGW(TAG, "Dispositivo 0x%02X ya registrado", addr);
            return ESP_OK;
        }
    }

    // Configurar dispositivo
    i2c_device_config_t dev_config = {
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error agregando dispositivo 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    registered_devices[device_count].address = addr;
    registered_devices[device_count].dev_handle = dev_handle;
    strncpy(registered_devices[device_count].name, name, sizeof(registered_devices[device_count].name) - 1);
    registered_devices[device_count].name[sizeof(registered_devices[device_count].name) - 1] = '\0';
    device_count++;

    ESP_LOGI(TAG, "Dispositivo registrado: %s (0x%02X)", name, addr);
    return ESP_OK;
}

static i2c_master_dev_handle_t get_dev_handle(uint8_t addr)
{
    for (int i = 0; i < device_count; i++) {
        if (registered_devices[i].address == addr) {
            return registered_devices[i].dev_handle;
        }
    }
    return NULL;
}

esp_err_t i2c_unregister_device(uint8_t addr)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    int index = -1;
    for (int i = 0; i < device_count; i++) {
        if (registered_devices[i].address == addr) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        ESP_LOGW(TAG, "Dispositivo 0x%02X no encontrado", addr);
        return ESP_ERR_NOT_FOUND;
    }

    // Remover del bus
    esp_err_t ret = i2c_master_bus_rm_device(registered_devices[index].dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error removiendo dispositivo 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    // Desplazar el array
    for (int i = index; i < device_count - 1; i++) {
        registered_devices[i] = registered_devices[i + 1];
    }
    device_count--;

    ESP_LOGI(TAG, "Dispositivo 0x%02X de-registrado", addr);
    return ESP_OK;
}

esp_err_t i2c_unregister_all_devices(void)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < device_count; i++) {
        esp_err_t ret = i2c_master_bus_rm_device(registered_devices[i].dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Error removiendo dispositivo 0x%02X: %s", registered_devices[i].address, esp_err_to_name(ret));
            // Continuar con los demás
        }
    }
    device_count = 0;

    ESP_LOGI(TAG, "Todos los dispositivos de-registrados");
    return ESP_OK;
}

esp_err_t i2c_manager_deinit(void)
{
    if (bus_handle == NULL) {
        ESP_LOGW(TAG, "Bus I2C ya de-inicializado");
        return ESP_OK;
    }

    // De-registrar todos los dispositivos
    esp_err_t ret = i2c_unregister_all_devices();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error de-registrando dispositivos durante de-init");
        // Continuar de todos modos
    }

    // Liberar el bus
    ret = i2c_del_master_bus(bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error liberando bus I2C: %s", esp_err_to_name(ret));
        return ret;
    }

    bus_handle = NULL;
    ESP_LOGI(TAG, "Bus I2C de-inicializado correctamente");
    return ESP_OK;
}

void i2c_print_registered_devices(void)
{
    if (device_count == 0) {
        ESP_LOGI(TAG, "No hay dispositivos registrados");
        return;
    }

    ESP_LOGI(TAG, "Dispositivos registrados (%d):", device_count);
    for (int i = 0; i < device_count; i++) {
        ESP_LOGI(TAG, "  [%d] %s (0x%02X)", i, registered_devices[i].name, registered_devices[i].address);
    }
}

esp_err_t i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Datos inválidos para escritura");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_dev_handle_t dev_handle = get_dev_handle(addr);
    if (dev_handle == NULL) {
        ESP_LOGE(TAG, "Dispositivo 0x%02X no encontrado", addr);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t ret = i2c_master_transmit(dev_handle, data, len, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error escribiendo en 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Escriba %d bytes en 0x%02X", len, addr);
    return ESP_OK;
}

esp_err_t i2c_read(uint8_t addr, uint8_t *data, size_t len)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL || len == 0) {
        ESP_LOGE(TAG, "Buffer inválido para lectura");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_dev_handle_t dev_handle = get_dev_handle(addr);
    if (dev_handle == NULL) {
        ESP_LOGE(TAG, "Dispositivo 0x%02X no encontrado", addr);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t ret = i2c_master_receive(dev_handle, data, len, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error leyendo de 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Leídos %d bytes de 0x%02X", len, addr);
    return ESP_OK;
}

esp_err_t i2c_write_read(uint8_t addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len)
{
    if (bus_handle == NULL) {
        ESP_LOGE(TAG, "Bus I2C no inicializado");
        return ESP_ERR_INVALID_STATE;
    }

    if (write_data == NULL || write_len == 0 || read_data == NULL || read_len == 0) {
        ESP_LOGE(TAG, "Parámetros inválidos para write-read");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_dev_handle_t dev_handle = get_dev_handle(addr);
    if (dev_handle == NULL) {
        ESP_LOGE(TAG, "Dispositivo 0x%02X no encontrado", addr);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, write_data, write_len, read_data, read_len, 1000 / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error en write-read en 0x%02X: %s", addr, esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Write-read en 0x%02X: escriba %d bytes, leídos %d bytes", addr, write_len, read_len);
    return ESP_OK;
}

