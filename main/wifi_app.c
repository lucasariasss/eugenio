/**
 * @file wifi_app.c
 * @brief Implementación de las funcionalidades relacionadas con la aplicación WiFi.
 *
 * Este archivo contiene las funciones y definiciones necesarias para la gestión
 * y control de la conectividad WiFi en el proyecto.
 *
 * @date 26 de septiembre de 2023
 * @author arias
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"

#include "app_nvs.h"
#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"

// tag para mensajes en monitor serie
static const char TAG [] = "wifi_app";

// Callback de la aplicación WiFi
static wifi_connected_event_callback_t wifi_connected_event_cb;

// Usado para devolver la configuración wifi
wifi_config_t *wifi_config = NULL;

// Usado para rastrear el número de reintentos cuando un intento de conexión falla
static int g_retry_number;

/*
 * Manejador del grupo de eventos de la aplicación WiFi y bit de estado
 */
static EventGroupHandle_t wifi_app_event_group;
const int WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT			= BIT0;
const int WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT			= BIT1;
const int WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT		= BIT2;
const int WIFI_APP_STA_CONNECTED_GOT_IP_BIT					= BIT3;


// queue handle mara manipular la cola de tareas del main
static QueueHandle_t wifi_app_queue_handle;

//netif objets para la station y el access point
esp_netif_t* esp_netif_sta = NULL;
esp_netif_t* esp_netif_ap = NULL;

/**
 * @brief Maneja eventos de Wi-Fi e IP.
 * @param arg Contexto del manejador (no usado).
 * @param event_base Base del evento (WIFI_EVENT o IP_EVENT).
 * @param event_id Identificador del evento.
 * @param event_data Datos asociados al evento.
 * @return void
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if(event_base == WIFI_EVENT)
	{
		switch(event_id)
		{
		case WIFI_EVENT_AP_START:
			ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
			break;

		case WIFI_EVENT_AP_STOP:
			ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
			break;

		case WIFI_EVENT_AP_STACONNECTED:
			ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
			break;

		case WIFI_EVENT_AP_STADISCONNECTED:
			ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
			break;

		case WIFI_EVENT_STA_START:
			ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
			break;

		case WIFI_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
			break;

		case WIFI_EVENT_STA_DISCONNECTED:
			ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

			wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = (wifi_event_sta_disconnected_t *)event_data;
			printf("WIFI_EVENT_STA_DISCONNECTED, reason code %d\n", wifi_event_sta_disconnected -> reason);

			if (g_retry_number < MAX_CONNECTION_RETRIES)
			{
				esp_wifi_connect();
				g_retry_number ++;
			}
			else
			{
				wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
			}

			break;
		}
	}
	else if(event_base == IP_EVENT)
	{
		switch(event_id)
		{
		case IP_EVENT_STA_GOT_IP:
			ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");

			wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);

			break;
		}
	}
}

/**
 * @brief Inicializa el loop de eventos y registra el manejador de eventos Wi-Fi e IP.
 */
static void wifi_app_event_handler_init(void)
{
	//Loop del driver del wifi
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//gestor de evento para la coneccion
	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));
}

/**
 * @brief Inicializa la pila TCP/IP y la configuración Wi-Fi por defecto (STA/AP).
 */
static void wifi_app_default_wifi_init(void)
{
	//inicializa la pila TCP
	ESP_ERROR_CHECK(esp_netif_init());

	//configuracion predeterminada del wifi - las operaciones deben estar en este orden
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	esp_netif_sta = esp_netif_create_default_wifi_sta();
	esp_netif_ap = esp_netif_create_default_wifi_ap();
}

/**
 * @brief Configura el modo access point y asigna un ip estatico.
 */

static void wifi_app_soft_ap_config(void)
{
	// configuracion de softAP - wifi
	wifi_config_t ap_config=
	{
			.ap = {
					.ssid = WIFI_AP_SSID,
					.ssid_len = strlen(WIFI_AP_SSID),
					.password = WIFI_AP_PASSWORD,
					.channel = WIFI_AP_CHANNEL,
					.ssid_hidden = WIFI_AP_SSID_HIDDEN,
					.authmode = WIFI_AUTH_WPA2_PSK,
					.max_connection = WIFI_AP_MAX_CONNECTIONS,
					.beacon_interval = WIFI_AP_BEACON_INTERVAL,
			},
	};

	//Configura el DHCP para el AP
	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));

	esp_netif_dhcps_stop(esp_netif_ap);
	inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);  						//le asigna al AP un ip, gateway y netmask
	inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

	ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info)); 		//configura estaticamente la interfaz de red
	ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap)); 					//comienza el servidor DHCP de la AP (para estaciones de coneccion como un telefono)

#if HAS_STA_MODE == 1
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));	// define el modo como estacion o punto de acceso
#else
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));		// define el modo como estacion
#endif // HAS_STA_MODE
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));		//setea la configuracion
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_AP_BANDWIDTH));	//20MHz
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));
}

/**
 * @brief Configura la STA con la configuración actual y realiza la conexión.
 */
static void wifi_app_connect_sta(void)
{
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_app_get_wifi_config()));
	ESP_ERROR_CHECK(esp_wifi_connect());
}

/**
 * @brief Tarea principal de la aplicación Wi-Fi: inicializa subsistemas, inicia Wi-Fi/AP y procesa la cola de eventos.
 * @param pvParameters Parámetros de la tarea
 */
static void wifi_app_task(void *pvParameters)
{
	wifi_app_queue_message_t msg;
	EventBits_t eventBits;

	//inicializar el gestor de eventos
	wifi_app_event_handler_init();

	//inicializar el TCP/IP stack y la configuracion del wifi
	wifi_app_default_wifi_init();

	//softAP config
	wifi_app_soft_ap_config();

	//comienza el wifi
	ESP_ERROR_CHECK(esp_wifi_start());

	//enviar el primer mensaje de evento
	wifi_app_send_message(WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS);

	for(;;)
	{
		if(xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
		{
			switch(msg.msgID)
			{
				case WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS:
					ESP_LOGI(TAG, "WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS");
					if (app_nvs_load_sta_creds())
					{
						ESP_LOGI(TAG, "Loaded station configuiration");
						wifi_app_connect_sta();
						xEventGroupSetBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
					}
					else
					{
						ESP_LOGI(TAG, "Unable to load station configuration");
					}

					// A continuación, iniciar el servidor web
					wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);
					break;

				case WIFI_APP_MSG_START_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");

					http_server_start();

					break;

				case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
					ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");

					xEventGroupSetBits(wifi_app_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);

					// Intentar la conexión
					wifi_app_connect_sta();

					// Establecer el número actual de reintentos a cero
					g_retry_number = 0;

					// Informar al servidor HTTP sobre el intento de conexión
					http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_INIT);

					break;

				case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
					ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");

					xEventGroupSetBits(wifi_app_event_group, WIFI_APP_STA_CONNECTED_GOT_IP_BIT);

					http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_SUCCESS);

					eventBits = xEventGroupGetBits(wifi_app_event_group);
					if (eventBits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT) // Guardar credenciales STA solo si se conecta desde el servidor HTTP (no cargadas desde NVS)
					{
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
					}
					else
					{
						app_nvs_save_sta_creds();
					}

					if (eventBits & WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT)
					{
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
					}

					// Verificar la devolución de llamada de conexión
					if (wifi_connected_event_cb)
					{
						wifi_app_call_callback();
					}

					break;

				case WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT:
					ESP_LOGI(TAG, "WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT");

					eventBits = xEventGroupGetBits(wifi_app_event_group);

					if (eventBits & WIFI_APP_STA_CONNECTED_GOT_IP_BIT)
					{
						xEventGroupSetBits(wifi_app_event_group, WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT);

						g_retry_number = MAX_CONNECTION_RETRIES;
						ESP_ERROR_CHECK(esp_wifi_disconnect());
						//app_nvs_clear_sta_creds();
					}

					break;

				case WIFI_APP_MSG_STA_DISCONNECTED:
					ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED");

					eventBits = xEventGroupGetBits(wifi_app_event_group);
					if (eventBits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT)
					{
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: ATTEMPT USING SAVED CREDENTIALS");
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
						//app_nvs_clear_sta_creds();
					}
					else if (eventBits & WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT)
					{
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: ATTEMPT FROM THE HTTP SERVER");
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
						http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_FAIL);
					}
					else if (eventBits & WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT)
					{
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: USER REQUESTED DISCONNECTION");
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT);
						http_server_monitor_send_message(HTTP_MSG_WIFI_USER_DISCONNECT);
					}
					else
					{
						ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: ATTEMPT FAILED, CHECK WIFI ACCESS POINT AVAILABILITY");
						//Adjust this case to your needs - maybe you want to keep trying to connect
					}

					if (eventBits & WIFI_APP_STA_CONNECTED_GOT_IP_BIT)
					{
						xEventGroupClearBits(wifi_app_event_group, WIFI_APP_STA_CONNECTED_GOT_IP_BIT);
					}

					break;

				default:
					break;


			}
		}
	}
}


BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
	wifi_app_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

wifi_config_t* wifi_app_get_wifi_config(void)
{
	return wifi_config;
}

void wifi_app_set_callback(wifi_connected_event_callback_t cb)
{
	wifi_connected_event_cb = cb;
}

void wifi_app_call_callback(void)
{
	wifi_connected_event_cb();
}

int8_t wifi_app_get_rssi(void)
{
	wifi_ap_record_t wifi_data;

	ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));

	return wifi_data.rssi;
}

void wifi_app_start(void)
{
	ESP_LOGI(TAG, "STARTING WIFI APPLICATION");

	// inabilitar el mensaje predeterminado de wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// Asignar memoria para la configuración wifi
	wifi_config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
	memset(wifi_config, 0x00, sizeof(wifi_config_t));

	// Crear cola de mensajes
	wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

	// Crear grupo de eventos de la aplicación Wifi
	wifi_app_event_group = xEventGroupCreate();

	// iniciar task de wifi
	xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_APP_TASK_CORE_ID);
}

