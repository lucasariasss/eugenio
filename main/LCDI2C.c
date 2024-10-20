/*
 * LCDI2C.c
 *
 *  Created on: 26 abr. 2024
 *      Author: arias
 */

#include "LCDI2C.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#define DATA_LENGTH 100

void iic_config(void)
{
	i2c_master_bus_config_t master_config = {
	    .clk_source = I2C_CLK_SRC_DEFAULT,
	    .i2c_port = -1,
	    .scl_io_num = GPIO_NUM_22,
	    .sda_io_num = GPIO_NUM_21,
	    .glitch_ignore_cnt = 7,
	};
	i2c_master_bus_handle_t bus_handle;

	ESP_ERROR_CHECK(i2c_new_master_bus(&master_config, &bus_handle));

	i2c_device_config_t dev_cfg = {
	    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
	    .device_address = 0x58,
	    .scl_speed_hz = 100000,
	};

	i2c_master_dev_handle_t dev_handle;
	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

	ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_wr, DATA_LENGTH, -1));

}

void lcd_config(void)
{

}

void lcd_display_message(void)
{
	iic_config();

	lcd_config();

	for(;;)
	{

	}
}

void start_lcd_iic(void)
{

}
