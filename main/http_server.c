/*
 * http_server.c
 *
 *  Created on: Oct 5, 2023
 *      Author: arias
 */

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "sys/param.h"

#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"

// Etiqueta utilizada para mensajes del monitor serie
static const char TAG[] = "http_server";

// Estado de conexión WiFi
static int g_wifi_connect_status = NONE;

//gestor de tareas del servidor http
static httpd_handle_t http_server_handle = NULL;

// controlador del monitor del servidor http
static TaskHandle_t task_http_server_monitor = NULL;

// controlador de la cola usado para manipular la cola principal de eventos
static QueueHandle_t http_server_monitor_queue_handle;

// archivos embebidos: jquery, index.html, app.css, app.js, favicon.ico model.png y logo.png
extern const uint8_t jquery_3_3_1_min_js_start[] 	asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[] 		asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[] 			asm("_binary_index_html_start");
extern const uint8_t index_html_end[] 				asm("_binary_index_html_end");
extern const uint8_t app_css_start[] 				asm("_binary_app_css_start");
extern const uint8_t app_css_end[] 					asm("_binary_app_css_end");
extern const uint8_t app_js_start[] 				asm("_binary_app_js_start");
extern const uint8_t app_js_end[] 					asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[] 			asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[] 				asm("_binary_favicon_ico_end");
extern const uint8_t model_png_start[] 				asm("_binary_model_png_start");
extern const uint8_t model_png_end[] 				asm("_binary_model_png_end");
extern const uint8_t logo_png_start[] 				asm("_binary_logo_png_start");
extern const uint8_t logo_png_end[] 				asm("_binary_logo_png_end");

/***************MONITOR***************/
/**
 * @brief Actualiza el mensaje de estado de la conexión WiFi.
 *
 * Esta función actualiza el estado de la conexión WiFi con un mensaje y estado dados.
 *
 * @param message Un puntero a un string que contiene el mensaje de estado.
 * @param status El estado de la conexión WiFi del tipo http_server_wifi_connect_status_e.
 */
static void http_server_update_wifi_connect_status(char *message, http_server_wifi_connect_status_e status)
{
	ESP_LOGI(TAG, "%s", message);
	g_wifi_connect_status = status;
}

/**
 * Esta función está destinada a monitorear el estado y el rendimiento del servidor HTTP
 * para asegurar que el servidor esté operando correctamente.
 *
 * @param parameter Un puntero a los parámetros para la tarea de monitoreo. Esto puede ser
 * utilizado para pasar información de configuración o estado a la tarea.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for(;;)
	{
		if(xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch(msg.msgID)
			{
				case HTTP_MSG_WIFI_CONNECT_INIT:
					http_server_update_wifi_connect_status("HTTP_MSG_WIFI_CONNECT_INIT", HTTP_WIFI_STATUS_CONNECTING);
				break;

				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					http_server_update_wifi_connect_status("HTTP_MSG_WIFI_CONNECT_SUCCESS", HTTP_WIFI_STATUS_CONNECT_SUCCESS);

				break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					http_server_update_wifi_connect_status("HTTP_MSG_WIFI_CONNECT_FAIL", HTTP_WIFI_STATUS_CONNECT_FAILED);
				break;

				case HTTP_MSG_WIFI_USER_DISCONNECT:
					http_server_update_wifi_connect_status("HTTP_MSG_WIFI_USER_DISCONNECT", HTTP_WIFI_STATUS_DISCONNECTED);
				break;

				default:
				break;
			}
		}
	}
}

/*CONTROLADORES GET*/
static esp_err_t http_server_static_handler(httpd_req_t *req, const char *type, const char *start, const char *end)
{
    ESP_LOGI(TAG, "%s requested", req->uri);

    httpd_resp_set_type(req, type);
    httpd_resp_send(req, start, end - start);

    return ESP_OK;
}

#define DEFINE_STATIC_HANDLER(name, type, start, end) \
    static esp_err_t name(httpd_req_t *req) { \
        return http_server_static_handler(req, type, (const char *)start, (const char *)end); \
    }

DEFINE_STATIC_HANDLER(http_server_jquery_handler, "application/javascript", jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end)
DEFINE_STATIC_HANDLER(http_server_index_html_handler, "text/html", index_html_start, index_html_end)
DEFINE_STATIC_HANDLER(http_server_app_css_handler, "text/css", app_css_start, app_css_end)
DEFINE_STATIC_HANDLER(http_server_app_js_handler, "application/javascript", app_js_start, app_js_end)
DEFINE_STATIC_HANDLER(http_server_favicon_ico_handler, "image/x-icon", favicon_ico_start, favicon_ico_end)
DEFINE_STATIC_HANDLER(http_server_model_png_handler, "image/x-icon", model_png_start, model_png_end)
DEFINE_STATIC_HANDLER(http_server_logo_png_handler, "image/x-icon", logo_png_start, logo_png_end)

/*
 * wifiConnect.json handle is invoked after the connect button is pressed
 * and handles receiving the SSID and password entered by the user
 * @param req HTTP request for wich the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnect.json requested");

	size_t len_ssid = 0, len_pass = 0;
	char *ssid_str = NULL, *pass_str = NULL;

	//get SSID header
	len_ssid = httpd_req_get_hdr_value_len(req, "my-connect-ssid") + 1;
	if(len_ssid > 1)
	{
		ssid_str = malloc(len_ssid);
		if(httpd_req_get_hdr_value_str(req, "my-connect-ssid", ssid_str, len_ssid) == ESP_OK)
		{
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: Found header => my-connect-ssid: %s", ssid_str);
		}
	}

	//get password header
	len_pass = httpd_req_get_hdr_value_len(req, "my-connect-pwd") + 1;
	if(len_pass > 1)
	{
		pass_str = malloc(len_pass);
		if(httpd_req_get_hdr_value_str(req, "my-connect-pwd", pass_str, len_pass) == ESP_OK)
		{
			ESP_LOGI(TAG, "http_server_wifi_connect_json_handler: Found header => my-connect-pwd: %s", pass_str);
		}
	}

	// Update the wifi networks configuration and let the wifi apllication know
	wifi_config_t* wifi_config = wifi_app_get_wifi_config();
	memset(wifi_config, 0x00, sizeof(wifi_config_t));
	memcpy(wifi_config -> sta.ssid, ssid_str, len_ssid);
	memcpy(wifi_config -> sta.password, pass_str, len_pass);
	wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);

	free(ssid_str);
	free(pass_str);

	return ESP_OK;
}

/*
 * wifiConnectStatus handler updates the connection status for the web page.
 * @param req HTTP request for wich the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectStatus requested");

	char statusJSON[100];

	sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, statusJSON, strlen(statusJSON));

	return ESP_OK;
}

static esp_err_t http_server_uart_msg_json_handler(httpd_req_t *req)
{
	esp_err_t ret = ESP_OK;
	ESP_LOGI(TAG, "/UARTmsg.json requested");

	char uartMsgJSON[100];
	memset(uartMsgJSON, 0, sizeof(uartMsgJSON));

	char disease[IP4ADDR_STRLEN_MAX];

	

	ESP_LOGI(TAG, "UART message received");
	ESP_LOGI(TAG, "The message is: %s", uartMsgJSON);

	sprintf(uartMsgJSON, "{\"msg\":\"%s\"}", "UART message received");

	httpd_resp_set_type(req, "application/json");
	ret = httpd_resp_send(req, uartMsgJSON, strlen(uartMsgJSON));
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "httpd_resp_send failed with error: %s", esp_err_to_name(ret));
	}
	
	return ESP_OK;
}

/*
 * wifiConnectInfo.json handler updates the webpage with the connection info.
 * @param req HTTP request for wich the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_wifi_connect_info_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/wifiConnectInfo.json requested");

	char ipInfoJSON[200];
	memset(ipInfoJSON, 0, sizeof(ipInfoJSON));

	char ip[IP4ADDR_STRLEN_MAX];
	char netmask[IP4ADDR_STRLEN_MAX];
	char gw[IP4ADDR_STRLEN_MAX];

	if (g_wifi_connect_status == HTTP_WIFI_STATUS_CONNECT_SUCCESS)
	{
		wifi_ap_record_t wifi_data;
		ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));
		char *ssid = (char*)wifi_data.ssid;

		esp_netif_ip_info_t ip_info;
		ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta, &ip_info));
		esp_ip4addr_ntoa(&ip_info.ip, ip, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.netmask, netmask, IP4ADDR_STRLEN_MAX);
		esp_ip4addr_ntoa(&ip_info.gw, gw, IP4ADDR_STRLEN_MAX);

		sprintf(ipInfoJSON, "{\"ip\":\"%s\",\"netmask\":\"%s\",\"gw\":\"%s\",\"ap\":\"%s\"}", ip, netmask, gw, ssid);
	}

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ipInfoJSON, strlen(ipInfoJSON));

	return ESP_OK;
}

/*
 * wifiDisconnect.json handler responds by sending a message to the wifi application to disconnect.
 * @param req HTTP request for wich the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_wifi_disconnect_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "wifiDisconnect.json requested");

	wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);

	return ESP_OK;
}

/**
 * @brief Manejador HTTP GET para recuperar el SSID del Punto de Acceso en formato JSON.
 *
 * Esta función maneja las solicitudes HTTP GET para obtener el SSID del Punto de Acceso
 * en formato JSON. Está destinada a ser utilizada como un manejador para el servidor HTTP.
 *
 * @param req Puntero a la estructura de solicitud HTTP.
 * 
 * @return
 *     - ESP_OK: Si la solicitud fue manejada exitosamente y se envió la respuesta.
 *     - ESP_FAIL: Si hubo un error al procesar la solicitud.
 */
static esp_err_t http_server_get_ap_ssid_json_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "/apSSID.json requested");

	char ssidJSON[50];

	wifi_config_t *wifi_config = wifi_app_get_wifi_config();
	esp_wifi_get_config(ESP_IF_WIFI_AP, wifi_config);
	char *ssid = (char*)wifi_config->ap.ssid;

	sprintf(ssidJSON, "{\"ssid\":\"%s\"}", ssid);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, ssidJSON, strlen(ssidJSON));

	return ESP_OK;
}

/************** CONFIGURAR SERVIDOR **************/

/**
 * @brief Registra un manejador para una URI específica.
 * 
 * Esta función registra un manejador para una URI específica y un método HTTP con la instancia del servidor HTTP proporcionada.
 *
 * @param uri La URI para la cual se va a registrar el manejador.
 * @param method El método HTTP (por ejemplo, GET, POST) para el cual se va a registrar el manejador.
 * @param handler El puntero a la función del manejador que será llamada cuando se solicite la URI y el método especificados.
 */
static void http_server_register_uri_handler(const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r))
{
    httpd_uri_t uri_handler = {
        .uri = uri,
        .method = method,
        .handler = handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(http_server_handle, &uri_handler);
}

/**
 * setea la configuracion predeterminada del servidor html
 * @return si la configuracion fue exitosa, devuelve la instancia del servidor http, sino NULL
 */
static httpd_handle_t http_server_configure(void)
{
	//generar configuracion predeterminada
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Crear el monitor del servidor http
	xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_CORE_ID);

	// Crear la cola de mensajes
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	config.core_id = HTTP_SERVER_TASK_CORE_ID;
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
	config.max_uri_handlers = 20;
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(TAG, "http_server_configure: starting server on port: '%d' with task priority: '%d'",
			config.server_port, config.task_priority);

	//comenzar el servidor httpd
    if(httpd_start(&http_server_handle, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_configure: Registering URI handlers");

        http_server_register_uri_handler("/jquery-3.3.1.min.js", HTTP_GET, http_server_jquery_handler);
        http_server_register_uri_handler("/", HTTP_GET, http_server_index_html_handler);
        http_server_register_uri_handler("/app.css", HTTP_GET, http_server_app_css_handler);
        http_server_register_uri_handler("/app.js", HTTP_GET, http_server_app_js_handler);
        http_server_register_uri_handler("/favicon.ico", HTTP_GET, http_server_favicon_ico_handler);
        http_server_register_uri_handler("/model.png", HTTP_GET, http_server_model_png_handler);
        http_server_register_uri_handler("/logo.png", HTTP_GET, http_server_logo_png_handler);
        http_server_register_uri_handler("/UARTmsg.json", HTTP_POST, http_server_uart_msg_json_handler);
        http_server_register_uri_handler("/wifiConnect.json", HTTP_POST, http_server_wifi_connect_json_handler);
        http_server_register_uri_handler("/wifiConnectStatus", HTTP_POST, http_server_wifi_connect_status_json_handler);
        http_server_register_uri_handler("/wifiConnectInfo.json", HTTP_GET, http_server_get_wifi_connect_info_json_handler);
        http_server_register_uri_handler("/wifiDisconnect.json", HTTP_DELETE, http_server_wifi_disconnect_json_handler);
        http_server_register_uri_handler("/apSSID.json", HTTP_GET, http_server_get_ap_ssid_json_handler);

        return http_server_handle;
    }

	return NULL;
}

void http_server_start(void)
{
	if(http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}

void http_server_stop(void)
{
	if(http_server_handle)
	{
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "http_server_stop: stopping http server");
		http_server_handle = NULL;
	}
	if(task_http_server_monitor)
	{
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}
