/**
 * @file uart_app.c
 * @author Lucas Arias (1605137@ucc.edu.ar)
 * @brief Implementación de funciones para comunicación UART en la aplicación principal.
 * @date 2024-09-8
 * 
 */

#include  <stdbool.h>
#include  <stdio.h>
#include  <string.h>

#include "esp_log.h"
#include "esp_err.h"
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

void uart_app_send_message(uint8_t enfermedad)
{ 
    uint8_t *data = (uint8_t *)malloc(sizeof(int));
    if (data == NULL)
    {
        ESP_LOGE(TAG, "uart_app_send_message: Error al asignar memoria");
        return;
    } else
    {
        ESP_LOGI(TAG, "uart_app_send_message: Memoria asignada");
    }

    bzero(data, sizeof(uint8_t));
    char chardisease = enfermedad;
    ESP_LOGI(TAG, "uart_app_send_message: Enviando mensaje por UART: %x", enfermedad);
    snprintf((char *)data, sizeof(int), "%c", chardisease);
    int bytes_written = uart_write_bytes(UART_NUM, (const char *)data, strlen((char *)data));
    if (bytes_written < 0) {
        ESP_LOGE(TAG, "uart_app_send_message: Error enviando mensaje");
    } else {
        ESP_LOGI(TAG, "uart_app_send_message: Enviando %d bytes", bytes_written);
        ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM, 100));
    }
    free(data);

}

esp_err_t uart_app_init(void)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "uart_app_init: Inicializando UART");
    ret |= uart_driver_install(UART_NUM, UART_APP_TASK_STACK_SIZE, UART_APP_TASK_STACK_SIZE, 0, NULL, 0);
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

