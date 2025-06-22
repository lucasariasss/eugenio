/*
 * lcd_app.c
 *
 *  Created on: 26 abr. 2024
 *      Author: arias
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "lcd_app.h"
#include "http_server.h"
#include "tasks_common.h"

#define DATA_LENGTH 100

static const char TAG[] = "lcd_app";

i2c_master_dev_handle_t dev_handle;
i2c_master_bus_handle_t bus_handle;

i2c_device_config_t dev_cfg = {
	.dev_addr_length = I2C_ADDR_BIT_LEN_7,
	.device_address = 0x58,
	.scl_speed_hz = 100000,
};

void lcd_app_send_message(void)
{
	esp_err_t ret;
	uint8_t *data = (uint8_t *)malloc(DATA_LENGTH);
	if (data == NULL)
	{
		ESP_LOGE(TAG, "lcd_app_send_message: Error al asignar memoria");
		return;
	} else
	{
		ESP_LOGI(TAG, "lcd_app_send_message: Memoria asignada");
	}

	// Simulando un mensaje para enviar al LCD
	snprintf((char *)data, DATA_LENGTH, "Hello LCD!");

	ret = i2c_master_transmit(dev_handle, data, strlen((char *)data), 1000 / portTICK_PERIOD_MS);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "lcd_app_send_message: Error al enviar datos al LCD: %s", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "lcd_app_send_message: Datos enviados correctamente al LCD");
	}

	free(data);
}

esp_err_t lcd_app_init(void)
{
	esp_err_t ret;
	ESP_LOGI(TAG, "lcd_app_init: Inicializando maestro I2C para LCD");
	const i2c_master_bus_config_t master_config = {
	    .clk_source = I2C_CLK_SRC_DEFAULT,
	    .i2c_port = I2C_NUM,
	    .scl_io_num = SCL_PIN,
	    .sda_io_num = SDA_PIN,
	    .glitch_ignore_cnt = 7,
		.flags = {
			.enable_internal_pullup = 1,
		}
	};

	ret = i2c_new_master_bus(&master_config, &bus_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "lcd_app_init: Error al inicializar el bus maestro I2C: %s", esp_err_to_name(ret));
		return ret;
	}

	ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "lcd_app_init: Error al agregar el dispositivo I2C: %s", esp_err_to_name(ret));
		return ret;
	}

	return ret;
}
