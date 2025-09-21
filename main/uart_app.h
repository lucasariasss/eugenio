/**
 * @file uart_app.h
 * @author Lucas Arias (1605137@ucc.edu.ar)
 * @brief Declaraciones de funciones para la inicialización y envío de mensajes por UART usando ESP-IDF.
 * @date 2024-09-8
 * 
 */

#ifndef MAIN_UART_APP_H_
#define MAIN_UART_APP_H_

#include <cJSON.h>

/**
 * @brief Inicializa la aplicación UART.
 *
 * Esta función configura y prepara el periférico UART para su uso en la aplicación.
 * Normalmente incluye la configuración de parámetros como la velocidad en baudios,
 * bits de datos, paridad, bits de parada y control de flujo, así como la instalación
 * del driver UART y la asignación de los recursos necesarios.
 *
 * @return
 *      - ESP_OK si la inicialización fue exitosa
 *      - Código de error esp_err_t correspondiente en caso contrario
 */
esp_err_t uart_app_init(void);

/**
 * @brief Envía un mensaje con la enfermedad a través de la interfaz UART configurada.
 *
 * @param enfermedad variable uint8_t que selecciona la enfermedad.
 * @note Asegúrese de que el driver UART esté inicializado antes de llamar a esta función.
 */
void uart_app_send_message(uint8_t enfermedad);


#endif /* MAIN_UARTHANDLE_H_ */
