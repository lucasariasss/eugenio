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

//tag usado para mensaje de la consola serie del ESP
static const char TAG[] = "http_server";

// Wifi connect status
static int g_wifi_connect_status = NONE;

//gestor de tareas del servidor http
static httpd_handle_t http_server_handle = NULL;

// controlador del monitor del servidor http
static TaskHandle_t task_http_server_monitor = NULL;

// controlador de la cola usado para manipular la cola principal de eventos
static QueueHandle_t http_server_monitor_queue_handle;

//archivos embebidos: jquery, index.html, app.css, app.js, favicon.ico model.png y logo.png
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


/**
 * monitor del servidor http usado para ubicar eventos del servidor http
 * @param pvParameter parameter que puede ser pasado a la tarea
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
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;
				break;

				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;

				break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");

					g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;
				break;

				case HTTP_MSG_WIFI_USER_DISCONNECT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_USER_DISCONNECT");

					g_wifi_connect_status = HTTP_WIFI_STATUS_DISCONNECTED;
				break;

				default:
				break;
			}
		}
	}
}

/*CONTROLADORES GET*/
/**
 * jquery solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "JQuery requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

/**
 * index.html solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/**
 * app.css solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

/**
 * app.js solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

/**
 * favicon.ico solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

/**
 * model.png solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_model_png_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "model.png requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)model_png_start, model_png_end - model_png_start);

	return ESP_OK;
}

/**
 * logo.png solicitado al acceder a la pagina web
 * @param req pedido HTTP que necesita ser controlado por la uri
 * @return ESP_OK
 */
static esp_err_t http_server_logo_png_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "logo.png requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)logo_png_start, logo_png_end - logo_png_start);

	return ESP_OK;
}

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

/*
 * apSSID.json handler responds by sending the ap SSID.
 * @param req HTTP request for wich the uri needs to be handled
 * @return ESP_OK
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

		//registrar el gestor de JQuery
		httpd_uri_t jquery_js = {
				.uri = "/jquery-3.3.1.min.js",
				.method = HTTP_GET,
				.handler = http_server_jquery_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		//registrar el gestor de index.html
		httpd_uri_t index_html = {
				.uri = "/",
				.method = HTTP_GET,
				.handler = http_server_index_html_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		//registrar el gestor de app.css
		httpd_uri_t app_css = {
				.uri = "/app.css",
				.method = HTTP_GET,
				.handler = http_server_app_css_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		//registrar el gestor de app.css
		httpd_uri_t app_js = {
				.uri = "/app.js",
				.method = HTTP_GET,
				.handler = http_server_app_js_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_js);

		//registrar el gestor de app.css
		httpd_uri_t favicon_ico = {
				.uri = "/favicon.ico",
				.method = HTTP_GET,
				.handler = http_server_favicon_ico_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);

		//registrar el gestor de model.png
		httpd_uri_t model_png = {
				.uri = "/model.png",
				.method = HTTP_GET,
				.handler = http_server_model_png_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &model_png);

		//registrar el gestor de model.png
		httpd_uri_t logo_png = {
				.uri = "/logo.png",
				.method = HTTP_GET,
				.handler = http_server_logo_png_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &logo_png);

		// register UARTmsg.json handler
		httpd_uri_t uart_msg_json = {
				.uri = "/UARTmsg.json",
				.method = HTTP_POST,
				.handler = http_server_uart_msg_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &uart_msg_json);

		// register wifiConnect.json handler
		httpd_uri_t wifi_connect_json = {
				.uri = "/wifiConnect.json",
				.method = HTTP_POST,
				.handler = http_server_wifi_connect_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

		// register wifiConnectStatus.json handler
		httpd_uri_t wifi_connect_status_json = {
				.uri = "/wifiConnectStatus",
				.method = HTTP_POST,
				.handler = http_server_wifi_connect_status_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

		// register wifiConnectInfo.json handler
		httpd_uri_t wifi_connect_info_json = {
				.uri = "/wifiConnectInfo.json",
				.method = HTTP_GET,
				.handler = http_server_get_wifi_connect_info_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_connect_info_json);

		// register wifiDisonnect.json handler
		httpd_uri_t wifi_disconnect_json = {
				.uri = "/wifiDisconnect.json",
				.method = HTTP_DELETE,
				.handler = http_server_wifi_disconnect_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &wifi_disconnect_json);

		// register apSSID.json handler
		httpd_uri_t ap_ssid_json = {
				.uri = "/apSSID.json",
				.method = HTTP_GET,
				.handler = http_server_get_ap_ssid_json_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &ap_ssid_json);

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
