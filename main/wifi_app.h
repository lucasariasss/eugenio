/*
 * wifi_app.h
 *
 *  Created on: Sep 26, 2023
 *      Author: arias
 */

#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h" //netif es network interface
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"

// Callback typedef
typedef void (*wifi_connected_event_callback_t)(void);

//CONFIGURACION DE APLICACION WIFI
#define WIFI_AP_SSID 			"Eugenio_AP"
#define WIFI_AP_PASSWORD		"password"
#define WIFI_AP_CHANNEL			1
#define WIFI_AP_SSID_HIDDEN		0				// 0=visible, 1=invisible
#define WIFI_AP_MAX_CONNECTIONS	5
#define WIFI_AP_BEACON_INTERVAL	100				// 100 milisegundos
#define WIFI_AP_IP				"192.168.0.1"	// El ip cuando esta como access point
#define WIFI_AP_GATEWAY			"192.168.0.1"
#define WIFI_AP_NETMASK			"255.255.255.0"
#define WIFI_AP_BANDWIDTH		WIFI_BW_HT20 	// 20MHz
#define WIFI_STA_POWER_SAVE		WIFI_PS_NONE	// No usa el power save
#define	MAX_SSID_LENGTH			32				// Es el maximo establecido por la IEEE
#define MAX_PASSWORD_LENGTH		64				// Tambien es el maximo establecido por IEEE
#define MAX_CONNECTION_RETRIES	5				// reintentos en desconexion

//objetos de la netif para la STATION y el ACCESS POINT
extern esp_netif_t* esp_netif_sta;
extern esp_netif_t* esp_netif_ap;

//IDs de mensajes para el task manager del wifi
typedef enum wifi_app_message
{
	WIFI_APP_MSG_START_HTTP_SERVER = 0, //mandaremos esto para que lo reciba una task en rtos
	WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER, // le avisamos al wifi que se esta conectando a un server http
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP, // se le asigna un ip al router
	WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
	WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS,
	WIFI_APP_MSG_STA_DISCONNECTED,
} wifi_app_message_e; //e de enum

//struct para la cola de mensajes
typedef struct wifi_app_queue_message
{
	wifi_app_message_e msgID;
} wifi_app_queue_message_t;

/*
 * envia un mensaje a la cola
 * @param msgID ID de mensaje desde wifi_app_msg_e enum
 * @return pdTRUE si un iten se mando con exito, sino pdFALSE
 */

BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/*
 * comienza la task de wifi en rtos
 */

void wifi_app_start(void);

/**
 * Gets the wifi configuration
 */
wifi_config_t* wifi_app_get_wifi_config(void);

/**
 * Sets callback function
 */
void wifi_app_set_callback(wifi_connected_event_callback_t cb);

/*
 * Calls the callback function
 */
void wifi_app_call_callback(void);

/*
 * Gets the RSSI value of the WiFi connection.
 * @return current RSSI level
 */
int8_t wifi_app_get_rssi(void);

#endif /* MAIN_WIFI_APP_H_ */
