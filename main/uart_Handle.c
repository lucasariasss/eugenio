/*
 * uart_Handle.c
 *
 *  Created on: 8 sep. 2024
 *      Author: arias
 */

#include  <stdbool.h>
#include  <stdio.h>
#include  <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include <cJSON.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "uart_Handle.h"
#include "http_server.h"

static const char TAG[] = "uart_Handle";

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)

void uart_Handle_init()
{
    ESP_LOGI(TAG, "uart_Handle_init: Inicializando UART");
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_Handle_send_message(cJSON *DISEASE)
{
    ESP_LOGI(TAG, "uart_Handle_send_message: Enviando mensaje por UART: %d", DISEASE->valueint);
}
