/*
 * uart_app.c
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

#include "uart_app.h"
#include "http_server.h"
#include "tasks_common.h"

static const char TAG[] = "uart_app";

#if defined(BOOTLOADER_UART) && (BOOTLOADER_UART == 1)
#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)
#define UART_NUM (UART_NUM_0)
#else
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define UART_NUM (UART_NUM_1)
#endif

void uart_Handle_send_message(int disease)
{ 
    uint8_t *data = (uint8_t *)malloc(sizeof(int));
    if (data == NULL)
    {
        ESP_LOGE(TAG, "uart_Handle_send_message: Error al asignar memoria");
        return;
    } else
    {
        ESP_LOGI(TAG, "uart_Handle_send_message: Memoria asignada");
    }


    if (disease != 0)
    {
        bzero(data, sizeof(int));
        char chardisease = disease;
        ESP_LOGI(TAG, "uart_Handle_send_message: Enviando mensaje por UART: %x", disease);
        snprintf((char *)data, sizeof(int), "%c", chardisease);
        int bytes_written = uart_write_bytes(UART_NUM, (const char *)data, strlen((char *)data));
        if (bytes_written < 0) {
            ESP_LOGE(TAG, "uart_Handle_send_message: Error enviando mensaje");
        } else {
            ESP_LOGI(TAG, "uart_Handle_send_message: Enviando %d bytes", bytes_written);
            ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM, 100));
        }
    }
    else
    {
        ESP_LOGE(TAG, "uart_Handle_send_message: DISEASE o DISEASE->valuestring es NULL");
    }
    free(data);

}

esp_err_t uart_Handle_init(void)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "uart_Handle_init: Inicializando UART");
    ret |= uart_driver_install(UART_NUM, UART_HANDLE_TASK_STACK_SIZE, UART_HANDLE_TASK_STACK_SIZE, 0, NULL, 0);
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ret |= uart_param_config(UART_NUM, &uart_config);
    ret |= uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "uart_handle_init: UART inicializada");
    return ret;
}

