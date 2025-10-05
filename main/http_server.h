/**
 * @file http_server.h

 * @brief Declaraciones para el servidor HTTP.
 *
 * Este archivo contiene las definiciones y declaraciones necesarias para implementar
 * un servidor HTTP en el proyecto. Proporciona las interfaces y estructuras requeridas
 * para la gestión de peticiones y respuestas HTTP.
 *
 * @date 5 de octubre de 2023
 * @author arias
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief Estados de conexión Wi-Fi para el servidor HTTP.
 */
typedef enum http_server_wifi_connect_status
{
	NONE = 0,
	HTTP_WIFI_STATUS_CONNECTING,
	HTTP_WIFI_STATUS_CONNECT_FAILED,
	HTTP_WIFI_STATUS_CONNECT_SUCCESS,
	HTTP_WIFI_STATUS_DISCONNECTED,
} http_server_wifi_connect_status_e;

/**
 * @brief Mensajes de eventos relacionados con la conexión Wi-Fi en el servidor HTTP.
 */
typedef enum http_server_message
{
	HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_WIFI_USER_DISCONNECT,
}http_server_message_e;

/**
 * @brief Mensaje de cola para eventos del servidor HTTP.
 */
typedef struct http_server_queue_message
{
	http_server_message_e msgID;
}http_server_queue_message_t;

/**
 * @brief Inicia el servidor HTTP si no está en ejecución.
 */
void http_server_start(void);

/**
 * @brief Detiene el servidor HTTP y su tarea de monitoreo.
 */
void http_server_stop(void);

/**
 * @brief Envía un mensaje al monitor del servidor HTTP.
 * @param msgID Identificador del mensaje a enviar.
 * @return pdPASS si el mensaje fue enviado correctamente, errQUEUE_FULL en caso de error.
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

#endif /* MAIN_HTTP_SERVER_H_ */
