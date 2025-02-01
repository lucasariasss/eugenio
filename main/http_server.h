/*
 * http_server.h
 *
 *  Created on: Oct 5, 2023
 *      Author: arias
 */

#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/*
 * Connection status for wifi
 */
typedef enum http_server_wifi_connect_status
{
	NONE = 0,
	HTTP_WIFI_STATUS_CONNECTING,
	HTTP_WIFI_STATUS_CONNECT_FAILED,
	HTTP_WIFI_STATUS_CONNECT_SUCCESS,
	HTTP_WIFI_STATUS_DISCONNECTED,
} http_server_wifi_connect_status_e;

//mensajes del monitor http
typedef enum http_server_message
{
	HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_WIFI_USER_DISCONNECT,
}http_server_message_e;

//estructura de la cola de mensajes
typedef struct http_server_queue_message
{
	http_server_message_e msgID;
}http_server_queue_message_t;

/**
 * Enviar mensaje a la cola
 * @param msgID id del mensaje de http_server_message_e
 * @return pdTRUE si un item fue correctamente mandado a la cola, sino pdFALSE
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

/**
 * iniciar el servidor http
 */
void http_server_start(void);

/**
 * detener el servidor http
 */
void http_server_stop(void);

#endif /* MAIN_HTTP_SERVER_H_ */
